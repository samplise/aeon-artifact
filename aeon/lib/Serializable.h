/* 
 * Serializable.h : part of the Mace toolkit for building distributed systems
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
#ifndef __SERIALIZABLE_H
#define __SERIALIZABLE_H

#include <inttypes.h>
#include "m_net.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <typeinfo>
#include <ctype.h>
#include <vector>
//#include <byteswap.h>
#include <boost/lexical_cast.hpp>
#include <boost/concept_check.hpp>
#include "Exception.h"
#include "massert.h"

/**
 * \file Serializable.h
 * \brief define Mace methods for serializing Mace types. 
 *
 * Includes methods for fixed-size types, and objects which known how to serialialize themselves (Serializable)
 */

static inline bool _DO_BYTE_SWAP() { return (1 != htonl(1)); } ///< true if the system requires byte swapping

#if 0
 #define htonll(t) \
 { \
   uint32_t low_h = t & 0xffffffff; \
   uint32_t low_n = htonl(low_h); \
   if (low_h == low_n) { t; } \
   else {uint32_t high_h = t >> 32; \
   uint32_t high_n = htonl(high_h); \
   uint64_t t_n = low_n; \
   t_n = (t_n << 32) + high_n; \
   } \
 }
#endif

/// Defines 8 byte swapping method (host to network)
inline uint64_t htonll(const uint64_t& t) {
  if (!_DO_BYTE_SWAP()) {
    return t;
  }
  uint32_t low_h = t & 0xffffffff;
  uint32_t low_n = htonl(low_h);
  uint32_t high_h = t >> 32;
  uint32_t high_n = htonl(high_h);
  uint64_t t_n = low_n;
  t_n = (t_n << 32) + high_n;
  return t_n;
}

#if 0
#define ntohll(t) \
 { \
   uint32_t low_n = t & 0xffffffff; \
   uint32_t low_h = ntohl(low_n); \
   if (low_h == low_n) { t; } \
   else {uint32_t high_n = t >> 32; \
   uint32_t high_h = ntohl(high_n); \
   uint64_t t_h = low_h; \
   t_h = (t_h << 32) + high_h; \
   } \
 }
#endif

/// Defines 8 byte swapping method (network to host)
inline uint64_t ntohll(const uint64_t& t) {
  if (!_DO_BYTE_SWAP()) {
    return t;
  }
  uint32_t low_n = t & 0xffffffff;
  uint32_t low_h = ntohl(low_n);
  uint32_t high_n = t >> 32;
  uint32_t high_h = ntohl(high_n);
  uint64_t t_h = low_h;
  t_h = (t_h << 32) + high_h;
  return t_h;
}


using std::istream;
using std::istringstream;
using std::skipws;
// using std::cerr;
// using std::endl;

namespace mace {

// extern int nextLog;

int nextLogId();
int nextEventId();

class LogNode {
public:
  LogNode** children;
  FILE* file;
  int logId;
  std::string logName;
  int nextId;
  
  LogNode(const std::string& lName, FILE* logFile) {
    logId = nextLogId();
    file = logFile;
    nextId = 0;
    logName = lName;
    children = NULL;
  }
  
  int nextIndex() {
    return nextId++;
  }
  
  int simpleCreate(const std::string& type, const mace::LogNode* node);
};

// int getIndex();

// class string;

  /// Base class for boost concept checking.  Template parameter must derive from Serializer.
  class Serializer { };

  /// indicates that this serializer does nothing, a type deriving from it will not be Serializable
  class SoftState : public Serializer {};

/// exception for errors during serialization or deserialization
class SerializationException : public Exception {
public:
  SerializationException(const std::string& m) : Exception(m) { }
  virtual ~SerializationException() throw() {}
  virtual void rethrow() const { throw *this; }
};


/**
 * \addtogroup Utils
 * @{
 */

/// Utility for helping with Xml serialization
class SerializationUtil {
public:
  /// reads the next tag from the input stream (with delimiters), requires an open bracket first
  static std::string getTag(istream& in, bool strict = true)
    throw(SerializationException) {
    std::string ret("<");
    char ch;

    expect(in, '<');
    in.get(ch);
    while(in && ch != '>' && (!strict || (isalnum(ch) || ch == '/' || ch == '_'))) {
//       cerr << "got " << ch << endl;
      ret.push_back(ch);
      in.get(ch);
    }
    if(!in) {
      throw SerializationException("EOF while deserializing tag: " + ret);
    }
    if (ch != '>') {
      std::string err = "Invalid character in tag " + ret + ": ";
      err.push_back(ch);
      throw SerializationException(err);
    }
    ret.push_back('>');

//     cerr << "returning " << ret << endl;
    return ret;
  }

