/* 
 * SwapMonitor.cc : part of the Mace toolkit for building distributed systems
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

/* Monitors system virtual memory swap paging statistics. Only known to work
 * with Linux v2.6 kernel. */

#include <inttypes.h>
#include "SwapMonitor.h"
#include "Log.h"
#include "params.h"

using std::string;

SwapMonitor* SwapMonitor::instance = 0;

void SwapMonitor::runSwapMonitor()
{
   // Start the swap monitor

   if( instance == 0 )
   {
      instance = new SwapMonitor();
   }

   // Reset the instance timer:
   instance->cancel();
   instance->schedule( instance->frequency );
}

void SwapMonitor::stopSwapMonitor()
{
   instance->halt = true;
}

inline void SwapMonitor::fillLast()
{
   /* Reads the vmstat file to fill in the last_in and last_out variables. */

   static string header;

   bool found_in = false;
   while( vmstat_file.good() && found_in == false )
   {
      vmstat_file >> header >> last_in;
      if( header == "pswpin" )
         found_in = true;
   }

   bool found_out = false;
   while( vmstat_file.good() && found_out == false )
   {
      vmstat_file >> header >> last_out;
      if( header == "pswpout" )
         found_out = true;
   }
   vmstat_file.seekg( 0, std::ios::beg );
}

SwapMonitor::SwapMonitor() : halt(false), frequency(DEFAULT_FREQUENCY),
                             cumulative_in(0), cumulative_out(0),
                             last_in(0), last_out(0),
                             peak_in(0), peak_out(0)
{
   vmstat_file.open( "/proc/vmstat" );
   if( ! vmstat_file.good() )
   {
      perror( "REPLAY error opening /proc/vmstat" );
      return;
   }

   if( params::containsKey( params::MACE_SWAP_MONITOR_FREQUENCY_MS ) )
   {
      frequency = (int)( params::get<int>( params::MACE_SWAP_MONITOR_FREQUENCY_MS ) * 1000 );
   }

   Log::logf( "SwapMonitor", "REPLAY Swap monitoring frequency is %" PRIu64
              " usecs.", frequency );


   fillLast();
   cumulative_in  = last_in;
   cumulative_out = last_out;
}


void SwapMonitor::expire()
{
   fillLast();

   /* Calculate per-second swap in/out rates. */
   double in_ps  = ((double) last_in  - cumulative_in ) / (frequency/(1000*1000));
   double out_ps = ((double) last_out - cumulative_out) / (frequency/(1000*1000));

   //    Log::logf( "SwapMonitor", "REPLAY Swap (in out in/s out/s peak_in peak_out) %d %d %.2f %.2f %.2f %.2f",
   //               last_in, last_out, in_ps, out_ps, peak_in, peak_out );
   Log::logf( "SwapMonitor", "(in out peakin peakout) %.2f %.2f %.2f %.2f",
              in_ps, out_ps, peak_in, peak_out );

   cumulative_in  = last_in;
   cumulative_out = last_out;

   /* Calculate peak per-second swap in/out rates. */
   if( peak_in < in_ps )
      peak_in = in_ps;
   if( peak_out < out_ps )
      peak_out = out_ps;

   if (!halt)
      schedule(frequency);
}
