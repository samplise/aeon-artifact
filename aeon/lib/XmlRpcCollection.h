/* 
 * XmlRpcCollection.h : part of the Mace toolkit for building distributed systems
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
#ifndef _XMLRPCCOLLECTION_H
#define _XMLRPCCOLLECTION_H

#include <string>
#include "Traits.h"
#include "XML_RPCSerialization.h"

/**
 * \file XmlRpcCollection.h
 * \brief handles collection XML_RPC serialiation and deserialization 
 *
 * this file is largely unneeded anymore, since it is now handled by CollectionSerializers.h
 */

namespace mace {

/// generic map serialization
template<class T, class Key>
void XmlRpcMapSerialize(std::string& str, const T& begin, const T& end)
  throw(SerializationException) {
  str.append("<struct>");
  for(T i=begin; i!=end; i++) {
    str.append("<member><name>");
    KeyTraits<Key>::append(str, i->first);
    str.append("</name><value>");
    mace::serializeXML_RPC(str, &(i->second), i->second);
    str.append("</value></member>");
  }
  str.append("</struct>");
}

/// generic map deserialization
template<class T, class Key, class Data>
int XmlRpcMapDeserialize(std:: istream& in, T& obj) 
  throw(SerializationException) {
    std::istream::pos_type offset = in.tellg();

  obj.clear();
//   std::cout << "attempting to parse <struct>" << std::endl;
  SerializationUtil::expectTag(in, "<struct>");
  //cout << "got struct" << endl;
  std::string tag = SerializationUtil::getTag(in);
  while (tag == "<member>") {
    //cout << "got tag  " << tag << endl;
//     std::cout << "parsed tag " << tag << std::endl;
    Key k;
    Data d;
    //cout << "got member token " << endl;
//       std::cout << "attempting to parse <name> " << tag << std::endl;
    SerializationUtil::expectTag(in, "<name>");
    k = KeyTraits<Key>::extract(in);
    //cout <<"extracted key as " << k << endl;
    //mace::deserializeXML_RPC(in, &k, k);
    SerializationUtil::expectTag(in, "</name>");
    SerializationUtil::expectTag(in, "<value>");
    mace::deserializeXML_RPC(in, &d, d);
    SerializationUtil::expectTag(in, "</value>");
    SerializationUtil::expectTag(in, "</member>");
    obj[k] = d;

    tag = SerializationUtil::getTag(in);
//     std::cout << "parsed tag " << tag << std::endl;
  }

  if (tag != "</struct>") {
    throw SerializationException("error parsing tag: expected </struct>, parsed " + tag);
  }

//   in >> skipws;
  return in.tellg() - offset;
} // XmlRpcMapDeserialize

/// generic array serialization
template<class T>
void XmlRpcArraySerialize(std::string& str, const T& begin, const T& end)
  throw(SerializationException) {
  str.append("<array><data>");
  for(T i=begin; i!=end; i++) {
    str.append("<value>");
    mace::serializeXML_RPC(str, &(*i), *i);
    str.append("</value>");
  }
  str.append("</data></array>");
}

/// generic array deserialization
template<class T, class Key>
int XmlRpcArrayDeserialize(std::istream& in, T& obj) 
  throw(SerializationException) {
  long offset = in.tellg();

  obj.clear();
  SerializationUtil::expectTag(in, "<array>");
  SerializationUtil::expectTag(in, "<data>");
  std::string tag = SerializationUtil::getTag(in);
  while (tag == "<value>") {
    Key t;
    mace::deserializeXML_RPC(in, &t, t);
    obj.push_back(t);
    SerializationUtil::expectTag(in, "</value>");
    tag = SerializationUtil::getTag(in);
  }

  if (tag != "</data>") {
    throw SerializationException("error parsing tag: expected </data>, parsed " + tag);
  }

  SerializationUtil::expectTag(in, "</array>");
//   in >> skipws;
  return (long)in.tellg() - offset;
} // XmlRpcArrayDeserialize

} // namespace mace

#endif // _XMLRPCOLLECTION_H
