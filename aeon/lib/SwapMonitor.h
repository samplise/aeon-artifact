/* 
 * SwapMonitor.h : part of the Mace toolkit for building distributed systems
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

/* Reports periodic system (virtual memory) swap statistics: cumulative pages
 * swapped in/out and pages swapped in/out for a user-defineable time period. 
 * Only known to work with Linux v2.6. */

#ifndef __SWAP_MONITOR_H
#define __SWAP_MONITOR_H

#include <fstream>
#include "Scheduler.h"

/**
 * \file SwapMonitor.h
 * \brief declares SwapMonitor
 */

/**
 * \addtogroup Monitors
 * \brief A set of system resource monitors which can be used during execution of your application.
 * @{
 */

/// A class which monitors and logs the swap utilization of a program
class SwapMonitor : public TimerHandler
{
   public:
      SwapMonitor();
      void expire();
      static void runSwapMonitor(); ///< start the swap monitor 
      static void stopSwapMonitor(); ///< stop the swap monitor

      static const uint64_t DEFAULT_FREQUENCY = 5*1000*1000; ///< the default swap monitor frequency (in microseconds)

   private:
      void fillLast();

      static SwapMonitor* instance;
      bool halt;
      uint64_t frequency; /* microseconds */
      std::ifstream vmstat_file;
      uint32_t cumulative_in;
      uint32_t cumulative_out;
      uint32_t last_in;
      uint32_t last_out;
      double   peak_in;
      double   peak_out;
};

/** @} */

#endif
