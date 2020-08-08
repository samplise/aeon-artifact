/* 
 * SampleSignificanceRateControl.h : part of the Mace toolkit for building distributed systems
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
#ifndef _SAMPLE_SIGNIFICANCE_RATE_CONTROL_H
#define _SAMPLE_SIGNIFICANCE_RATE_CONTROL_H

#include "mdeque.h"
#include "RateControl.h"

class SampleSignificanceRateControl : public virtual RateControl {
public:
  SampleSignificanceRateControl(double alpha, double beta,
				double ea, double eb, double k);
  virtual ~SampleSignificanceRateControl();
  bool update(double data);
  double getTarget();
  void update(uint64_t id, double r, bool full, uint64_t qsz, uint64_t& maxq);

protected:
  class SIGNIFICANCE_RESULT {
  public:
    typedef int type;
    static const type BAD = -1;
    static const type INDETERMINATE = 0;
    static const type GOOD = 1;
  };

  double computeSignificance(unsigned int sample, unsigned int total) const;
  SIGNIFICANCE_RESULT::type cmpSignificance(double s) const;

protected:
  const double SIG_ALPHA;
  const double SIG_BETA;
  const double EMWA_ALPHA;
  const double EMWA_BETA;
  const double K;
//   const uint32_t N;
//   const double XI;

  typedef mace::deque<double> DoubleList;
  DoubleList samples;
  double rtt;
  double rttvar;
  double prevtarget;
  double target;
  uint32_t minSamples;
  uint32_t maxSamples;
};

#endif // _SAMPLE_SIGNIFICANCE_RATE_CONTROL_H
