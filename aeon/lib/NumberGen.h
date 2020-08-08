/* 
 * NumberGen.h : part of the Mace toolkit for building distributed systems
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
#ifndef _NUMBER_GEN_H
#define _NUMBER_GEN_H

#include <map>
#include "Log.h"
#include "LogIdSet.h"
#include "params.h"
#include "mace-macros.h"
#include "mstring.h"
#include "SimulatorBasics.h"

/**
 * \file NumberGen.h 
 * \brief implements the NumberGen class
 */

/**
 * \brief The NumberGen class provides a key based map for generating incremental values.
 *
 * Common usage:
 * \code
 * registration_uid_t rid1 = NumberGen::Instance("registration_uid_t")->GetVal();
 * registration_uid_t rid2 = NumberGen::Instance("registration_uid_t")->GetVal();
 * registration_uid_t rid3 = NumberGen::Instance("registration_uid_t")->GetVal();
 * registration_uid_t rid4 = NumberGen::Instance("registration_uid_t")->GetVal();
 * \endcode
 *
 * Each call to GetVal returns a new value, incremented from the last call.
 * Each index string is a different series.  If the index string is also a
 * parameter set, it is used as the initial value.  Otherwise, DEFAULT_BASE is
 * used.
 *
 * When simulating, the node number is appended to the index, to ensure each
 * node gets a different series.  
 *
 * NumberGen is used for registration uids and port offsets.
 */
class NumberGen {
private:
  static const int DEFAULT_BASE = 0; ///< The default starting point for number generation.
//   static const int DEFAULT_PORT_BASE = 5377;

  unsigned val;
  typedef std::map<mace::string, NumberGen*> NumberGenMap;
  static NumberGenMap instances;

  NumberGen(int base) {
    val = base;
  }

public:
  static const mace::string HANDLER_UID;
  static const mace::string SERVICE_INSTANCE_UID;
  static const mace::string PORT;
  static const mace::string TEST_ID;

  /// Use only in the modelchecker/simulator.  Clears all number gen values.
  static void Reset() { 
    for(NumberGenMap::iterator i = instances.begin(); i != instances.end(); i++) {
      delete i->second;
    } 
    instances.clear();
  }

  /// clears a specific number gen value.  Used only in the modelchecker/simulator.
  static void Reset(const mace::string& index) { 
    NumberGenMap::iterator i = instances.find(index);
    if (i != instances.end()) {
      delete i->second;
      instances.erase(i);
    }
  }
  
  /// returns the number generator for the given \c index
  static NumberGen* Instance(mace::string index) {
    ADD_FUNC_SELECTORS;
    if(macesim::SimulatorFlags::simulated()) {
      index += macesim::SimulatorFlags::getCurrentNodeString();
    }
    if(instances.find(index) == instances.end()) {
      int base = DEFAULT_BASE;
      if(params::containsKey(index)) {
        base = params::get<int>(index);
        maceLog("Using params base for index %s %d", index.c_str(), base);
      }
      else {
        maceLog("Using default base for index %s %d", index.c_str(), DEFAULT_BASE);
      }
      instances[index] = new NumberGen(base);
    }
    return instances[index];
  }

  /// returns the next value in the series.
  unsigned GetVal() {
    return val++;
  }
};

#endif // _NUMBER_GEN_H
