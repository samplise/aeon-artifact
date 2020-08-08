/* 
 * Stopwatch.h : part of the Mace toolkit for building distributed systems
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
#include "massert.h"

/**
 * \file Stopwatch.h
 * \brief Defines the mace::Stopwatch class for measuring the cumulative time.
 */

namespace mace {
/**
 * \brief used to measure cumulative time.
 *
 * Common usage 1 (start and stop around timed work):
 * \code
 * mace::Stopwatch s;
 * 
 * s.start();
 * doSomeWork();
 * s.stop();
 * 
 * unrelatedWork();
 *
 * s.start();
 * doSomeWork();
 * s.stop();
 *
 * cout << "Time: " << s.getCumulativeTime();
 * \endcode
 *
 * Common usage 2 (when not sure whether to count iteration):
 * \code
 * mace::Stopwatch s;
 * 
 * do {
 *   s.start();
 *   doSomeWork();
 *   s.mark();
 *   if (workWasInteresting()) {
 *     s.confirm();
 *   }
 * } while (notDone());
 *
 * cout << "Time: " << s.getCumulativeTime();
 * \endcode
 *
 */
class Stopwatch {
  private:
  uint64_t cumulativeTime;
  uint64_t startTime;
  uint64_t markTime;
  public:
  /// set the start time
  void start() {
    ASSERT(startTime == 0);
    startTime = TimeUtil::timeu();
  }
  /// set a potential stop time
  void mark() {
    ASSERT(startTime != 0);
    markTime = TimeUtil::timeu();
  }
  /// accumulate markTime - startTime on the watch
  void confirm() {
    ASSERT(markTime != 0);
    cumulativeTime += markTime - startTime;
    markTime = 0;
    startTime = 0;
  }
  /// mark(), then confirm()
  void stop() {
    ASSERT(startTime != 0);
    mark();
    confirm();
  }
  /// return the cumulative of the time
  uint64_t getCumulativeTime() { return cumulativeTime; }
};
}
