/* 
 * SmoothedEMWARateControl.h : part of the Mace toolkit for building distributed systems
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
#ifndef _SMOOTHED_EMWA_RATECONTROL_H
#define _SMOOTHED_EMWA_RATECONTROL_H

#include "RateControl.h"

class SmoothedEMWARateControl : public virtual RateControl {
public:
  SmoothedEMWARateControl(double alpha = 1.0/8.0,
			  double beta = 1.0/4.0,
			  double k = 4) :
    skip(0), notFull(0),
    ALPHA(alpha), BETA(beta), K(k),
    rtt(0), rttvar(0), target(0), prevrtt(0), prevrttvar(0), prevtarget(0) {
  }

  virtual ~SmoothedEMWARateControl() { }

  bool update(double r) {
    double prevtarget = target;
    if (rtt == 0) {
      rtt = r;
//       rttvar = r / 2;
      rttvar = 0;
    }
    else {
      rttvar = (1 - BETA) * rttvar + BETA * fabs(rtt - r);
      rtt = (1 - ALPHA) * rtt + ALPHA * r;
    }
    target = rtt + K * rttvar;
    if (prevtarget == 0) {
      return true;
    }
    return r <= prevtarget;
  } // update

  void update(uint64_t id, double r, bool full, uint64_t qsz, uint64_t& max);
  
  double getTarget() {
    return target;
  }

protected:
  uint32_t skip;
  uint32_t notFull;
  double ALPHA;
  double BETA;
  double K;
  double rtt;
  double rttvar;
  double target;
  double prevrtt;
  double prevrttvar;
  double prevtarget;
};

#endif // _SMOOTHED_EMWA_RATECONTROL_H
