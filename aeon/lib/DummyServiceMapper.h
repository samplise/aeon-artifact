/* 
 * DummyServiceMapper.h : part of the Mace toolkit for building distributed systems
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
#ifndef _DUMMY_SERVICE_MAPPER_H
#define _DUMMY_SERVICE_MAPPER_H

#include "m_map.h"
#include "BinaryLogObject.h"
#include "ScopedLogReader.h"
#include "ScopedStackExecution.h"
#include "Accumulator.h"
#include "StringLogReader.h"
#include "ParamReader.h"
#include "MethodMap.h"

/** 
 * \file DummyServiceMapper.h
 * \brief defines the DummyServiceMapper class
 */


namespace mace {
  /** 
   * \brief Used for binary logging, this class allows associations between a
   * log type (string) and an object which can deserialize the string.
   */
  typedef BinaryLogObject* (*BinaryLogFactory)();
  
  class DummyServiceMapper{
  private:
    bool doInit() {
      ScopedLogReader_namespace::init();
      SSEReader::init();
      AccumulatorLogObject::init();
      StringLogReader_namespace::init();
      ParamReader_namespace::init();
      MethodMap_namespace::init();
      return true;
    }
    
  public:
   static map<std::string, BinaryLogFactory, SoftState>& mapper() {
	static map<std::string, BinaryLogFactory, SoftState>* m = new map<std::string, BinaryLogFactory, SoftState>();
	return *m;
   } //; ///< stores the set of binary log objects
   void addFactory(const std::string& type, BinaryLogFactory fac) { 
     mapper()[type] = fac; 
   } ///< adds a mapping between the type (key) and the object (obj)
   /// Returns an object pointer for a given key 
   BinaryLogFactory getFactory(const std::string& key) {
     static bool inited __attribute__((unused)) = doInit();
     return mapper()[key]; 
    }
  };
}

#endif // _DUMMY_SERVICE_MAPPER_H
