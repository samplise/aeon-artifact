/* 
 * VMRSSLimit.h : part of the Mace toolkit for building distributed systems
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
#ifndef __VMRSSLimit__H
#define __VMRSSLimit__H
#include <fstream>
#include <cassert>
#include "Scheduler.h"

/**
 * \file VMRSSLimit.h
 * \brief declares VMRSSLimit
 */

/**
 * \addtogroup Monitors
 * @{
 */

/**
 * \brief Virtual Memory Resident Set Size Limiter
 *
 * Kills your application if it uses too much virtual memory, after calling the (optional) callback function
 *
 * \warning Assumes a page size of 4KB. 
 * \todo JBurke, can you check these docs, and add an example of what callbackFnc should be?
 */
class VMRSSLimit : public TimerHandler {
public:
  VMRSSLimit( double maxRSS_MB, void (*callbackFnc)(double)  ); ///< \todo should this constructor be private?
  void expire();
  static void runVMRSSLimit( double maxRSS_MB, void (*callbackFnc)(double) ); ///< Start the CPU load and usage monitor with max in MB, optional callback function
  static void stopVMRSSLimit(); ///< stops the limiter
  static double getRSS() { return instance ? instance->currentRSS : 0; } ///< return the latest virtual memory size

public:
  static const uint64_t DEFAULT_FREQUENCY = 5*1000*1000; ///< default limiter frequency

private:
  static VMRSSLimit* instance;
  bool halt;
  uint64_t frequency;
  std::ifstream statfile;
  double currentRSS;
  double maxRSS;
  void (*callbackFnc)(double);
};

/** @} */

#endif
