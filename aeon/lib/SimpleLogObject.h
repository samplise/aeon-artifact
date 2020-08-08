/* 
 * SimpleLogObject.h : part of the Mace toolkit for building distributed systems
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
#ifndef __SIMPLE_LOG_OBJECT_H
#define __SIMPLE_LOG_OBJECT_H

#include "BinaryLogObject.h"
#include "Log.h"
#include "Printable.h"

namespace mace {

template<typename T>
class SimpleLogObject : public BinaryLogObject, public PrintPrintable {

protected:
  static LogNode* rootLogNode;
  const std::string& logType;
  const T& obj;
  
public:
  SimpleLogObject(const std::string& desc, const T& _obj) : logType(desc),
							    obj(_obj) {
  }
  
  const std::string& getLogType() const {
    return logType;
  }
  
  const std::string& getLogDescription() const {
    static const std::string desc = "Return Type";
    return desc;
  }
  
  LogClass getLogClass() const {
    return STRUCTURE;
  }
  
  void serialize(std::string& __str) const {
    // not meant to be serialized
  }
  
  int deserialize(std::istream& is) throw(mace::SerializationException) {
    // not meant to be deserialized
    return 0;
  }
  
  void print(std::ostream& __out) const {
    __out << "Return value: ";
    printItem(__out, &obj);
  }
  
  void sqlize(mace::LogNode* node) const {
    mace::sqlize(&obj, node);
  }
  
  LogNode*& getChildRoot() const { 
    return rootLogNode;
  }
};

template<typename T> T & logVal(T & a, log_id_t out, const std::string& logType) {
  Log::binaryLog(out, mace::SimpleLogObject<T>(logType, a));
  return a;
}

  
template<typename T> T const & logVal(T const & a, log_id_t out, const std::string& logType) {
  Log::binaryLog(out, mace::SimpleLogObject<T>(logType, a));
  return a;
}

}
#endif // __SIMPLE_LOG_OBJECT_H
