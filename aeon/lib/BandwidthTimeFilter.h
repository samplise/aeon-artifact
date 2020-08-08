/* 
 * BandwidthTimeFilter.h : part of the Mace toolkit for building distributed systems
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
#ifndef _BANDWIDTH_TIME_FILTER
#define _BANDWIDTH_TIME_FILTER

#include <list>
#include <pthread.h>

/**
 * \file BandwidthTimeFilter.h
 * \brief declares the BandwidthTimeFilter class
 * \todo Ryan, can you document this?
 *
 * CK: I think the main thing here beyond the bandwidth filter is that the
 * filter can be active and inactive.  Bandwidth is only averaged over the
 * active time, not the inactive time.
 */

class BandwidthTriplet {
public:
  BandwidthTriplet(double start1, double stop1, int size1) 
    : size(size1), start(start1), stop(stop1) {}
  int size;
  double start, stop;
};
 
/**
 * \addtogroup Filters
 * @{
 */
 
/**
 * \brief bandwidth filter estimation with segments
 * \todo Ryan, can you document this?
 */
class BandwidthTimeFilter {

protected:
  std::list<BandwidthTriplet> history; 
  double window;
  double bandwidth;
  
public:
  static const double DEFAULT_BANDWIDTH_WINDOW; ///< the bandwidth estimation window, in seconds

  BandwidthTimeFilter(double window = DEFAULT_BANDWIDTH_WINDOW);
  virtual ~BandwidthTimeFilter();
  void startUpdate();
  void update(int size);
  void finishUpdate();
  void cancelEntry();
  void clear();
  double getValue();
  
protected:
  void check();
};

/** @} */

#endif // _BANDWIDTH_TIME_FILTER
