/* 
 * mpair.h : part of the Mace toolkit for building distributed systems
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
#include "Serializable.h"
#include "XML_RPCSerialization.h"

/**
 * \file mpair.h
 * \brief defines the mace::pair extension std::pair
 */

#ifndef _MACE_PAIR_H
#define _MACE_PAIR_H

namespace mace {

/**
 * \addtogroup Collections
 * @{
 */

/// implements the printable version of std::pair.
template<class F, class S>
class printpair : public std::pair<F, S>, public PrintPrintable {
public:
  printpair() : std::pair<F, S>() {
  }

  printpair(const F& f, const S& s) : std::pair<F, S>(f, s) {
  }

  virtual ~printpair() { }

  void print(std::ostream& out) const {
    out << "(" << this->first << ", " << this->second << ")";
  }
  
  void printState(std::ostream& out) const {
    out << "(";
    mace::printState(out, &this->first, this->first);
    out << ", ";
    mace::printState(out, &this->second, this->second);
    out << ")";
  }

};

/// derives from mace::printpair, also adds serializability
template<class F, class S>
class pair : public mace::printpair<F, S>, public Serializable {
public:
  pair() : mace::printpair<F, S>() {
  }

  pair(const F& f, const S& s) : mace::printpair<F, S>(f, s) {
  }

  virtual ~pair() { }
  
  int deserialize(std::istream& in) throw(SerializationException) {
    int bytes = 0;

    bytes += mace::deserialize(in, &this->first);
    bytes += mace::deserialize(in, &this->second);

    return bytes;
  }
  
  int deserializeXML_RPC(std::istream& in) throw(SerializationException) {
    long offset = in.tellg();
    SerializationUtil::expectTag(in, "<array>");
    SerializationUtil::expectTag(in, "<data>");
    SerializationUtil::expectTag(in, "<value>");
    mace::deserializeXML_RPC(in, &this->first, this->first);
    SerializationUtil::expectTag(in, "</value>");
    SerializationUtil::expectTag(in, "<value>");
    mace::deserializeXML_RPC(in, &this->second, this->second);
    SerializationUtil::expectTag(in, "</value>");
    SerializationUtil::expectTag(in, "</data>");
    SerializationUtil::expectTag(in, "</array>");
    return (long)in.tellg() - offset;
  }

  void serialize(std::string &str) const {
    mace::serialize(str, &this->first);
    mace::serialize(str, &this->second);
  }

  void serializeXML_RPC(std::string& str) const throw(SerializationException) {
    str.append("<array><data><value>");
    mace::serializeXML_RPC(str, &this->first, this->first);
    str.append("</value><value>");
    mace::serializeXML_RPC(str, &this->second, this->second);
    str.append("</value></data></array>");
  }
};

/** @} */

}
#endif // _MACE_PAIR_H
