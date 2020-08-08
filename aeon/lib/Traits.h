/* 
 * Traits.h : part of the Mace toolkit for building distributed systems
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
#include <string>
#include <istream>
#include <iostream>
#include "Serializable.h"
#include "mstring.h"
#include "boost/lexical_cast.hpp"

#ifndef _MACE_TRAITS_H
#define _MACE_TRAITS_H

/**
 * \file Traits.h
 * \brief define the Traits classes, to tell properties about types
 */

namespace mace {

//   class string;
  class MaceTime;

/// Basic class with types and no special properties
template<class T>
class KeyTraits {
  public:
  /// Whether or not the type can be represented as a string
  static bool isString() {
    return false;
  }
  /// Wether or not the type is an integer
  static bool isInt() {
    return false;
  }
  /// Whether or not the type is deterministic (in the simulator, MaceTime is non-deterministic, so ordering is random
  static bool isDeterministic() {
    return true;
  }
  /// Appends value \c key onto string \c str
  static void append(std::string& str, const T& key) {
  }
  /// For XML_RPC, read from \c in an element of type \c T, returning it.
  static T extract(std::istream& in) throw(SerializationException) {
    throw SerializationException("Cannot extract unsupported type");
  }
  // /// deserialize \c k from string \c s
  // static void set(T& k, const std::string& s) {
  //   deserialize(s, &k, k);
  // }
  // /// serialize \c key and return the string.
  // static std::string toString(const T& key) {
  //   return serialize(&key, key);
  // }
};

template<>
class KeyTraits<std::string> {
 public:
  static bool isString() {
    return true;
  }
  static bool isInt() {
    return false;
  }
  static bool isDeterministic() {
    return true;
  }
  static void append(std::string& str, const std::string& key) {
    str.append(key);
  }
  static std::string extract(std::istream& in) throw(SerializationException) {
    return SerializationUtil::get(in, '<');
  }
};

// template<>
// class KeyTraits<mace::string> : public KeyTraits<std::string> {
// };

template<>
class KeyTraits<long long> {
public:
  static bool isString() {
    return false;
  }
  static bool isInt() {
    return true;
  }
  static bool isDeterministic() {
    return true;
  }
  static void append(std::string& str, const long long& key) {
    char buf[30];
    
    sprintf(buf, "%lld", key);
    str += buf;
  }
  static unsigned long long extract(std::istream& in) 
    throw(SerializationException) {
    std::string token = SerializationUtil::get(in, '<');
    return boost::lexical_cast<unsigned long long>(token);
  }
};

template<>
class KeyTraits<unsigned long long> : public KeyTraits<long long> {
public:
  static void append(std::string& str, const unsigned long long& key) {
    char buf[30];
    
    sprintf(buf, "%llu", key);
    str += buf;
  }
};

template<>
class KeyTraits<int> : public KeyTraits<long long> {
};

template<>
class KeyTraits<unsigned> : public KeyTraits<unsigned long long> {
};

template<>
class KeyTraits<short> : public KeyTraits<long long> {
};

template<>
class KeyTraits<unsigned short> : public KeyTraits<unsigned long long> {
};

template<>
class KeyTraits<char> : public KeyTraits<long long> {
};

template<>
class KeyTraits<unsigned char> : public KeyTraits<unsigned long long> {
};


} // namespace mace
#endif // _TRAITS_H
