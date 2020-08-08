/* 
 * Scheduler.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, Dejan Kostic, Ryan Braud, James W. Anderson, John Fisher-Ogden, Calvin Hubble, Duy Nguyen, Justin Burke, David Oppenheimer, Amin Vahdat, Adolfo Rodriguez, Sooraj Bhat
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
#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <sys/types.h>       
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include "mmultimap.h"
#include "ThreadCreate.h"
#include "NumberGen.h"
#include "LockedSignal.h"

/**
 * \file Scheduler.h 
 * \brief defines the Scheduler and TimerHandler classes.
 */

#include "TimerHandler.h"

/**
 * \brief The Scheduler class is responsible for firing timers and joining
 * ended threads.
 *
 * Threads in Mace should be started using the runNewThread or
 * runNewThreadClass macros.  They then call shutdownJoinThread() and
 * joinThread(), allowing the Scheduler to properly join each of them.
 *
 * The Scheduler remains one of the tricky pieces of code to get right.
 * Finding new bugs in the (relatively short and simple) schedule has been
 * common.
 */
class Scheduler : public RunThreadClass {

  friend void _runNewThread(pthread_t* t, func f, void* arg, 
			    pthread_attr_t* attr, const char* fname, bool joinThread);
  friend void _runNewThreadClass(pthread_t* t, RunThreadClass* c, classfunc f,
				 void* arg, pthread_attr_t* attr, 
				 const char* fname, bool joinThread);
  friend void* threadStart(void* vfa);
  friend class TimerHandler;

public:
  virtual ~Scheduler();
  static void haltScheduler(); ///< tells the schedule thread to join all remaining threads and shutdown.  Then this thread waits to join the scheduler thread.

  static const uint64_t CLOCK_RESOLUTION = 1000; ///< 1ms, the assumed smallest sleep period
  static bool simulated() { return Scheduler::Instance().isSimulated(); } ///< returns whether or not the scheduler is simulated

protected:
  typedef mace::map<uint64_t, pthread_t, mace::SoftState> IdThreadMap;
  virtual void runSchedulerThread();
  void fireTimer(bool locked);
  void joinThread(uint64_t vtid, pthread_t tid);
  void shutdownJoinThread(uint64_t vtid, pthread_t tid);
  size_t joinThreads(IdThreadMap& s);
  void lock() { assert(pthread_mutex_lock(&slock) == 0); }
  void unlock() { assert(pthread_mutex_unlock(&slock) == 0); }

  virtual uint64_t schedule(TimerHandler& timer, uint64_t time, bool abs = false); 
  virtual void cancel(TimerHandler& timer);
  virtual bool isSimulated() { return false; }

  static void* startSchedulerThread(void* arg);

  static Scheduler& Instance() {
    if (scheduler == 0) {
      scheduler = new Scheduler();
      _runNewThread(&scheduler->schedulerThread, 
		    Scheduler::startSchedulerThread, scheduler, 0, 
		    "Scheduler::startSchedulerThread", false);
    }
    return *scheduler;
  }



protected:
  bool running;
  uint64_t next;

  LockedSignal sig;

  typedef mace::multimap<uint64_t, TimerHandler*, mace::SoftState> TimerMap;
  TimerMap timers;

  IdThreadMap joinSet;
  IdThreadMap shutdownJoinSet;

  pthread_mutex_t slock;

  static Scheduler* scheduler;
  Scheduler(); //This is protected so the SimScheduler may use it.

private:  
  pthread_t schedulerThread;
}; // Scheduler

#endif // _SCHEDULER_H
