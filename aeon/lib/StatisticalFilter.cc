/* 
 * StatisticalFilter.cc : part of the Mace toolkit for building distributed systems
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
#include <math.h>
#include "StatisticalFilter.h"
#include "massert.h"

using std::set;
using std::less;

StatisticalFilter::StatisticalFilter(int history) : total(0), total_square(0),
						    standard_deviation(0),
						    measurements(0),
						    keep_history(history) {
}

StatisticalFilter::~StatisticalFilter() {
}

void StatisticalFilter::update(double incoming) {
  total += incoming;
  total_square += incoming * incoming;
  measurements++;
  standard_deviation = sqrt((total_square * measurements - (total * total)) /
			    (measurements * (measurements - 1)));
  
  if (keep_history) {
    contents.insert(incoming);
  }
}

double StatisticalFilter::getDeviation() const {
  return standard_deviation;
}

double StatisticalFilter::percentileToValue(double percentile) const {

  if (percentile < 0 || percentile > 1.0 || !keep_history || 
      contents.size() == 0) {
    return -1;
  }

  int which = (int)((double)(contents.size() - 1) * percentile);
  std::multiset<double, less<double> >::const_iterator element;
  int counter = 0;

  for(element = contents.begin(); element != contents.end(); element++) {
    if (counter == which) {
      return *element;
    }
    counter++;
  }
  //somehow we got here, return the largest value
  return *contents.rbegin();
}

double StatisticalFilter::valueToPercentile(double value) const {
   
  if (!keep_history || contents.size() == 0) {
    return -1;
  }

  std::multiset<double, less<double> >::const_iterator element;
  int counter = 0;

  if (value >= *contents.rbegin()) {
    // shortcut if value is larger than everything in set
    return 1;
  }

  for(element = contents.begin(); element != contents.end(); element++) {
    if (*element > value) {
      return (double)counter / contents.size();
    }
    counter++;
  }
  // unreachable
  ASSERT(0);
}

double StatisticalFilter::getMin() const {
  if (!keep_history || contents.size() == 0) {
    return -1;
  }
  return *contents.begin();
}

double StatisticalFilter::getMax() const {
  if (!keep_history || contents.size() == 0) {
    return -1;
  }
  return *contents.rbegin();
}

int StatisticalFilter::getCount() const {
  return measurements;
}

double StatisticalFilter::getAvg() const {
  return ( measurements ? total / measurements : 0 ); // returns 0 when there are no measurements
}

int StatisticalFilter::reset(){
  measurements = 0;
  total = 0;
  total_square = 0;
  standard_deviation = 0;
  contents.clear();
  return 0;
}