  /// reads in whitespace, then calls expect() with the tag
  static void expectTag(istream& in, const char* tag) 
    throw(SerializationException) {

    while (in && isspace(in.peek())) {
      in.get();
    }

    expect(in, tag);
  }

  /// reads in bytes untli the delimiter is found, returning as a string
  static std::string get(istream& in, char delim) throw(SerializationException) {
    std::string ret = "";
    char ch;

    while(in) {
      in.get(ch);
      if(ch != delim)
	ret += ch;
      else {
	in.unget();
	return ret;
      }
    }
    throw SerializationException(std::string("EOF while looking for delimeter ") + delim);
  }

  /// throws an exception of \c str is not next in \c in
  static void expect(istream& in, const char* str) 
    throw(SerializationException) {
    unsigned len = strlen(str);
//     std::cerr << "expect: " << str << " " << len << " " << in.tellg() << std::endl;
//     char* buf = new char[len + 1];
    char* buf = new char[len];
    in.read(buf, len);
//     buf[len] = 0;
//     std::cerr << buf << " " << in.tellg() << std::endl;
    if(!in || strncmp(buf, str, len) != 0) {
      delete [] buf;
      std::string errstr = "error parsing string: ";
      if (!in) {
	errstr += "could not read " + boost::lexical_cast<std::string>(len) +
	  " bytes, expecting ";
	errstr.append(str);
      }
      else {
	errstr.append(str);
	errstr += " != ";
	errstr.append(buf, len);
      }
      throw SerializationException(errstr);
    }
    delete [] buf;
  }

  /// equivalent to expect(istream&, const char*)
  static void expect(istream& in, const std::string& str) 
    throw(SerializationException) {
    expect(in, str.c_str());
  }

  /// reads from \c in using >>, expects the value to be \c val
  template<typename S>
  static void expect(istream& in, const S& val) throw(SerializationException) {
    S v;

    in >> v;
    if (!in || v != val) {
      std::string errstr = "error parsing token " +
	boost::lexical_cast<std::string>(val) + ": ";
      if (!in) {
	errstr += "EOF";
      }
      else {
	errstr += "parsed " + boost::lexical_cast<std::string>(v);
      }
	
      throw SerializationException(errstr);
    }
  }

  /// reads in a whitespace-tokenized token
  template<typename S>
  static void getToken(istream& in, S& val) throw(SerializationException) {
    in >> val;
    if (!in) {
      throw SerializationException("error parsing token " +
				   boost::lexical_cast<std::string>(val));
    }
  }

}; // SerializationUtil

/** @} */

  /// Objects which know how to serialize themselves
class Serializable : public Serializer {
public:
  /// Replace the current object value from the bytes read from \c in, returning the number of bytes read
  virtual int deserialize(std::istream& in) throw(SerializationException) = 0;
  /// Encode the current object at the end of \c str as a bytestring in a compact format (and which doesn't require delimiters to parse)
  virtual void serialize(std::string& str) const = 0;
  virtual void sqlize(LogNode* node) const {
    
  }
  /// Encode the current object as a bytestring in a compact, non-delimited format, and return it
//   virtual void serializeStr(std::string& str) const {
//     serialize(str);
//   }
  virtual std::string serializeStr() const {
    std::string s;
    serialize(s);
    return s;
  }
  /// Replace the current object value from the serialized object in \c s.  
  virtual void deserializeStr(const std::string& s) throw(SerializationException) {
    std::istringstream is(s);
    deserialize(is);
  }

