/* 
 * TimeUtil.h : part of the Mace toolkit for building distributed systems
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
#include <time.h>
#include <sys/time.h>
#include <stdint.h>

#ifndef __TIME_UTIL_H
#define __TIME_UTIL_H

/**
 * \file TimeUtil.h
 * \brief Defines functions for getting the time of day.
 */

/// Define a Mace-friendly type for the difference in two times, in micro-seconds.
typedef int64_t m_suseconds_t;

namespace mace {

}

/**
 * \addtogroup Utils
 * @{
 */

/**
 * \brief A Mace-friendly class for getting the time of day.  
 *
 * Implemented using virtual functions to allow the simulator to provide
 * alternate implementations of the time functions.
 */
class TimeUtil {
  protected:
    static TimeUtil* _inst;
    TimeUtil() { }
    static TimeUtil& Instance();

    virtual uint64_t timeuImpl(); 
    virtual time_t timeImpl();

  public:
    static uint64_t timeu() { return Instance().timeuImpl(); } ///< return the current time as microseconds.
    static time_t time() { return Instance().timeImpl(); } ///< return the current time as seconds.

    static double timed(); ///< return the current time as a double value in seconds and partial microseconds.
    static void fillTimeval(uint64_t t, struct timeval& tv); ///< fills a timeval based on a timestamp t.
    static uint64_t timeu(const struct timeval& tv); ///< returns the number of microseconds represented by timeval \c tv.
    static m_suseconds_t timediff(const timeval& start, const timeval& end); ///< return the number of microseconds between start and end.

    virtual ~TimeUtil() {}
};

/** @} */

#endif //__TIME_UTIL_H
