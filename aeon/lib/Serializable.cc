/* 
 * Serializable.cc : part of the Mace toolkit for building distributed systems
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
#include <sstream>
#include <iostream>

#include "Serializable.h"
#include "XML_RPCSerialization.h"
#include "Log.h"
#include "mstring.h"

namespace mace {

int nextLog = 1; // the events table gets id 0
int nextEvent = 0;

int nextLogId() {
  return nextLog++;
}

int nextEventId() {
  return nextEvent++;
}

int LogNode::simpleCreate(const std::string& type, 
			  const mace::LogNode* node) {
  int next = nextIndex();
  if (next == 0) {
    std::ostringstream out;
    
    out << "CREATE TABLE " << node->logName
	<< " (_id INT PRIMARY KEY, value " << type << ");" << std::endl;
    Log::logSqlCreate(out.str(), node);
  }
  return next;
}
// int getIndex(const std::string& tableName) {
//   return tableOffsetMap[tableName];
// }

int Serializable::deserializeXML_RPC(std::istream& in) throw(SerializationException) {
//     throw SerializationException("XML-RPC deserialization not supported");
  std::string s;
  int r = mace::deserializeXML_RPC(in, &s, s);
  istringstream is(s);
  deserialize(is);
  return r;
}

void Serializable::serializeXML_RPC(std::string& str) const
  throw(SerializationException) {
//     throw SerializationException("XML-RPC serialization not supported");
  std::string s;
  serialize(s);
  mace::serializeXML_RPC(str, &s, s);
}

} // namespace mace
