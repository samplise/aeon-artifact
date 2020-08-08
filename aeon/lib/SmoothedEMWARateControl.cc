/* 
 * SmoothedEMWARateControl.cc : part of the Mace toolkit for building distributed systems
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
#include "SmoothedEMWARateControl.h"

#include "Log.h"
#include "TimeUtil.h"
#include "Printable.h"
#include "params.h"

class SmoothedEMWARateControlBLO : public mace::BinaryLogObject, public mace::PrintPrintable {
public:
  double r;
  bool full;
  double prevrtt;
  double prevrttvar;
  double prevtarget;
  double rtt;
  double rttvar;
  double target;
  uint64_t queuesize;
  uint64_t maxqueuesize;
  std::string direction;
  uint64_t id;

  SmoothedEMWARateControlBLO() { }
  SmoothedEMWARateControlBLO(double _r, bool _full,
		 double _prevrtt, double _prevrttvar, double _prevtarget,
		 double _rtt, double _rttvar, double _target,
		 uint64_t _qs, uint64_t _mqs, const std::string& _dir,
		 uint64_t _id) :
    r(_r), full(_full),
    prevrtt(_prevrtt), prevrttvar(_prevrttvar), prevtarget(_prevtarget),
    rtt(_rtt), rttvar(_rttvar), target(_target),
    queuesize(_qs), maxqueuesize(_mqs), direction(_dir), id(_id) {
  }

protected:
  static mace::LogNode* rootLogNode;

public:
  void serialize(std::string& __str) const { }
  int deserialize(std::istream& is) throw(mace::SerializationException) {
    return 0;
  }

  void print(std::ostream& __out) const {
    __out << "maxq=" << maxqueuesize << " q=" << queuesize << " " << direction
	  << " r=" << r
	  << " f=" << full
	  << " prevrtt=" << prevrtt
	  << " prevrttvar=" << prevrttvar
	  << " prevtarget=" << prevtarget
	  << " rtt=" << rtt
	  << " rttvar=" << rttvar
	  << " target=" << target
	  << " id=" << id;
  }

  //   void sqlize(mace::LogNode* node) const { } //CK: commented out since inherited behavior should be same.

  const std::string& getLogType() const {
    static const std::string type = "RateControl";
    return type;
  }

  const std::string& getLogDescription() const {
    static const std::string desc = "RateControl";
    return desc;
  }
  
  LogClass getLogClass() const {
    return STRUCTURE;
  }
  
  mace::LogNode*& getChildRoot() const { 
    return rootLogNode;
  }
  
};

mace::LogNode* SmoothedEMWARateControlBLO::rootLogNode = 0;

void SmoothedEMWARateControl::update(uint64_t id, double r, bool full,
				     uint64_t qsz, uint64_t& maxq) {
  static const log_id_t lid = Log::getId("RateControl::update");
  static const bool LATENCY_BIAS = params::get("RATE_CONTROL_LATENCY_BIAS", false);
  static const bool DYNAMIC_ALPHA_BETA = params::get("RATE_CONTROL_DYNAMIC_AB", false);
  static const uint32_t MISSED_TARGET_DECREASE = params::get("RATE_CONTROL_DECREASE", 1);
  static const uint32_t DECAY = params::get("RATE_CONTROL_DECAY", 0);
  static const uint32_t DECAY_LENGTH = params::get("RATE_CONTROL_DECAY_LENGTH", 1);
  static const uint32_t MINQ = params::get("RATE_CONTROL_MINQ", 16);
  static bool loggedParams = false;
  if (!loggedParams) {
    loggedParams = true;
    Log::log("RateControl::params")
      << "RATE_CONTROL_LATENCY_BIAS=" << LATENCY_BIAS
      << " RATE_CONTROL_DYNAMIC_AB=" << DYNAMIC_ALPHA_BETA
      << " RATE_CONTROL_DECREASE=" << MISSED_TARGET_DECREASE
      << " RATE_CONTROL_DECAY=" << DECAY
      << " RATE_CONTROL_DECAY_LENGTH=" << DECAY_LENGTH
      << " RATE_CONTROL_MINQ=" << MINQ
      << " ALPHA=" << ALPHA
      << " BETA=" << BETA
      << " K=" << K
      << Log::endl;
  }
  
  prevrtt = rtt;
  prevrttvar = rttvar;
  double tprevtarget = prevtarget;
  if (LATENCY_BIAS) {
    prevtarget = target;
  }
  update(r);
  std::string dir = "0";

  if (LATENCY_BIAS) {
    if (maxq == 1 + qsz) {
      if (prevtarget < target) {
	maxq--;
	dir = "-";
      } 
      else {
	maxq++;
	dir = "+";
      }
    }
  }
  else {
    if (skip == 0) {
      if (full) {
	if (rtt < tprevtarget) {
	  maxq++;
	  dir = "+";
	} 
	else {
 	  maxq -= MISSED_TARGET_DECREASE;
	  dir = "-";
	}
	skip = qsz;
	notFull = 0;
	prevtarget = target;
      }
      else if (qsz > MINQ) {
	notFull++;
	if (notFull * DECAY_LENGTH > qsz) {
	  maxq -= DECAY;
	  notFull = 0;
	}
      }
      if (DYNAMIC_ALPHA_BETA && maxq > 16) {
	ALPHA = 2.0 / maxq;
	BETA = 4.0 / maxq;
      }
    }
    if (skip > 0) {
      skip--;
    }
  }
  
  Log::binaryLog(lid, new SmoothedEMWARateControlBLO(
		   r, full, prevrtt, prevrttvar, prevtarget,
		   rtt, rttvar, target, qsz, maxq, dir, id));
}

