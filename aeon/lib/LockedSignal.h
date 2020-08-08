/* 
 * LockedSignal.h : part of the Mace toolkit for building distributed systems
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
#ifndef _LOCKED_SIGNAL_H
#define _LOCKED_SIGNAL_H

#include "ScopedLock.h"
#include "TimeUtil.h"
#include <errno.h>

class LockedSignal {
public:
  LockedSignal() : waiting(false), signalled(false) {
    ASSERT(pthread_mutex_init(&mutex, 0) == 0);
    ASSERT(pthread_cond_init(&cond, 0) == 0);
  }
  
  virtual ~LockedSignal() {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
  }

  void signal() {
    ScopedLock sl(mutex);
    if (!signalled) {
      signalled = true;
      if (waiting) {
        pthread_cond_signal(&cond);
      }
    }
  }

  int wait(uint64_t usec = 0) {
    if (usec == 0) {
      return lockWait(0);
    }
    else {
      struct timeval t;
      struct timespec ts;
      uint64_t fintime = TimeUtil::timeu() + usec;
      TimeUtil::fillTimeval(fintime, t);
      ts.tv_sec = t.tv_sec;
      ts.tv_nsec = t.tv_usec * 1000;
      return lockWait(&ts);
    }
  }

protected:

  int lockWait(const struct timespec* ts) {
    int r = 1;
    ScopedLock sl(mutex);
    if (!signalled) {
      waiting = true;
      if (ts) {
	r = pthread_cond_timedwait(&cond, &mutex, ts);
      }
      else {
	pthread_cond_wait(&cond, &mutex);
      }
    }
    waiting = false;
    signalled = false;
    return r != ETIMEDOUT;
  }

private:
  bool waiting;
  bool signalled;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
}; // class LockedSignal


#endif // _LOCKED_SIGNAL_H
