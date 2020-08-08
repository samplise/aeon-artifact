/* 
 * StatisticalFilter.h : part of the Mace toolkit for building distributed systems
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
#include <set>

/**
 * \file StatisticalFilter.h
 * \brief Declares StatisticalFilter
 */

#ifndef _STATISTICAL_FILTER_H
#define _STATISTICAL_FILTER_H

/**
 * \addtogroup Filters
 * @{
 */
 
/**
 * \brief returns basic statistics about series of values updated.
 *
 * Can return percentiles or values, the standard deviation, and the average of the values.
 */
class StatisticalFilter {
	
public:
  /// Construct a new filter, and pass whether to keep history (to support percentiles)
  StatisticalFilter(int history = 1);
  ~StatisticalFilter();
  
  void update(double incoming);
  double getDeviation() const; ///< return the standard deviation
  int getCount() const; ///< return the number of samples
  double percentileToValue(double percentile) const; ///< return the value for a given percentile
  double valueToPercentile(double value) const; ///< return the percentile for a given value
  double getMin() const; ///< return the minimum value (or getPercentile(0.0))
  double getMax() const; ///< return the maximum value (or getPrecentile(1.0))
  double getAvg() const; ///< return the average value (or 0 if no values provided)
  int reset(); ///< reset the filter to 0

protected:
  double total;
  double total_square;
  double standard_deviation;
  int measurements; 
  int keep_history ;
  std::multiset<double, std::less<double> > contents;
};

/** @} */

#endif //_STATISTICAL_FILTER_H
