/* 
 * StartEventReader.h : part of the Mace toolkit for building distributed systems
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
#ifndef _START_EVENT_READER_H
#define _START_EVENT_READER_H

#include <boost/format.hpp>
#include "DummyServiceMapper.h"
#include "BinaryLogObject.h"

namespace StartEventReader_namespace {

  class StartEventReader : public mace::BinaryLogObject, public mace::PrintPrintable {
  protected:
    static mace::LogNode* rootLogNode;
    
  public:
    int node;
    uint64_t nodeTime;
    bool begin;
    static const std::string& type();
    
  public:
    
    StartEventReader(int n = 0, uint64_t time = 0, bool b = 0) : node(n), nodeTime(time), begin(b) {
      
    }
    
    void serialize(std::string& __str) const {
      mace::serialize(__str, &node);
      mace::serialize(__str, &nodeTime);
      mace::serialize(__str, &begin);
    }
    
    int deserialize(std::istream& is) throw(mace::SerializationException) {
      int bytes = 0;
      
      bytes += mace::deserialize(is, &node);
      bytes += mace::deserialize(is, &nodeTime);
      bytes += mace::deserialize(is, &begin);
      
      return bytes;
    }
    
    void sqlize(mace::LogNode* __node) const {
      int index = __node->nextIndex();
      
      if (index == 0) {
	std::ostringstream out;
	out << "CREATE TABLE " << __node->logName
	    << " (_id INT PRIMARY KEY, node INT, nodeTime INT8, begin INT);" 
	    << std::endl;
	Log::logSqlCreate(out.str(), __node);
      }
      std::ostringstream out;
      out << __node->logId << "\t" << index << "\t" << node << "\t" 
	  << nodeTime << "\t" << begin << std::endl;
      ASSERT(fwrite(out.str().c_str(), out.str().size(), 1, __node->file) > 0);
    }
    
    void print(std::ostream& __out) const {
      double printedTime = ((double)nodeTime) / 1000000;
      __out << (begin ? "Starting" : "Ending") << " event on node " << node 
	    << " at time " << boost::format("%.6f") % printedTime;
    }
    
    const std::string& getLogType() const {
      return type();
    }

    const std::string& getLogDescription() const {
      static const std::string desc = "StartEvent";
      return desc;
    }

    LogClass getLogClass() const {
      return STRUCTURE;
    }
    
    mace::LogNode*& getChildRoot() const {
      return rootLogNode;
    }
  };
  
  void init() __attribute__((constructor));
}

#endif // _START_EVENT_READER_H
