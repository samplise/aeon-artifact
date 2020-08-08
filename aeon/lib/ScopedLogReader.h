/* 
 * ScopedLogReader.h : part of the Mace toolkit for building distributed systems
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
#ifndef _SCOPED_LOG_READER_H
#define _SCOPED_LOG_READER_H

#include "BinaryLogObject.h"
#include "Log.h"

namespace ScopedLogReader_namespace {

  class ScopedLogReader : public mace::BinaryLogObject, public mace::PrintPrintable {
  public:
    static const std::string type;
    bool begin;
    bool matched;

    ScopedLogReader(bool b = true, bool d = true) : begin(b), matched(d)
    { }

  protected:
    static mace::LogNode* rootLogNode;
    
  public:
    void serialize(std::string& __str) const {
      __str += (char)(begin ? (matched ? '1' : '2') : (matched ? '0' : '3') );
    }
    
    int deserialize(std::istream& is) throw(mace::SerializationException) {
      char c;
      
      //       is >> c;
      c = is.get();
      if (c == '1') {
	begin = true;
        matched = true;
      }
      else if (c == '0') {
	begin = false;
        matched = true;
      }
      else if (c == '2') {
	begin = true;
        matched = false;
      }
      else if (c == '3') {
	begin = false;
        matched = false;
      }
      else {
	std::cerr << "Invalid ScopedLog token " << (int)c << " found" << std::endl;
	throw mace::SerializationException("Invalid ScopedLog token found");
      }
      return 1;
    }
    
    void print(std::ostream& __out) const {
      if (begin) {
        if (matched) {
          __out << "STARTING";
        } else {
          __out << "CALLED";
        }
      }
      else {
        if (matched) {
          __out << "ENDING";
        } else {
          __out << "COMPLETED";
        }
      }
    }
    
    void sqlize(mace::LogNode* node) const {
      node->nextIndex();
      
//       if (index == 0) {
// 	std::ostringstream out;
	
// 	out << "CREATE TABLE " << node->logName
// 	    << " (_id INT PRIMARY KEY, begin INT, matched INT);" 
// 	    << std::endl;
// 	Log::logSqlCreate(out.str(), node);
//       }
// //       std::ostringstream out;
// //       out << node->logId << "\t" << index << "\t" << begin << "\t" 
// // 	  << matched << std::endl;
// //       fwrite(out.str().c_str(), out.str().size(), 1, node->file);
//       fprintf(node->file, "%d\t%d\t%d\t%d\n", node->logId, index, begin, matched);
    }
    
    const std::string& getLogType() const {
      return type;
    }

    const std::string& getLogDescription() const {
      static const std::string desc = "ScopedLog";
      return desc;
    }

    LogClass getLogClass() const {
      return STRUCTURE;
    }
    
    mace::LogNode*& getChildRoot() const {
      return rootLogNode;
    }
  };
  
  void init();
}

#endif // _SCOPED_LOG_READER_H
