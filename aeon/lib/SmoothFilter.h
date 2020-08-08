/* 
 * SmoothFilter.h : part of the Mace toolkit for building distributed systems
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

/**
 * \file SmoothFilter.h
 * \brief Declares SmoothFilter
 */

#ifndef __SmoothFilter
#define __SmoothFilter
/**
 * \addtogroup Filters
 * @{
 */
 
/// Implements a simple weighted moving average based on a SmoothFilter
class SmoothFilter
{
  
public:
  /**
   * \brief creates a new SmoothFilter, setting the smooth_factor
   * @param smooth_factor the weight for the weighted moving average
   */
  SmoothFilter	( double smooth_factor = 0.9);
  ~SmoothFilter	( );
  /**
   * \brief update the average with incoming
   *
   * \f$ value = oldValue \times (smooth\_factor) + (incoming \times (1.0 - smooth\_factor)) \f$
   *  
   *  if the value is empty, just use the \c incoming value.
   *
   * @param incoming the new value
   */
  void  update (double incoming);
  double age (); ///< ages value without new value; equivalent to update(0.0); getValue();
  void reset(); ///< resets value to 0.0
public:
  double getValue() { return value; }
protected:
  double value;
  double smooth_factor;
  int samples;
private:
};
/** @} */
#endif //__SmoothFilter