  /// deserialize this object from XML_RPC format from \c in
  virtual int deserializeXML_RPC(std::istream& in) throw(SerializationException);
  /// serialize this object onto the end of \c str in XML_RPC format
  virtual void serializeXML_RPC(std::string& str) const throw(SerializationException);

  virtual ~Serializable() {}
};

/// returns a Serializable pointer if the object is Serializable, NULL otherwise.  (uses static typing only)
inline Serializable* getSerializable(void* pitem) {
  return NULL;
}
/// returns a Serializable pointer if the object is Serializable, NULL otherwise.  (uses static typing only)
inline Serializable* getSerializable(Serializable* pitem) {
  return pitem;
}
/// returns a Serializable const pointer if the object is Serializable, NULL otherwise.  (uses static typing only)
inline const Serializable* getSerializable(const void* pitem) {
  return NULL;
}
/// returns a Serializable const pointer if the object is Serializable, NULL otherwise.  (uses static typing only)
inline const Serializable* getSerializable(const Serializable* pitem) {
  return pitem;
}
inline const std::string* getSerializableString(const void* pitem) {
  return NULL;
}
inline const std::string* getSerializableString(const std::string* pitem) {
  return pitem;
}
inline std::string* getSerializableString(void* pitem) {
  return NULL;
}
inline std::string* getSerializableString(std::string* pitem) {
  return pitem;
}

//inline void serialize(std::string& str, const void* pitem) {
//  ABORT("This type cannot be serialized properly!");
//}

inline void sqlize(const void* item, LogNode* node) {
  int next = node->simpleCreate("TEXT", node);
  fprintf(node->file, "%d\t%d\t%p\n", node->logId, next, item);
}

/// serialize object onto str using the fact the object knows how to serialize itself
inline void serialize(std::string& str, const Serializable* pitem) {
  pitem->serialize(str);
}

inline void sqlize(const Serializable* pitem, LogNode* node) {
  pitem->sqlize(node);
}

/// serialize object onto str using a simple append
inline void serialize(std::string& str, const uint8_t* pitem) {
  str.append((char*)pitem, sizeof(uint8_t));
}

inline void sqlize(const uint8_t* pitem, LogNode* node) {
  int next = node->simpleCreate("INT", node);
  fprintf(node->file, "%d\t%d\t%d\n", node->logId, next, *pitem);
}

/// serialize object onto str using htons()
inline void serialize(std::string& str, const uint16_t* pitem) {
  uint16_t tmp = htons(*pitem);
  str.append((char*)&tmp, sizeof(tmp));
}

inline void sqlize(const uint16_t* pitem, LogNode* node) {
  int next = node->simpleCreate("INT", node);
  fprintf(node->file, "%d\t%d\t%d\n", node->logId, next, *pitem);
}

/// serialize object onto str using htonl()
inline void serialize(std::string& str, const uint32_t* pitem) {
  uint32_t tmp = htonl(*pitem);
  str.append((char*)&tmp, sizeof(tmp));
}

// FIX: This is compatibility fix to solve ambiguity problem in new libboost.
inline void sqlize(const uint32_t* pitem, LogNode* node) {
  int next = node->simpleCreate("INT", node);
  fprintf(node->file, "%d\t%d\t%u\n", node->logId, next, *pitem);
}


/// serialize object onto str using htonll()
inline void serialize(std::string& str, const uint64_t* pitem) {
  uint64_t tmp = htonll(*pitem);
  str.append((char*)&tmp, sizeof(tmp));
}

inline void sqlize(const uint64_t* pitem, LogNode* node) {
  int next = node->simpleCreate("NUMERIC(20, 0)", node);
  fprintf(node->file, "%d\t%d\t%"PRIu64"\n", node->logId, next, *pitem);
}


/// serialize object onto str using a simple append
inline void serialize(std::string& str, const int8_t* pitem) {
  str.append((char*)pitem, sizeof(int8_t));
}

inline void sqlize(const int8_t* pitem, LogNode* node) {
  int next = node->simpleCreate("INT", node);
  fprintf(node->file, "%d\t%d\t%d\n", node->logId, next, *pitem);
}

/// serialize object onto str using htons()
inline void serialize(std::string& str, const int16_t* pitem) {
  int16_t tmp = htons(*pitem);
  str.append((char*)&tmp, sizeof(tmp));
}

inline void sqlize(const int16_t* pitem, LogNode* node) {
  int next = node->simpleCreate("INT", node);
  fprintf(node->file, "%d\t%d\t%d\n", node->logId, next, *pitem);
}

/// serialize object onto str using htonl()
inline void serialize(std::string& str, const int32_t* pitem) {
  int32_t tmp = htonl(*pitem);
  str.append((char*)&tmp, sizeof(tmp));
}

inline void sqlize(const int32_t* pitem, LogNode* node) {
  int next = node->simpleCreate("INT", node);
  fprintf(node->file, "%d\t%d\t%d\n", node->logId, next, *pitem);
}


/// serialize object onto str using htonll()
inline void serialize(std::string& str, const int64_t* pitem) {
  int64_t tmp = htonll(*pitem);
  str.append((char*)&tmp, sizeof(tmp));
}

inline void sqlize(const int64_t* pitem, LogNode* node) {
  int next = node->simpleCreate("INT8", node);
  fprintf(node->file, "%d\t%d\t%"PRId64"\n", node->logId, next, *pitem);
}


/// treat pitem as a uint8_t and call serialize()
inline void serialize(std::string& str, const bool* pitem) {
  uint8_t tmp = *pitem;
  serialize(str, &tmp);
}

inline void sqlize(const bool* pitem, LogNode* node) {
  int next = node->simpleCreate("INT", node);
  fprintf(node->file, "%d\t%d\t%d\n", node->logId, next, (*pitem ? 1 : 0));
}

/// serialize a float using append
/// \todo should modify to fix byte ordering
inline void serialize(std::string& str, const float* pitem) {
  str.append((char*)pitem, sizeof(float));
}

inline void sqlize(const float* pitem, LogNode* node) {
  int next = node->simpleCreate("REAL", node);
  fprintf(node->file, "%d\t%d\t%.16f\n", node->logId, next, *pitem);
}

/// serialize a double using append
/// \todo should modify to fix byte ordering
inline void serialize(std::string& str, const double* pitem) {
  str.append((char*)pitem, sizeof(double));
}

inline void sqlize(const double* pitem, LogNode* node) {
  int next = node->simpleCreate("DOUBLE PRECISION", node);
  fprintf(node->file, "%d\t%d\t%.16f\n", node->logId, next, *pitem);
}


/// serialize a string onto a string by serializing first its size, then the contents
inline void serialize(std::string &str, const std::string* pitem) {
  uint32_t sz = (uint32_t)pitem->size();
  mace::serialize(str, &sz);
  if (sz > 0) {
    str.append(pitem->data(), sz);    
  }
}

inline void sqlize(const std::string* pitem, LogNode* node) {
  int next = node->simpleCreate("TEXT", node);
  fprintf(node->file, "%d\t%d\t%s\n", node->logId, next, pitem->c_str());
}

/// serialize a shared pointer by first saving whether its null, then serializing the object
template<typename S>
inline void serialize(std::string &str, const boost::shared_ptr<S>* pitem) {
  uint8_t n = (NULL == pitem->get());
  mace::serialize(str, &n);
  if (n) {
    mace::serialize(str, pitem->get());
  }
}

template<typename S>
inline void sqlize(const boost::shared_ptr<S>* pitem, LogNode* node) {
  uint8_t n = (NULL == pitem->get());
  if (n) {
    mace::sqlize(pitem->get(), node);
  }
}


/// deserialize an object, taking advantage that it knows how to deserialize itself (return bytes deserialized)
inline int deserialize(std::istream& in, Serializable* pitem) throw(SerializationException) {
  return pitem->deserialize(in);
}

/// read in a byte for the integer
inline int deserialize(std::istream& in, int8_t* pitem) throw(SerializationException) {
  in.read((char *)pitem, sizeof(int8_t));
  if (!in) {
    throw SerializationException("could not read " +
				 boost::lexical_cast<std::string>(sizeof(int8_t)) + " bytes");
  }
  return sizeof(int8_t);
}
/// read in two bytes, using ntohs
inline int deserialize(std::istream& in, int16_t* pitem) throw(SerializationException) {
  in.read((char *)pitem, sizeof(int16_t));
  if (!in) {
    throw SerializationException("could not read " +
				 boost::lexical_cast<std::string>(sizeof(int16_t)) + " bytes");
  }
  *pitem = ntohs(*pitem);
  return sizeof(int16_t);
}
/// read in four bytes, using ntohl
inline int deserialize(std::istream& in, int32_t* pitem) throw(SerializationException) {
  in.read((char *)pitem, sizeof(int32_t));
  if (!in) {
    throw SerializationException("could not read " +
				 boost::lexical_cast<std::string>(sizeof(int32_t)) + " bytes");
  }
  *pitem = ntohl(*pitem);
  return sizeof(int32_t);
}



/// read in eight bytes, using ntohll
inline int deserialize(std::istream& in, int64_t* pitem) throw(SerializationException) {
  in.read((char *)pitem, sizeof(int64_t));
  if (!in) {
    throw SerializationException("could not read " +
				 boost::lexical_cast<std::string>(sizeof(int64_t)) + " bytes");
  }
  *pitem = ntohll(*pitem);
  return sizeof(int64_t);
}

/// read in a byte for the integer
inline int deserialize(std::istream& in, uint8_t* pitem) throw(SerializationException) {
  in.read((char *)pitem, sizeof(uint8_t));
  if (!in) {
    throw SerializationException("could not read " +
				 boost::lexical_cast<std::string>(sizeof(uint8_t)) + " bytes");
  }
  return sizeof(uint8_t);
}
/// read in two bytes, using ntohs
inline int deserialize(std::istream& in, uint16_t* pitem) throw(SerializationException) {
  in.read((char *)pitem, sizeof(uint16_t));
  if (!in) {
    throw SerializationException("could not read " +
				 boost::lexical_cast<std::string>(sizeof(uint16_t)) + " bytes");
  }
  *pitem = ntohs(*pitem);
  return sizeof(uint16_t);
}
/// read in four bytes, using ntohl
inline int deserialize(std::istream& in, uint32_t* pitem) throw(SerializationException) {
  in.read((char *)pitem, sizeof(uint32_t));
  if (!in) {
    throw SerializationException("could not read " +
				 boost::lexical_cast<std::string>(sizeof(uint32_t)) + " bytes");
  }
  *pitem = ntohl(*pitem);
  return sizeof(uint32_t);
}

//FIX by SH.
/// read in eight bytes, using ntohll
inline int deserialize(std::istream& in, uint64_t* pitem) throw(SerializationException) {
  in.read((char *)pitem, sizeof(uint64_t));
  if (!in) {
    throw SerializationException("could not read " +
				 boost::lexical_cast<std::string>(sizeof(uint64_t)) + " bytes");
  }
  *pitem = ntohll(*pitem);
  return sizeof(uint64_t);
}

/// treat like an uint8_t
inline int deserialize(std::istream& in, bool* pitem) throw(SerializationException) {
  uint8_t tmp;
  deserialize(in, &tmp);
  *pitem = tmp;
  return sizeof(uint8_t);
}

/// read in 4 bytes
/// \todo modify to do byte-swapping
inline int deserialize(std::istream& in, float* pitem) throw(SerializationException) {
  in.read((char *)pitem, sizeof(float));
  if (!in) {
    throw SerializationException("could not read " +
				 boost::lexical_cast<std::string>(sizeof(float)) + " bytes");
  }
  return sizeof(float);
}
/// read in 8 bytes
/// \todo modify to do byte-swapping
inline int deserialize(std::istream& in, double* pitem) throw(SerializationException) {
  in.read((char *)pitem, sizeof(double));
  if (!in) {
    throw SerializationException("could not read " +
				 boost::lexical_cast<std::string>(sizeof(double)) + " bytes");
  }
  return sizeof(double);
}
/// deserialize by reading first the size, then extracting that number of bytes
inline int deserialize(std::istream& in, std::string* pitem) throw(SerializationException){
  uint32_t sz;
  char* buf;
  mace::deserialize(in, &sz);
  if (sz == 0) {
    pitem->clear();
    return sizeof(sz);
  }
  buf = new char[sz];
  if (buf == NULL) {
    throw SerializationException("Could not allocate memory to deserialize");
  }

  in.read(buf, sz);
  if (!in) {
      throw SerializationException("String not long enough to deserialize");
  }
  pitem->assign(buf, sz);
  delete [] buf;
  return sizeof(sz) + sz;
}

template<typename S>
inline int deserialize(std::istream& in, boost::shared_ptr<S>* pitem) throw(SerializationException) {
  uint32_t sz = 0;
  uint8_t n;
  sz += mace::deserialize(in, &n);
  if (n) {
    S* s = new S();
    sz += mace::deserialize(in, s);
    *pitem = boost::shared_ptr<S>(s);
  }
  return sz;
}
/// deserialize from a string (rather than istream) by using an istringstream
template<typename S> int deserializeStr(const std::string& str, S* pitem) {
  std::istringstream in(str);
  return mace::deserialize(in, pitem);
}
// /// special handling for deserializing a string from a string -- use whole string with assign.  
// template<typename S> int deserializeStr(const std::string& str, std::string* pitem) {
//   pitem->assign(str);
//   return str.size();
// }
// /// special handling for serializing a string, returning a string -- just return the string
// template<typename S> std::string serialize(const std::string* pitem) {
//   return *pitem;
// }
/// serialize an arbitrary item into a string to return by calling either serialize(string&, const S*) or using serializeStr()
template<typename S> std::string serialize(const S* pitem) {
  const std::string* sptr = getSerializableString(pitem);
  if (sptr != NULL) {
    return *sptr;
  }
  const Serializable* tmp = getSerializable(pitem);
  if (tmp == NULL) {
    std::string str;
    serialize(str, pitem);
    return str;
  }
  else {
    return tmp->serializeStr();
  }
}

// template<typename S> serialize(const S* pitem, std::string& str) {
//   const std::string* sptr = getSerializableString(pitem);
//   if (sptr != NULL) {
//     str = *sptr;
//     return;
//   }
//   const Serializable* tmp = getSerializable(pitem);
//   if (tmp == NULL) {
//     serialize(str, pitem);
//   }
//   else {
//     tmp->serializeStr(str);
//   }
// }

/// deserialize an object object from a string (rather than istream)
template<typename S> void deserialize(const std::string& s, S* pitem) throw(SerializationException) {
  std::string* sptr = getSerializableString(pitem);
  if (sptr != NULL) {
    sptr->assign(s);
    return;
  }
  Serializable* tmp = getSerializable(pitem);
  if (tmp == NULL) {
    deserializeStr(s, pitem);
  }
  else {
    tmp->deserializeStr(s);
  }
}

/// Generic (and now unused) method for serializing a map
template<typename S> void serializeMap(std::string& str, const S& s) {
  uint32_t sz = (uint32_t)s.size();
  mace::serialize(str, &sz);
  for(typename S::const_iterator i=s.begin(); i!=s.end(); i++) {
    mace::serialize(str, &(i->first));
    mace::serialize(str, &(i->second));
  }
}
/// Generic (and now unused) method for deserializing a map
template<typename S> int deserializeMap(std::istream& in, S& s) throw(SerializationException) {
  typename S::key_type k;
  uint32_t sz;
  int bytes = sizeof(sz);

  s.clear();
  mace::deserialize(in, &sz);
  for (uint64_t i = 0; i < sz; i++) {
    bytes += mace::deserialize(in, &k);
    bytes += mace::deserialize(in, s[k]);
  }
  return bytes;
}

  /// class for use with concept checking to make better error messages
  template<typename T> class SerializeConcept {
  public:
    void constraints() {
      std::string s;
      T t;
      serialize(s, &t);
      std::istringstream in(s);
      deserialize(in, &t);
    }
  };

} // namespace mace
#endif
