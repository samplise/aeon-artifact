/* 
 * TransportScheduler.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson, Charles Killian
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the names of the contributors, nor their associated universities 
 *      or organizations may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ----END-OF-LEGAL-STUFF---- */
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <sys/resource.h>
#include <inttypes.h>

#include "PipedSignal.h"
#include "SysUtil.h"
#include "TransportScheduler.h"
#include "ThreadCreate.h"
#include <signal.h>
#include "Pipeline.h"
//#include "mace-macros.h"

using std::max;
using std::cerr;
using std::endl;
using std::ostream;


TransportScheduler::TransportList TransportScheduler::transports;
TransportScheduler::PendingTransportList TransportScheduler::pendingTransports;
pthread_t TransportScheduler::schedulerThread;
bool TransportScheduler::running = false;
bool TransportScheduler::freeSocketsRequested = false;
fd_set TransportScheduler::rset;
fd_set TransportScheduler::wset;
const time_t TransportScheduler::SELECT_TIMEOUT_MICRO = 500*1000;
const uint32_t TransportScheduler::CHECK_RUNNING_COUNT = 1000*1000 / SELECT_TIMEOUT_MICRO;
pthread_mutex_t TransportScheduler::slock = PTHREAD_MUTEX_INITIALIZER;
PipedSignal* sig = NULL;
TransportScheduler::TransportSchedulerTimer* TransportScheduler::timer;
size_t TransportScheduler::bytesQueued = 0;
bool TransportScheduler::inSelect = false;

// size_t _TS_deferredCount = 0;
// size_t _TS_byteCount = 0;
// size_t _TS_schedCount = 0;

TransportScheduler::TransportScheduler() {
} // TransportScheduler

TransportScheduler::~TransportScheduler() {
} // ~TransportScheduler

static void createPipeline(BaseTransportPtr t) {
  PipelineElement* p = NULL;
  Pipeline::PipelineList list;

  if (params::get("USE_FLOW_PIPELINE", false)) {
    list.push_back(new FlowPipeline());
  }

  if (params::get("USE_LOGICAL_CLOCK", false)) {
    list.push_back(new LogicalClockPipeline());
  }

  if (list.size() == 1) {
    p = list.front();
  }
  else if (list.size() > 1) {
    p = new Pipeline(list);
  }

  t->setPipeline(p);
}

void TransportScheduler::add(BaseTransportPtr t) {
  ScopedLock sl(slock);
  createPipeline(t);
  pendingTransports.push(t);
//   Log::log("TransportScheduler::add") << "added transport, count=" << transports.size()
// 				      << Log::endl;
  run();
} // add

void TransportScheduler::run() {
  if (running) {
    return;
  }


  struct rlimit fdlim;
  ASSERT(getrlimit(RLIMIT_NOFILE, &fdlim) == 0);
  if (fdlim.rlim_cur > FD_SETSIZE) {
    fdlim.rlim_cur = FD_SETSIZE;
    ASSERT(setrlimit(RLIMIT_NOFILE, &fdlim) == 0);
  }
  
  running = true;

  Log::log("TransportScheduler::run") << "creating piped signal for the transport scheduler" << Log::endl;
  ASSERT(sig == NULL);
  sig = new PipedSignal();
  timer = new TransportSchedulerTimer();

  Log::log("TransportScheduler::run") << "creating schedulerThread" << Log::endl;
  runNewThread(&schedulerThread, TransportScheduler::startSchedulerThread, 0, 0);
} // run

void TransportScheduler::halt() {
  ADD_SELECTORS("TransportScheduler::halt");
  maceout << "halting" << Log::endl;
  running = false;
} // halt

void* TransportScheduler::startSchedulerThread(void* arg) {
  runSchedulerThread();
  return 0;
} // startSchedulerThread

