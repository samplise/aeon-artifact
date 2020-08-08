/* 
 * ThreadCreate.cc : part of the Mace toolkit for building distributed systems
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
// #include <errno.h>

#include <sys/types.h>
#include <unistd.h>
#include "ThreadCreate.h"
#include "Log.h"
#include "massert.h"
#include "pip_includer.h"
#include "Util.h"
#include "Scheduler.h"
#include "maceConfig.h"

// _syscall0(pid_t,gettid)

static uint64_t getVtid() {
  static pthread_mutex_t vtidlock = PTHREAD_MUTEX_INITIALIZER;
  static uint64_t tid = 0;
  ScopedLock sl(vtidlock);
  return tid++;
}


struct FuncArg {
  func f;
  RunThreadClass* c;
  classfunc cf;
  void* arg;
  uint64_t vtid;
  std::string fname;
  bool joinThread;
  FuncArg(func tf, RunThreadClass* tc, classfunc tcf, void* targ, 
	  uint64_t tvtid, const char* fname, bool join) : f(tf), c(tc), cf(tcf),
							 arg(targ), vtid(tvtid),
							 fname(fname),
							 joinThread(join) {}
};

#ifndef HAVE_GETPPID
static unsigned int getppid() { return 0; }
#endif

void logThread(uint64_t vtid, const std::string& fname, bool ending) {
  Log::log("logThread") << fname << " :: pid = " << getpid() << " :: ppid = " << getppid() << " :: v_pthread_id = " << vtid << " :: v_parent_pthread_id = UNKNOWN " << (ending ? "ending" : "starting") << Log::endl;
  //XXX: CK - could use thread local storage to support parent t thread id.  
}

void* threadStart(void* vfa) {
  FuncArg* fa = (FuncArg*)vfa;
  #ifdef PIP_MESSAGING
  ANNOTATE_SET_PATH_ID_STR(NULL, 0, "thread-%s-%d", Util::getAddrString(Util::getMaceAddr()).c_str(), (int)pthread_self());
  #endif
  Log::traceNewThread(fa->fname);
  logThread(fa->vtid, fa->fname);
  //NOTE: Here would be a great place to add exception handling code, etc.
  if(fa->f != NULL) {
    (*(fa->f))(fa->arg);
  } else {
    ASSERT(fa->c != NULL);
    ASSERT(fa->cf != NULL);
    (fa->c->*(fa->cf))(fa->arg);
  }
  logThread(fa->vtid, fa->fname, true);

  if (fa->joinThread) {
    Scheduler::Instance().joinThread(fa->vtid, pthread_self());
  }

  delete fa;

  return 0;
}

void _runNewThread(pthread_t* t, func f, void* arg, pthread_attr_t* attr, 
		   const char* fname, bool joinThread) {
  int ret;
  FuncArg *fa = new FuncArg(f, NULL, NULL, arg, getVtid(), fname, 
			    joinThread);
  if((ret = pthread_create(t, attr, threadStart, fa)) != 0) {
    perror("pthread_create");
    Log::err() << "Error " << ret << " in creating thread!" << Log::endl;
    abort();
  }
  if (joinThread) {
    Scheduler::Instance().shutdownJoinThread(fa->vtid, *t);
  }
}

void _runNewThreadClass(pthread_t* t, RunThreadClass* c, classfunc f, 
			void* arg, pthread_attr_t* attr, const char* fname,
			bool joinThread) {
  int ret;
  FuncArg *fa = new FuncArg(NULL, c, f, arg, getVtid(), fname, 
			    joinThread);
  if((ret = pthread_create(t, attr, threadStart, fa)) != 0) {
    perror("pthread_create");
    Log::err() << "Error " << ret << " in creating thread!" << Log::endl;
    abort();
  }
  if (joinThread) {
    Scheduler::Instance().shutdownJoinThread(fa->vtid, *t);
  }
}
