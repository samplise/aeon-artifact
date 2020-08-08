/* 
 * XML_RPCSerialization.h : part of the Mace toolkit for building distributed systems
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
#ifndef __XML_RPC__SERIALIZABLE_H
#define __XML_RPC__SERIALIZABLE_H

#include "m_net.h"
#include <string>
#include <sstream>
#include <iostream>
#include <typeinfo>
#include <ctype.h>
#include "Exception.h"
#include "massert.h"
#include "Serializable.h"
#include "mstring.h"

/**
 * \file XML_RPCSerialization.h
 * \brief Defines XML_RPC methods for serialization
 */

using std::istream;
using std::istringstream;
// using std::cerr;
// using std::endl;

namespace mace {

/* template<typename Key> */
/* bool isXML_RPCMapKeyType(const Key& k) { */
/*   const std::type_info& t = typeid(k); */
/*   return (t == typeid(int) || t == typeid(unsigned) || */
/* 	  t == typeid(long) || t == typeid(long long) || */
/* 	  t == typeid(short) || t == typeid(unsigned long) || */
/* 	  t == typeid(unsigned long long) || t == typeid(unsigned short) || */
/* 	  t == typeid(char) || t == typeid(unsigned char) ||  */
/* 	  t == typeid(bool) || t == typeid(std::string) || */
/* 	  t == typeid(mace::string)); */
/* } */

/// void* method for serialization, serialize first into a string, then use string serialization
/**
 * Has both a pointer and reference so it can call serialize from the reference.
 */
template<typename S>
void serializeXML_RPC(std::string& str, const void* pitem, const S& item)
  throw(SerializationException) {
//   throw SerializationException("Cannot serialize unsupported type");
  string s;
  serialize(s, &item);
//   s.serialize(str);
  serializeXML_RPC(str, &s, s);
}

/// string serialization in a \c string tag, encoding entities, or base64 encoding for non-printable characters
template<typename S>
void serializeXML_RPC(std::string& str, const std::string* pitem, const S& item) throw(SerializationException) {
  if (Base64::isPrintable(item)) {
    str.append("<string>");
    for (size_t x = 0; x < item.size(); x++) {
      char c = item[x];
      switch(c) {
      case '<':
	str.append("&lt;");
	break;
      case '>':
	str.append("&gt;");
	break;
      case '&':
	str.append("&amp;");
	break;
      default:
	str.push_back(c);
      }
    }
    str.append("</string>");
  }
  else {
    str.append("<base64>");
    str.append(Base64::encode(item));
    str.append("</base64>");
  }
} // serializeXML_RPC

/// handle serialization for objects which can serialize themselves
template<typename S>
void serializeXML_RPC(std::string& str, const Serializable* pitem, const S& item) throw(SerializationException) {
  item.serializeXML_RPC(str);
}

/// 4 byte integer serialization using \c i4
template<typename S>
void serializeXML_RPC(std::string& str, const int* pitem, 
		      const S& item) {
  char buf[30];
  sprintf(buf, "%d", *pitem);

  str.append("<i4>");
  str.append(buf);
  str.append("</i4>");
}

/// boolean serialization using \c boolean tag
template<typename S>
void serializeXML_RPC(std::string& str, const bool* pitem, 
		      const S& item) {
  str.append("<boolean>");
  if(*pitem)
    str.append("1");
  else
    str.append("0");
  str.append("</boolean>");
}

/// double serialization using \c double tag
template<typename S>
void serializeXML_RPC(std::string& str, const double* pitem, 
		      const S& item) {
  char buf[30];
  snprintf(buf, 30, "%f", *pitem);
  str.append("<double>");
  str.append(buf);
  str.append("</double>");
}

/// void deserialization using string itermediate
template<typename S>
int deserializeXML_RPC(std::istream& in, void* pitem, S& item)
  throw(SerializationException) {
//   throw SerializationException("Cannot deserialize unsupported type");
  string s;
//   int r = s.deserialize(in);
  int r = deserializeXML_RPC(in, &s, s);
  istringstream is(s);
  deserialize(is, &item);
  return r;
}

/// deserialize objects which know how to deserialize themselves
template<typename S>
int deserializeXML_RPC(std::istream& in, Serializable* pitem, S& item) 
  throw(SerializationException) {
  return item.deserializeXML_RPC(in);
}

/// handle string deserialization (see serializeXML_RPC(std::istream&, const std::string*, const S&)
template<typename S>
int deserializeXML_RPC(std::istream& in, std::string* pitem, S& item) throw(SerializationException) {
  long offset = in.tellg();
  char ch, ch1, ch2, ch3, ch4;
  std::string tag;
  bool dhack = false;

  item.clear();
  in >> std::skipws;

  ch = in.peek();
  if (ch != '<') {
    // this is a legit string, just with no <string></string> tag
    dhack = true;
  }
  else { // we may be looking at <string></string> or </value>
    long dsp = in.tellg();
    in >> ch >> ch1 >> ch2 >> ch3 >> ch4;
    in.seekg(dsp);
    if (ch1 == '/' && ch2 == 'v' && ch3 == 'a' && ch4 == 'l') {
      // empty string and without a <string></string> tag
      return (long)in.tellg() - offset;
    }
    else {
      dhack = false; // proceed as normal
    }
  }

  if (!dhack) {
    tag = SerializationUtil::getTag(in);
    //     SerializationUtil::expect(in, "<string>");
  }
  if (dhack || tag == "<string>") {
    in >> std::noskipws >> ch;
    while (ch != '<') {
      switch (ch) {
      case '&':
	in >> ch1 >> ch2 >> ch3;
	//std:: cout << "got " << ch1 << ch2 << ch3;
	if (!in) {
	  throw SerializationException("String not long enough to deserialize");
	}
	if (ch2 == 't' && ch3 == ';') {
	  if (ch1 == 'l') {
	    item.push_back('<');
	  }
	  else if (ch1 == 'g') {
	    item.push_back('>');
	  }
	  else {
	    throw SerializationException("Unrecognizable escape sequence while deserializing string");
	  }
	}
	else {
	  in >> ch4;
	  if (!in) {
	    throw SerializationException("String not long enough to deserialize");
	  }
	  if (ch1 != 'a' || ch2 != 'm' || ch3 != 'p'|| ch4 != ';') {
	    throw SerializationException("Unrecognizable escape sequence while deserializing string");
	  }
	  else {
	    item.push_back('&');
	  }
	}
	break;
      default:
	item.push_back(ch);
      }
      in >> ch;
      if (!in) {
	throw SerializationException("String not long enough to deserialize");
      }
    }
    if (!dhack) {
      SerializationUtil::expect(in, "/string>");
      in >> std::skipws;
    }
    else {
      in.putback('<');
    }
  }
  else if (tag == "<base64>") {
    std::string buf;
    std::istringstream* is = dynamic_cast<std::istringstream*>(&in);
    if (is) {
      std::string ibuf = is->str();
      long ioff = is->tellg();
      size_t i = ibuf.find('<', ioff);
      if (i == std::string::npos) {
	throw SerializationException("String not long enough to deserialize");
      }
      buf = ibuf.substr(ioff, i - ioff);
      in.seekg(i + 1, std::ios::beg);
    }
    else {
      in >> ch;
      while (ch != '<') {
	buf.push_back(ch);
	in >> ch;
	if (!in) {
	  throw SerializationException("String not long enough to deserialize");
	}
      }
    }
    SerializationUtil::expect(in, "/base64>");
    item.append(Base64::decode(buf));
  }
  else {
    throw SerializationException("Bad tag for string: " + tag);
  }
  return (long)in.tellg() - offset;
} // deserializeXML_RPC

/// 4 byte integer deserialization using \c i4 tag or \c int tag
template<typename S>
int deserializeXML_RPC(std::istream& in, int* pitem, S& item) 
  throw(SerializationException) {
    std::istream::pos_type offset = in.tellg();
  std::string tag;

  tag = SerializationUtil::getTag(in);
  if(tag == "<i4>" || tag == "<int>") {
    SerializationUtil::getToken(in, *pitem);
    if(tag == "<i4>") {
      SerializationUtil::expectTag(in, "</i4>");
    }
    else {
      SerializationUtil::expectTag(in, "</int>");
    }
  }
  else
    throw SerializationException("error parsing integer tag: " + tag);
  return in.tellg() - offset;
}

/// boolean deserialization using \c boolean tag
template<typename S>
int deserializeXML_RPC(std::istream& in, bool* pitem, S& item) 
  throw(SerializationException) {
    std::istream::pos_type offset = in.tellg();
  char c;

  SerializationUtil::expectTag(in, "<boolean>");
  SerializationUtil::getToken(in, c);
  if(c == 1) {
    *pitem = true;
  }
  else if(c == 0) {
    *pitem = false;
  }
  else {
    throw SerializationException("Invalid value for boolean variable: " + c);
  }
  SerializationUtil::expectTag(in, "</boolean>");

  return in.tellg() - offset;
}

/// double deserialization using \c double tag
template<typename S>
int deserializeXML_RPC(std::istream& in, double* pitem, S& item)
  throw(SerializationException) {
    std::istream::pos_type offset = in.tellg();

  SerializationUtil::expectTag(in, "<double>");
  SerializationUtil::getToken(in, *pitem);
  SerializationUtil::expectTag(in, "</double>");

  return in.tellg() - offset;
}

/// deserialization for a parameter (\c param and \c value tags)
template<typename S>
int deserializeXML_RPCParam(std::istream& in, S* pitem, S& item) 
  throw(SerializationException) {
    std::istream::pos_type offset = in.tellg();

  SerializationUtil::expectTag(in, "<param>");
  SerializationUtil::expectTag(in, "<value>");
  mace::deserializeXML_RPC(in, pitem, item);
  SerializationUtil::expectTag(in, "</value>");
  SerializationUtil::expectTag(in, "</param>");
  
  return in.tellg() - offset;
}

/// serialize arbitrary object and return a string
template<typename S>
std::string serializeXML_RPC(const S* pitem, const S& item) 
  throw(SerializationException) {
    std::string s;
    mace::serializeXML_RPC(s, &item, item);
    return s;
}
//template<typename S>
//std::string serializeXML_RPC(const Serializable* pitem, const S& item) 
//  throw(SerializationException) {
//    std::string s;
//    item.serializeXML_RPC(s);
//    return s;
//}

/// deserialize an arbitrary object from a string rather than an istream
template<typename S>
int deserializeXML_RPCStr(const std::string& str, void* pitem, S& item) 
  throw(SerializationException) {
    std::istringstream in(str);
    return mace::deserializeXML_RPC(in, pitem, item);
}
} // namespace mace
#endif