void TransportScheduler::runSchedulerThread() {
  ADD_SELECTORS("TransportScheduler::runSchedulerThread");
  #ifndef NO_SIGPIPE
  SysUtil::signal(SIGPIPE, SIG_IGN);
  #endif
  uint64_t cumulativeSelect = 0;
  uint64_t cumulativeSelectCount = 0;
  uint64_t shortestSelect = UINT_MAX;
  uint64_t longestSelect = 0;

  uint32_t count = 0;
  ASSERT(sig != NULL);

  while (running) {

    while (!pendingTransports.empty()) {
      BaseTransportPtr t = pendingTransports.front();
      transports.push_back(t);
      macedbg(1) << "Added transport, size " << transports.size() << Log::endl;
      pendingTransports.pop();
    }

    FD_ZERO(&rset);
    FD_ZERO(&wset);

    socket_t selectMax = 0;
    sig->addToSet(rset, selectMax);

    int n = 0;

    if (count % CHECK_RUNNING_COUNT == 0) {
      lock();
      TransportList::iterator rem = transports.begin();
      while (rem != transports.end()) {
	       if (!(*rem)->isRunning()) {
	          rem = transports.erase(rem);
            macedbg(1) << "Removed transport, remaining size " << transports.size() << Log::endl;
	       } else {
	         rem++;
	       }
      }
      unlock();
    }

    if (transports.empty()) {
      halt();
      continue;
    }

    for (size_t i = 0; i < transports.size(); i++) {
      if (freeSocketsRequested) {
	       transports[i]->freeSockets();
      }
      transports[i]->addSockets(rset, wset, selectMax);
    }
    freeSocketsRequested = false;

    if (selectMax > 0) {
      selectMax++;
    }
    count++;

    struct timeval polltv = { 0, SELECT_TIMEOUT_MICRO };
    if (!macedbg(2).isNoop()) {
      macedbg(2) << "Selecting with selectMax " << selectMax << Log::endl;
    }
    uint64_t before = TimeUtil::timeu();
    inSelect = true;
    n = SysUtil::select(selectMax, &rset, &wset, 0, &polltv);
    inSelect = false;
    uint64_t diff = TimeUtil::timeu() - before;
    if (!macedbg(2).isNoop()) {
      macedbg(2) << "Done selecting with selectMax " << selectMax << ": select returned " << n << Log::endl;
    }
    cumulativeSelect += diff;
    cumulativeSelectCount++;
    if (diff < shortestSelect) {
      shortestSelect = diff;
    }
    if (diff > longestSelect) {
      longestSelect = diff;
    }

    if (n < 0) {
      Log::perror("select");
      ABORT("select");
    }
    else if (n == 0) {
      continue;
    }

//     macedbg(1) << "calling clear on signal" << Log::endl;
//     uint64_t sc0 = TimeUtil::timeu();
    sig->clear(rset);
//     uint64_t sc1 = TimeUtil::timeu();
//     uint64_t scdiff = sc1 - sc0;
//     macedbg(1) << "signal cleared in " << scdiff << Log::endl;


    // eventually replace this with a thread pool
    for (size_t i = 0; i < transports.size(); i++) {
      // macedbg(1) << "calling doIO on transport " << i << Log::endl;
      transports[i]->doIO(rset, wset, before);
    }

  }

  transports.clear();
  maceout << "exiting" << Log::endl;
  if (cumulativeSelectCount > 0) {
    maceout << "select stats: cumulative = " << cumulativeSelect
	    << " -- count = " << cumulativeSelectCount 
	    << " -- avg = " << (cumulativeSelect/cumulativeSelectCount)
	    << " -- min = " << shortestSelect
	    << " -- max = " << longestSelect
	    << Log::endl;
  }
  else {
    maceLog("select stats: select never called by the TransportScheduler.\n");
  }
//   maceout << "deferredCount=" << _TS_deferredCount
// 	  << " byteCount=" << _TS_byteCount
// 	  << " schedCount=" << _TS_schedCount << Log::endl;
  
} // runSchedulerThread

void TransportScheduler::signal(size_t b, bool interrupt) {
  bytesQueued += b;
  if (interrupt || bytesQueued > TransportScheduler::TIMER_THRESHOLD) {
//     _TS_byteCount++;
    timer->cancel();
    signal();
    return;
  }
  timer->schedule(TransportScheduler::MAX_TIMER_WAIT);
//   else {
//     _TS_deferredCount++;
//   }
} // signal

void TransportScheduler::signal() {
  if (inSelect) {
    inSelect = false;
    sig->signal();
  }
  bytesQueued = 0;
} // signal

void TransportScheduler::TransportSchedulerTimer::expire() {
  TransportScheduler::signal();
} // expire

