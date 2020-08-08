/* 
 * StatisticalLimitFilter.cc : part of the Mace toolkit for building distributed systems
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
#include "StatisticalLimitFilter.h"

using std::list;

StatisticalLimitFilter::StatisticalLimitFilter(int limit) 
  : total(0), total_square(0),  standard_deviation(0), measurements(0), 
    keep_limit(limit) {
}

StatisticalLimitFilter::~StatisticalLimitFilter() {
}

void StatisticalLimitFilter::update(double incoming_value) {
  if (contents.size() > keep_limit) {
    contents.pop_front();
  }
  contents.push_back(incoming_value);
  measurements = contents.size();
  total = 0;
  total_square = 0;
  for(list<double>::iterator it = contents.begin(); 
      it != contents.end(); ++it) {
    double incoming = *it;
    total += incoming;
    total_square += incoming * incoming;
  }
  standard_deviation = sqrt((total_square * measurements - (total * total)) /
			    (measurements * (measurements - 1)));
  value = total / measurements;
}

double StatisticalLimitFilter::getDeviation() {
  return standard_deviation;
}

int StatisticalLimitFilter::getCount() {
  return contents.size();
}

int StatisticalLimitFilter::reset() {
  measurements = 0;
  value = 0;
  total = 0;
  total_square = 0;
  standard_deviation = 0;
  contents.clear();
  return 0;
}
