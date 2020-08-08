/* 
 * MaceTime.cc : part of the Mace toolkit for building distributed systems
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
#include "MaceTime.h"

bool mace::MaceTime::simulated = false;
mace::MonotoneTimeImpl *mace::MonotoneTimeImpl::mt = NULL;
mace::MonotoneTimeInit mace::MonotoneTimeInit::init;
double mace::LogicalClock::LOG_PATH_PCT;

// uint64_t mace::MaceTime::timeofday = 1000000000;


pthread_key_t mace::LogicalClock::ThreadSpecific::pkey;
pthread_once_t mace::LogicalClock::ThreadSpecific::keyOnce = PTHREAD_ONCE_INIT;
mace::LogicalClock* mace::LogicalClock::inst = NULL;

mace::LogicalClock::ThreadSpecific::ThreadSpecific() :
  pending(0), path(0), logPath(true), mid(0), rclock(0), curclock(0) {
} // ThreadSpecific

mace::LogicalClock::ThreadSpecific::~ThreadSpecific() {
} // ~ThreadSpecific

mace::LogicalClock::ThreadSpecific* mace::LogicalClock::ThreadSpecific::init() {
  pthread_once(&keyOnce, mace::LogicalClock::ThreadSpecific::initKey);
  ThreadSpecific* t = (ThreadSpecific*)pthread_getspecific(pkey);
  if (t == 0) {
    t = new ThreadSpecific();
    assert(pthread_setspecific(pkey, t) == 0);
  }

  return t;
} // init

void mace::LogicalClock::ThreadSpecific::initKey() {
  assert(pthread_key_create(&pkey, NULL) == 0);
} // initKey

// void mace::LogicalClock::ThreadSpecific::setPendingClock(lclock_t val) {
//   ThreadSpecific* t = (ThreadSpecific*)init();
//   t->pending = std::max(t->pending, val);
// } // setPendingClock

void mace::LogicalClock::ThreadSpecific::setPendingClock(lclock_t val,
							 lclockpath_t pv,
							 uint64_t id,
							 lclock_t cur,
							 bool lp) {
  ThreadSpecific* t = (ThreadSpecific*)init();
  t->pending = std::max(t->pending, val);
  ASSERT(t->path == 0);
  t->path = pv;
  t->logPath = lp;
  ASSERT(t->mid == 0);
  t->mid = id;
  t->rclock = val;
  ASSERT(t->curclock == 0);
  t->curclock = cur;
} // setPendingClock

bool mace::LogicalClock::ThreadSpecific::clockAdvanced(lclock_t cur,
						       lclock_t& cl,
						       lclockpath_t& p,
						       bool& lp) {
  ThreadSpecific* t = (ThreadSpecific*)init();
  bool r = (t->curclock < cur);
  cl = t->pending;
  p = t->path;
  lp = t->logPath;
  t->curclock = 0;
  t->path = 0;
  t->logPath = true;
  t->mid = 0;
  return r;
} // isCurrentClock

void mace::LogicalClock::ThreadSpecific::getPending(lclock_t& cl,
						    lclockpath_t& p,
						    uint64_t& id,
						    lclock_t& rcl,
						    bool& logp) {
  ThreadSpecific* t = (ThreadSpecific*)init();
  cl = t->pending;
  p = t->path;
  logp = t->logPath;
  t->path = 0;
  t->logPath = true;
  if (p == 0) {
    p = TimeUtil::timeu();
    //Note: computation of xweight and mweight as const unsigned means that LOG_PATH_PCT cannot change during the run.  
    //  If it is desired to change, then these need to be made non-const and non-static.
    //This change made to be more modelchecker friendly.
    if (LOG_PATH_PCT < 1) {
      static const unsigned mweight = (unsigned)(LOG_PATH_PCT * 1000);
      static const unsigned xweight = 1000 - mweight;
      logp = RandomUtil::randInt(2, xweight, mweight);
    }
    else {
      logp = true;
    }
  }
  id = t->mid;
  t->mid = 0;
  rcl = t->rclock;
  t->rclock = 0;
  t->curclock = 0;
} // getPending

// mace::LogicalClock::lclock_t mace::LogicalClock::ThreadSpecific::getPendingClock() {
//   ThreadSpecific* t = (ThreadSpecific*)init();
//   return t->pending;
// } // getPendingClock

// mace::LogicalClock::lclockpath_t mace::LogicalClock::ThreadSpecific::getPendingPath() {
//   ThreadSpecific* t = (ThreadSpecific*)init();
//   lclockpath_t r = t->path;
//   t->path = 0;
//   if (r == 0) {
//     r = TimeUtil::timeu();
//   }
//   return r;
// } // getPendingPath

// mace::LogicalClock::lclock_t mace::LogicalClock::ThreadSpecific::getRemoteClock() {
//   ThreadSpecific* t = (ThreadSpecific*)init();
//   lclock_t r = t->rclock;
//   t->rclock = 0;
//   return r;
// } // getRemoteClock

// void mace::LogicalClock::ThreadSpecific::setPendingPath(lclockpath_t val) {
//   ThreadSpecific* t = (ThreadSpecific*)init();
//   t->path = val;
// } // setPendingPath
