/* 
 * VMRSSLimit.cc : part of the Mace toolkit for building distributed systems
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
// #include <unistd.h>
// #include <stdio.h>
#include <sys/types.h>
#include <inttypes.h>

#include <boost/lexical_cast.hpp>

#include "VMRSSLimit.h"
#include "Log.h"
#include "massert.h"
#include "params.h"


using namespace std;

VMRSSLimit* VMRSSLimit::instance = 0;

void VMRSSLimit::runVMRSSLimit( double maxRSS_MB, void (*callbackFnc)(double) ) {
  // Start the CPU load and usage monitor
  if (instance == 0) {
    instance = new VMRSSLimit( maxRSS_MB, callbackFnc );
  }
  instance->cancel();
  instance->schedule(instance->frequency);
} // runVMRSSLimit

void VMRSSLimit::stopVMRSSLimit() {
  instance->halt = true;
} // stopVMRSSLimit

VMRSSLimit::VMRSSLimit( double m, void (*f)(double) ) : 
   halt(false), frequency(DEFAULT_FREQUENCY), maxRSS(m), callbackFnc(f)
{
  ADD_SELECTORS("VMRSSLimit");
  statfile.open("/proc/self/statm");
  if (!statfile.good()) {
    perror( "REPLAY error opening /proc/self/statm" );
    return;
  }

  if( params::containsKey(params::MACE_VMRSSLIMIT_FREQUENCY_MS))
  {
    frequency = (int)(params::get<int>(params::MACE_VMRSSLIMIT_FREQUENCY_MS) * 1000);
  }

  maceLog("REPLAY VM RSS monitoring frequency is %" PRIu64 
	    " usecs.", frequency );
} // VMRSSLimit

void VMRSSLimit::expire() {
  ADD_SELECTORS("VMRSSLimit");
  static string tmp;
  int rss_pages;

  statfile >> tmp >> rss_pages;
  statfile.seekg(0, ios::beg);

  double currentRSS = ((double) rss_pages) * 4096.0 / (1024.0 * 1024.0); // assumes 4kB pages

  maceLog("REPLAY VM RSS: %d pages  %.1f MB",
	    rss_pages, currentRSS );

  if( maxRSS > 0 && currentRSS > maxRSS )
  {
     fprintf( stderr, "VMRSSLimit:: REPLAY VM RSS: ERROR maximum RSS exceeded. %.1f current %.1f max\n",
           currentRSS, maxRSS );
     maceError("REPLAY VM RSS: ERROR maximum RSS exceeded. %.1f current %.1f max",
           currentRSS, maxRSS );

     if( callbackFnc != NULL )
     {
        maceLog("REPLAY VM RSS: entering callback function" );
        (*callbackFnc)(currentRSS);
     }

     fflush(NULL);
     ABORT("VMRSSLimit Killing Program");
     //      exit(0);
  }

  if (!halt) {
    schedule(frequency);
  }
} // expire

