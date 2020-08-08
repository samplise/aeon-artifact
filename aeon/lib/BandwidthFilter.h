/* 
 * BandwidthFilter.h : part of the Mace toolkit for building distributed systems
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
#ifndef _BANDWIDTH_FILTER_H
#define _BANDWIDTH_FILTER_H

#include <stdint.h>
#include <list>

/**
 * \file BandwidthFilter.h
 * \brief declares the BandwidthFilter class
 */

/// stores a <size, time> pair for bandwidth estimation
class BandwidthPair {
public:
  BandwidthPair(int  size1,  uint64_t  when1):  size (size1),  when (when1) {}
  int  size; ///< The number of bytes
  uint64_t  when; ///< The timestamp associated with the bytes
};
 

/**
 * \addtogroup Filters
 * @{
 */
 
/**
 * \brief Another filter for estimating bandwidth, but without fixed-size packets.
 *
 * Unlike the AdolfoFilter, the BandwidthFilter does not assume fixed-size
 * packets.  Instead of the GenericFilter::update(double), it provides an
 * update(int), and leaves the other to do awkward things.  
 */
class BandwidthFilter {
  
public:
  static const uint64_t BANDWIDTH_WINDOW=2500000; ///< the window to estimate the bandwidth over in microseconds

  BandwidthFilter();
  ~BandwidthFilter( );
  void  update (int size ); ///< adds size bytes at the current time to the estimator
  void  clear (); ///< resets the filter to 0
  double getValue(); ///< computes and returns the bandwidth value in bits per second
  
protected:
  void check ();
  std::list <BandwidthPair> history; 
  int history_count;

private:
  double bandwidth; ///< average bandwith over the last BANDWIDTH_WINDOW seconds.
  uint64_t time_initial; ///< time the class was initialized.  Used to prevent ramp-up issues during the initial window.
};

/** @} */

#endif //_BANDWIDTH_FILTER_H
