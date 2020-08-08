/* 
 * TransportScheduler.h : part of the Mace toolkit for building distributed systems
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
#ifndef TRANSPORT_SCHEDULER_H
#define TRANSPORT_SCHEDULER_H

#include <pthread.h>
#include <vector>

#include "MaceTypes.h"
#include "Collections.h"
#include "CircularQueue.h"

#include "BaseTransport.h"

class TransportScheduler {
public:
  static const uint64_t MAX_TIMER_WAIT = 1*1000;
  static const size_t TIMER_THRESHOLD = 32768;

public:
  static void add(BaseTransportPtr t);
  static void signal(size_t bytes, bool interrupt);
  static void freeSockets() { freeSocketsRequested = true; }

private:
  class TransportSchedulerTimer : public TimerHandler {
  public:
    TransportSchedulerTimer() : TimerHandler("TransportSchedulerTimer", 1, true) { }
    virtual ~TransportSchedulerTimer() { }
    void expire();
  };

  friend class TransportSchedulerTimer;

private:
  static void signal();
  TransportScheduler();
  ~TransportScheduler();
  static void run();
  static void halt();
  static void runSchedulerThread();
  static void* startSchedulerThread(void* arg);
  static void lock() { assert(pthread_mutex_lock(&slock) == 0); }
  static void unlock() { assert(pthread_mutex_unlock(&slock) == 0); }

private:
  typedef std::vector<BaseTransportPtr> TransportList;
  static TransportList transports;

  typedef CircularQueue<BaseTransportPtr> PendingTransportList;
  static PendingTransportList pendingTransports;
  
  static bool running;
  static bool freeSocketsRequested;
  static fd_set rset;
  static fd_set wset;

  static TransportSchedulerTimer *timer;
  static size_t bytesQueued;
  static bool inSelect;

  static pthread_t schedulerThread;
  static pthread_mutex_t slock;
  static const time_t SELECT_TIMEOUT_MICRO;
  static const uint32_t CHECK_RUNNING_COUNT;
}; // TransportScheduler
  
#endif // TRANSPORT_SCHEDULER_H
