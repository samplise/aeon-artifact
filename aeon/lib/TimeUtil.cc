/* 
 * TimeUtil.cc : part of the Mace toolkit for building distributed systems
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
#include "TimeUtil.h"

TimeUtil& TimeUtil::Instance() {
  if (_inst == NULL) {
    _inst = new TimeUtil();
  }
  return *_inst;
}

uint64_t TimeUtil::timeuImpl() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return timeu(tv);
}

time_t TimeUtil::timeImpl() {
  return ::time(0);
}

void TimeUtil::fillTimeval(uint64_t t, struct timeval& tv) {
  tv.tv_sec = t / 1000000;
  tv.tv_usec = t % 1000000;
} // fillTimeval

uint64_t TimeUtil::timeu(const struct timeval& tv) {
  return (((uint64_t)tv.tv_sec * 1000000) + tv.tv_usec);  
} 

m_suseconds_t TimeUtil::timediff(const timeval& start, const timeval& end) {
  return ((int64_t)end.tv_usec - start.tv_usec) + (((int64_t)end.tv_sec - start.tv_sec) * 1000000);
} // timediff

double TimeUtil::timed() {
  return timeu() / 1000000.0;
} // timed

TimeUtil* TimeUtil::_inst = NULL;
