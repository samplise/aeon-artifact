/* 
 * mstring.h : part of the Mace toolkit for building distributed systems
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
#include "hash_string.h"
// #include <iostream>
// #include "Base64.h"
// #include "Serializable.h"

#ifndef _MACE_STRING_H
#define _MACE_STRING_H

/** 
 * \file mstring.h
 * \brief definition of mace::string
 */

namespace mace {
  
/**
 * \addtogroup Collections
 * @{
 */

  /// once a full type, now just a typedef for std::string, as no special collection functionality other than print/serialize was implemented.
  typedef std::string string;

/** @} */
}

// namespace mace {

// class string : public std::string, public Serializable {

// public:
//   string() : std::string() {
//   }

//   string(const char* s) : std::string(s) {
//   }

//   string(const char* s, size_type len) : std::string(s, len) {
//   }

//   string(const std::string& s) : std::string(s) {
//   }

//   virtual ~string() { }


//   int deserialize(std::istream& in) throw(SerializationException){
//     uint64_t sz;
//     char* buf;

//     mace::deserialize(in, &sz);
//     //     buf = new char[sz+1];
//     if(sz == 0) {
//       clear();
//       return sizeof(sz);
//     }
//     buf = new char[sz];
//     if(buf == NULL)
//       throw SerializationException("Could not allocate memory to deserialize");

//     in.read(buf, sz);
//     if(!in)
//       throw SerializationException("String not long enough to deserialize");
//     //     buf[sz] = '\0';
//     assign(buf, sz);
//     delete [] buf;
//     return sizeof(sz) + sz;
//   }

//   void deserializeStr(const std::string& s) throw(SerializationException) {
//     assign(s);
//   }

//   int deserializeXML_RPC(std::istream& in) throw(SerializationException) {
//     long offset = in.tellg();
//     char ch, ch1, ch2, ch3, ch4;
//     std::string tag;
//     bool dhack = false;

//     clear();
//     in >> std::skipws;

//     ch = in.peek();
//     if (ch != '<') {
//       // this is a legit string, just with no <string></string> tag
//       dhack = true;
//     }
//     else { // we may be looking at <string></string> or </value>
//       long dsp = in.tellg();
//       in >> ch >> ch1 >> ch2 >> ch3 >> ch4;
//       in.seekg(dsp);
//       if (ch1 == '/' && ch2 == 'v' && ch3 == 'a' && ch4 == 'l') {
// 	// empty string and without a <string></string> tag
// 	return (long)in.tellg() - offset;
//       }
//       else {
// 	dhack = false; // proceed as normal
//       }
//     }

//     if (!dhack) {
//       tag = SerializationUtil::getTag(in);
//       //     SerializationUtil::expect(in, "<string>");
//     }
//     if (dhack || tag == "<string>") {
//       in >> std::noskipws >> ch;
//       while(ch != '<') {
// 	switch(ch) {
// 	case '&':
// 	  in >> ch1 >> ch2 >> ch3;
// 	  //std:: cout << "got " << ch1 << ch2 << ch3;
// 	  if(!in)
// 	    throw SerializationException("String not long enough to deserialize");
// 	  if(ch2 == 't' && ch3 == ';') {
// 	    if(ch1 == 'l')
// 	      push_back('<');
// 	    else if(ch1 == 'g')
// 	      push_back('>');
// 	    else
// 	      throw SerializationException("Unrecognizable escape sequence while deserializing string");
// 	  }
// 	  else {
// 	    in >> ch4;
// 	    if(!in)
// 	      throw SerializationException("String not long enough to deserialize");
// 	    if(ch1 != 'a' || ch2 != 'm' || ch3 != 'p'|| ch4 != ';')
// 	      throw SerializationException("Unrecognizable escape sequence while deserializing string");
// 	    else
// 	      push_back('&');
// 	  }
// 	  break;
// 	default:
// 	  push_back(ch);
// 	}
// 	in >> ch;
// 	if(!in)
// 	  throw SerializationException("String not long enough to deserialize");
//       }
//       if (!dhack) {
// 	SerializationUtil::expect(in, "/string>");
// 	in >> std::skipws;
//       }
//       else {
// 	in.putback('<');
//       }
//     }
//     else if (tag == "<base64>") {
//       std::string buf;
//       std::istringstream* is = dynamic_cast<std::istringstream*>(&in);
//       if (is) {
// 	std::string ibuf = is->str();
// 	long ioff = is->tellg();
// 	size_t i = ibuf.find('<', ioff);
// 	if (i == std::string::npos) {
// 	  throw SerializationException("String not long enough to deserialize");
// 	}
// 	buf = ibuf.substr(ioff, i - ioff);
// 	in.seekg(i + 1, std::ios::beg);
//       }
//       else {
// 	in >> ch;
// 	while (ch != '<') {
// 	  buf.push_back(ch);
// 	  in >> ch;
// 	  if (!in) {
// 	    throw SerializationException("String not long enough to deserialize");
// 	  }
// 	}
//       }
//       SerializationUtil::expect(in, "/base64>");
//       append(Base64::decode(buf));
//     }
//     else {
//       throw SerializationException("Bad tag for string: " + tag);
//     }
//     return (long)in.tellg() - offset;
//   } // deserializeXML_RPC

//   void serialize(std::string &str) const {
//     uint64_t sz = size();
//     mace::serialize(str, &sz);
//     if(sz > 0) {
//       str.append(data(), sz);    
//     }
//   }

//   std::string serializeStr() const {
//     return *this;
//   }
  
//   void serializeXML_RPC(std::string& str) const throw(SerializationException) {
//     if (Base64::isPrintable(*this)) {
//       str.append("<string>");
//       for(unsigned x=0; x<size(); x++) {
// 	char c = operator[](x);
// 	switch(c) {
// 	case '<':
// 	  str.append("&lt;");
// 	  break;
// 	case '>':
// 	  str.append("&gt;");
// 	  break;
// 	case '&':
// 	  str.append("&amp;");
// 	  break;
// 	default:
// 	  str.push_back(c);
// 	}
//       }
//       str.append("</string>");
//     }
//     else {
//       str.append("<base64>");
//       str.append(Base64::encode(*this));
//       str.append("</base64>");
//     }
//   } // serializeXML_RPC
// };

// } // namespace mace

#ifdef __MACE_USE_UNORDERED__

// namespace std {
// namespace tr1 {
// 
// template<> struct hash<mace::string>  {
//   size_t operator()(const mace::string& x) const {
//     static const hash_string h = hash_string();
//     return h(x);
//   }
// };
// 
// } 
// }

#else

namespace __gnu_cxx {

template<> struct hash<mace::string>  {
  size_t operator()(const mace::string& x) const {
    static const hash_string h = hash_string();
    return h(x);
  }
};

} // namespace __gnu_cxx

#endif

#endif // _MACE_STRING_H
