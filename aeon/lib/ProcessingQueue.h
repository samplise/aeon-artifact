/* 
 * ProcessingQueue.h : part of the Mace toolkit for building distributed systems
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
#ifndef _PROCESSING_QUEUE_H
#define _PROCESSING_QUEUE_H

#include <pthread.h>
#include <cassert>

#include "CircularQueueList.h"
#include "Exception.h"
#include "ThreadCreate.h"

/**
 * \file ProcessingQueue.h
 * \brief defines ProcessingQueue class for handling a queue of jobs
 * \todo JWA, please document
 */

namespace ProcessingQueueCallbacks {
template<class P, class C, class R, class B1>
class callback_cref {
  typedef R (C::*cb_t) (const B1&);
  P c;
  cb_t f;
public:
  callback_cref(const P &cc, cb_t ff)
    : c (cc), f (ff) {}
  R operator() (const B1& b1)
    { return ((*c).*f) (b1); }
};

template<class P, class C, class R, class B1>
class callback_copy {
  typedef R (C::*cb_t) (B1);
  P c;
  cb_t f;
public:
  callback_copy(const P &cc, cb_t ff)
    : c (cc), f (ff) {}
  R operator() (B1 b1)
    { return ((*c).*f) (b1); }
};
};

class ProcessingQueueException : public virtual Exception {
public:
  ProcessingQueueException(const std::string& m) : Exception(m) { }
  virtual ~ProcessingQueueException() throw() {}
  void rethrow() const { throw *this; }
}; // ProcessingQueueException

template <class C, class R, class T>
class ProcessingQueue {

public:
  ProcessingQueue(C p, pthread_cond_t* sig = 0, pthread_mutex_t* siglock = 0) :
    proc(p), resultSignal(sig), resultLock(siglock), halt(false), busy(false) {

    pthread_mutex_init(&taskLock, 0);
    pthread_cond_init(&taskSignal, 0);

    _runNewThread(&processingThread, startProcessingThread, this, 0, 
		  "startProcessingThread", false);
  } // ProcessingQueue

  virtual ~ProcessingQueue() {
    shutdown();
    
    assert(pthread_join(processingThread, NULL) == 0);
    pthread_mutex_destroy(&taskLock);
    pthread_cond_destroy(&taskSignal);
  } // ~ProcessingQueue

  void shutdown() {
    halt = true;
    signalTask();
  }
  
  void push(const T& v) {
    busy = true;
    tasks.push(v);
    signalTask();
  } // push

  const R& front() const throw (ProcessingQueueException) {
    if (results.empty()) {
      throw ProcessingQueueException("dequeue called on empty queue");
    }
    const R& r = results.front();
    return r;
  } // front

  void pop() throw (ProcessingQueueException) {
    if (results.empty()) {
      throw ProcessingQueueException("dequeue called on empty queue");
    }
    results.pop();
  } // pop

  // clear the result queue
  void clear() {
    results.clear();
  } // clear

  // return true if there are no results (cannot call dequeue)
  bool empty() {
    return results.empty();
  } // empty

  // return number of pending results
  size_t size() {
    return results.size();
  } // size

  void clearQueue() {
    tasks.clear();
  } // clearQueue

  // return true if there are no processing tasks
  bool queueEmpty() {
    return tasks.empty();
  } // queueEmpty

  // return numbef or pending processing tasks
  size_t queueSize() {
    return tasks.size();
  } // queueSize

  bool isBusy() const {
    return busy;
  } // isBusy

  void setResultSignal(pthread_cond_t* sig, pthread_mutex_t* mutex) {
    resultSignal = sig;
    resultLock = mutex;
  } // setResultSignal

  void clearResultSignal() {
    resultSignal = 0;
  } // clearResultSignal

protected:
  static void* startProcessingThread(void* arg) {
    ProcessingQueue<C, R, T>* pq = (ProcessingQueue<C, R, T> *)arg;
    pq->processQueue();
    return 0;
  } // startProcessingThread

private:
  void signalTask() {
    acquireLock(taskLock);
    signalCond(taskSignal);
    releaseLock(taskLock);
  }

  void processQueue() {
    while (1) {
      while (tasks.empty() && !halt) {
	busy = false;
	acquireLock(taskLock);
	waitForCond(taskSignal, taskLock);
	releaseLock(taskLock);
      }
      if (halt) {
	busy = false;
	return;
      }

      T& t = tasks.front();

      R r = proc(t);

      tasks.pop();

      results.push(r);

      if (resultSignal) {
	acquireLock(*resultLock);
	signalCond(*resultSignal);
	releaseLock(*resultLock);
      }
    }
  } // processQueue

  void acquireLock(pthread_mutex_t& l) const {
    assert(pthread_mutex_lock(&l) == 0);
  } // acquireLock

  void releaseLock(pthread_mutex_t& l) const {
    assert(pthread_mutex_unlock(&l) == 0);
  } // releaseLock

  void signalCond(pthread_cond_t& c) {
    assert(pthread_cond_signal(&c) == 0);
  } // signalCond

  void waitForCond(pthread_cond_t& c, pthread_mutex_t& l) {
    // assume caller has lock
    assert(pthread_cond_wait(&c, &l) == 0);
  } // waitForCond

private:
  C proc;
  pthread_cond_t* resultSignal;
  pthread_mutex_t* resultLock;
  bool halt;
  bool busy;

  pthread_t processingThread;
  mutable pthread_mutex_t taskLock;
  mutable pthread_cond_t taskSignal;

  CircularQueueList<T> tasks;
  CircularQueueList<R> results;
  
  
}; // ProcessingQueue

#endif // _PROCESSING_QUEUE_H
