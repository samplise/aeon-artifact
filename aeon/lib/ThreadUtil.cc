/* 
 * ThreadUtil.cc : part of the Mace toolkit for building distributed systems
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
#include <sched.h>
#include "ThreadUtil.h"
#include "mace-macros.h"
#include "maceConfig.h"

void ThreadUtil::create(pthread_t& t, const pthread_attr_t& attr,
			void *(*start_routine)(void*), void* arg) {
  ADD_SELECTORS("ThreadUtil::create");
  traceout << Log::end;
  int r = pthread_create(&t, &attr, start_routine, arg);
  if (r != 0) {
    Log::perror("pthread_create");
    assert(0);
  }
} // create

void ThreadUtil::lock(pthread_mutex_t& mutex) {
  ADD_SELECTORS("ThreadUtil::lock");
  traceout << Log::end;
  int r = pthread_mutex_lock(&mutex);
  if (r != 0) {
    Log::perror("pthread_mutex_lock");
    assert(0);
  }
} // lock

void ThreadUtil::unlock(pthread_mutex_t& mutex) {
  ADD_SELECTORS("ThreadUtil::unlock");
  traceout << Log::end;
  int r = pthread_mutex_unlock(&mutex);
  if (r != 0) {
    Log::perror("pthread_mutex_unlock");
    assert(0);
  }
} // unlock

void ThreadUtil::wait(pthread_cond_t& signal, pthread_mutex_t& mutex) {
  ADD_SELECTORS("ThreadUtil::wait");
  traceout << Log::end;
  int r = pthread_cond_wait(&signal, &mutex);
  if (r != 0) {
    Log::perror("pthread_cond_wait");
    assert(0);
  }
} // wait

void ThreadUtil::signal(pthread_cond_t& sig) {
  ADD_SELECTORS("ThreadUtil::signal");
  traceout << Log::end;
  int r = pthread_cond_signal(&sig);
  if (r != 0) {
    Log::perror("pthread_cond_signal");
    assert(0);
  }
} // signal

void ThreadUtil::broadcast(pthread_cond_t& sig) {
  ADD_SELECTORS("ThreadUtil::broadcast");
  traceout << Log::end;
  int r = pthread_cond_broadcast(&sig);
  if (r != 0) {
    Log::perror("pthread_cond_broadcast");
    assert(0);
  }
} // broadcast

void ThreadUtil::yield() {
#ifdef OSX_SCHED_YIELD
  sched_yield();
#else
  pthread_yield();
#endif
} // yield
