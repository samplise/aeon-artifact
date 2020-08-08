/* 
 * TimerHandler.cc : part of the Mace toolkit for building distributed systems
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
#include "massert.h"
#include "TimerHandler.h"
#include "Scheduler.h"
#include "NumberGen.h"
#include "ScopedLock.h"

TimerHandler::TimerHandler(std::string desc, int simulatedWeight, bool lk) :
  description(desc), simWeight(simulatedWeight), locked(lk),
  scheduled(false), running(false),
  id(NumberGen::Instance("TimerHandler")->GetVal()) {

  pthread_mutex_init(&tlock, 0);
}

TimerHandler::~TimerHandler() {
  pthread_mutex_destroy(&tlock);
}

void TimerHandler::cancel() {
  if (locked) {
    lock();
  }
  if (scheduled) {
    Scheduler::Instance().cancel(*this);
  }
  scheduled = false;
  running = false;
  if (locked) {
    unlock();
  }
} // TimerHandler::cancel

uint64_t TimerHandler::schedule(uint64_t time, bool abs, bool align) {
  if (locked) {
    lock();
  }
  else {
    ASSERT(!scheduled);
  }
    
  scheduled = true;
  running = true;

  if (align && (time % (1000*1000) == 0)) {
    uint64_t now = TimeUtil::timeu();
    uint sec = time / (1000*1000);
    uint64_t sleep = 1000*1000;
    sleep -= (now % (1000 * 1000));
    if (sleep < 500*1000) {
      sleep += 1000*1000;
    }
    time = (sec - 1) * 1000*1000 + sleep;
  }
    
  uint64_t r = Scheduler::Instance().schedule(*this, time, abs);
  if (locked) {
    unlock();
  }
  return r;
} // TimerHandler::schedule
