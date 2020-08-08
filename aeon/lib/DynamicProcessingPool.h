/* 
 * DynamicProcessingPool.h : part of the Mace toolkit for building distributed systems
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
#ifndef _DYNAMIC_PROCESSING_POOL_H
#define _DYNAMIC_PROCESSING_POOL_H

#include <deque>

#include "ThreadCreate.h"
#include "ScopedLock.h"
#include "mset.h"

/**
 * \file DynamicProcessingPool.h
 * \brief defines the DynamicProcessingPool class
 *
 * \todo JWA, will you document this?
 */

namespace DynamicProcessingPoolCallbacks {

template<class P, class C, class B1>
class callback_cref {
  typedef void (C::*cb_t) (const B1&);
  P c;
  cb_t f;
public:
  callback_cref(const P &cc, cb_t ff)
    : c (cc), f (ff) {}
  void operator() (const B1& b1)
    { ((*c).*f) (b1); }
};

template<class P, class C, class B1>
class callback_copy {
  typedef void (C::*cb_t) (B1);
  P c;
  cb_t f;
public:
  callback_copy(const P &cc, cb_t ff)
    : c (cc), f (ff) {}
  void operator() (B1 b1)
    { ((*c).*f) (b1); }
};
}; // namespace DynamicProcessingPoolCallbacks


template <class C, class T>
class DynamicProcessingPool {
  
public:
  DynamicProcessingPool(C p, size_t numThreads, size_t maxThreads) :
    proc(p), threadCount(numThreads), maxThreads(maxThreads), stop(false), threads(0), sleeping(0) {

    ASSERT(pthread_mutex_init(&poolMutex, 0) == 0);
    ASSERT(pthread_cond_init(&signalv, 0) == 0);

    for (size_t i = 0; i < threadCount; i++) {
      createThread();
    }
  } // DynamicProcessingPool

  virtual ~DynamicProcessingPool() {
    halt();
  }

  void halt() {
    ScopedLock sl(poolMutex);
    stop = true;
    signal();
  }

  void execute(const T& v) {
    ScopedLock sl(poolMutex);

    tasks.push_back(v);
    if (sleeping == 0 && threads < maxThreads) {
      createThread();
    }
    signal();
  } // execute

protected:
  static void* startThread(void* arg) {
    DynamicProcessingPool<C, T>* pq = (DynamicProcessingPool<C, T> *)arg;
    pq->run();
    return 0;
  } // startThread

private:
  void createThread() {
    ScopedLock sl(poolMutex);
    pthread_t t;
    runNewThread(&t, DynamicProcessingPool::startThread, this, 0);
    threads++;
    sleeping++;
  }

  void lock() const {
    ASSERT(pthread_mutex_lock(&poolMutex) == 0);
  } // lock

  void unlock() const {
    ASSERT(pthread_mutex_unlock(&poolMutex) == 0);
  } // unlock

  void signal() {
    // assumes caller has lock
    pthread_cond_broadcast(&signalv);
  } // signal

  void wait() {
    ASSERT(pthread_cond_wait(&signalv, &poolMutex) == 0);
  } // wait

  void run() {
    ScopedLock sl(poolMutex);

    while (!stop) {
      if (tasks.empty()) {
	wait();
	continue;
      }

      ASSERT(sleeping > 0);
      sleeping--;
      T t = tasks.front();
      tasks.pop_front();

      sl.unlock();

      proc(t);

      sl.lock();

      if (tasks.empty() && threads > threadCount) {
        ASSERT(threads > 0);
	threads--;
	return;
      }

      sleeping++;
      ASSERT(sleeping <= threads);
    }
  } // run


private:
  C proc;
  size_t threadCount;
  size_t maxThreads;
  bool stop;

  typedef mace::set<pthread_t> ThreadSet;
  size_t threads;
  size_t sleeping;

  mutable pthread_mutex_t poolMutex;
  pthread_cond_t signalv;

  std::deque<T> tasks;

}; // DynamicProcessingPool

#endif // _DYNAMIC_PROCESSING_POOL_H
