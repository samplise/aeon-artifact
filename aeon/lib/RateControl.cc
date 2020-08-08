/* 
 * RateControl.cc : part of the Mace toolkit for building distributed systems
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
#include "maceConfig.h"
#include "RateControl.h"
#include "SmoothedEMWARateControl.h"
#ifdef HAVE_GSL
#include "SampleSignificanceRateControl.h"
#endif

#include "mace-macros.h"
#include "Log.h"
#include "TimeUtil.h"
#include "Printable.h"
#include "params.h"

using namespace std;

RateControl& RateControl::instance() {
  ADD_SELECTORS("RateControl::instance");
  string controlType = params::get<string>("RATE_CONTROL_ALGORITHM");
  const double EMWA_ALPHA = params::get("RATE_CONTROL_EMWA_ALPHA", 8.0);
  const double EMWA_BETA = params::get("RATE_CONTROL_EMWA_BETA", 4.0);
  const double EMWA_K = params::get("RATE_CONTROL_EMWA_K", 4.0);
  if (controlType == "SmoothedEMWA") {
// WC: gcc 4.4.x has a bug. use the following hack to make the compiler happy
#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
    SmoothedEMWARateControl* p= NULL;
    p = new SmoothedEMWARateControl(1.0 / EMWA_ALPHA, 1.0 / EMWA_BETA, EMWA_K);
    RateControl* x = reinterpret_cast<RateControl*>(p);
    return *x;
#else
    return *(new SmoothedEMWARateControl(1.0 / EMWA_ALPHA, 1.0 / EMWA_BETA, EMWA_K) );
#endif
  }
  #ifdef HAVE_GSL
  else if (controlType == "SampleSignificance") {
    const double SIG_ALPHA = params::get("RATE_CONTROL_SIG_ALPHA", 0.05);
    const double SIG_BETA = params::get("RATE_CONTROL_SIG_BETA", 0.2);
//     const uint32_t N = params::get("RATE_CONTROL_SIG_N", 1000);
    return *(new SampleSignificanceRateControl(SIG_ALPHA, SIG_BETA,
					       1.0 / EMWA_ALPHA,
					       1.0 / EMWA_BETA,
					       EMWA_K));
  }
  #endif
  else {
    maceerr << "unknown RateControl type: " << controlType << Log::endl;
    ABORT("unknown RateControl type");
  }
} // instance

