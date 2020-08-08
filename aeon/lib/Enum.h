/* 
 * Enum.h : part of the Mace toolkit for building distributed systems
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
#ifndef __ENUM_H
#define __ENUM_H

#include "Serializable.h"
#include "Printable.h"

/**
 * \file Enum.h
 * \brief defines the Enum class for serialization and printing of enums
 */

/// this namespace contains the helper methods for printing, serializing, and deserializing references from enums.  (Not documented)
namespace EnumNS {

inline void printRef(std::ostream& out, const int8_t& val)      { out << val; }
inline void printRef(std::ostream& out, const int16_t& val)     { out << val; }
inline void printRef(std::ostream& out, const int32_t& val)     { out << val; }
inline void printRef(std::ostream& out, const int64_t& val)     { out << val; }
inline void printRef(std::ostream& out, const uint8_t& val)     { out << val; }
inline void printRef(std::ostream& out, const uint16_t& val)    { out << val; }
inline void printRef(std::ostream& out, const uint32_t& val)    { out << val; }
inline void printRef(std::ostream& out, const uint64_t& val)    { out << val; }
inline void serializeRef(std::string& str, const int8_t& val)   { mace::serialize(str, &val); }
inline void serializeRef(std::string& str, const int16_t& val)  { mace::serialize(str, &val); }
inline void serializeRef(std::string& str, const int32_t& val)  { mace::serialize(str, &val); }
inline void serializeRef(std::string& str, const int64_t& val)  { mace::serialize(str, &val); }
inline void serializeRef(std::string& str, const uint8_t& val)  { mace::serialize(str, &val); }
inline void serializeRef(std::string& str, const uint16_t& val) { mace::serialize(str, &val); }
inline void serializeRef(std::string& str, const uint32_t& val) { mace::serialize(str, &val); }
inline void serializeRef(std::string& str, const uint64_t& val) { mace::serialize(str, &val); }
inline void sqlizeRef(mace::LogNode* node, const int8_t& val)   { mace::sqlize(&val, node); }
inline void sqlizeRef(mace::LogNode* node, const int16_t& val)  { mace::sqlize(&val, node); }
inline void sqlizeRef(mace::LogNode* node, const int32_t& val)  { mace::sqlize(&val, node); }
inline void sqlizeRef(mace::LogNode* node, const int64_t& val)  { mace::sqlize(&val, node); }
inline void sqlizeRef(mace::LogNode* node, const uint8_t& val)  { mace::sqlize(&val, node); }
inline void sqlizeRef(mace::LogNode* node, const uint16_t& val) { mace::sqlize(&val, node); }
inline void sqlizeRef(mace::LogNode* node, const uint32_t& val) { mace::sqlize(&val, node); }
inline void sqlizeRef(mace::LogNode* node, const uint64_t& val) { mace::sqlize(&val, node); }
inline int  deserializeRef(std::istream& in, int8_t& val)       { return mace::deserialize(in, &val); }
inline int  deserializeRef(std::istream& in, int16_t& val)      { return mace::deserialize(in, &val); }
inline int  deserializeRef(std::istream& in, int32_t& val)      { return mace::deserialize(in, &val); }
inline int  deserializeRef(std::istream& in, int64_t& val)      { return mace::deserialize(in, &val); }
inline int  deserializeRef(std::istream& in, uint8_t& val)      { return mace::deserialize(in, &val); }
inline int  deserializeRef(std::istream& in, uint16_t& val)     { return mace::deserialize(in, &val); }
inline int  deserializeRef(std::istream& in, uint32_t& val)     { return mace::deserialize(in, &val); }
inline int  deserializeRef(std::istream& in, uint64_t& val)     { return mace::deserialize(in, &val); }

template<typename T> int deserializeRef(std::istream& in, const int8_t& val,   T& tval)  { int8_t v;   int rv = mace::deserialize(in, &v); tval = (T)v; return rv;}
template<typename T> int deserializeRef(std::istream& in, const int16_t& val,  T& tval)  { int16_t v;  int rv = mace::deserialize(in, &v); tval = (T)v; return rv;}
template<typename T> int deserializeRef(std::istream& in, const int32_t& val,  T& tval)  { int32_t v;  int rv = mace::deserialize(in, &v); tval = (T)v; return rv;}
template<typename T> int deserializeRef(std::istream& in, const int64_t& val,  T& tval)  { int64_t v;  int rv = mace::deserialize(in, &v); tval = (T)v; return rv;}
template<typename T> int deserializeRef(std::istream& in, const uint8_t& val,  T& tval)  { uint8_t v;  int rv = mace::deserialize(in, &v); tval = (T)v; return rv;}
template<typename T> int deserializeRef(std::istream& in, const uint16_t& val, T& tval)  { uint16_t v; int rv = mace::deserialize(in, &v); tval = (T)v; return rv;}
template<typename T> int deserializeRef(std::istream& in, const uint32_t& val, T& tval)  { uint32_t v; int rv = mace::deserialize(in, &v); tval = (T)v; return rv;}
template<typename T> int deserializeRef(std::istream& in, const uint64_t& val, T& tval)  { uint64_t v; int rv = mace::deserialize(in, &v); tval = (T)v; return rv;}

#ifdef __CYGWIN32__
inline void printRef(std::ostream& out, const int& val)     { out << val; }
inline void serializeRef(std::string& str, const int& val)  { mace::serialize(str, &val); }
inline int  deserializeRef(std::istream& in, int& val)      { return mace::deserialize(in, &val); }
template<typename T> int deserializeRef(std::istream& in, const int& val,  T& tval)  { int v;  int rv = mace::deserialize(in, &v); tval = (T)v; return rv;}

inline void printRef(std::ostream& out, const unsigned int& val)     { out << val; }
inline void serializeRef(std::string& str, const unsigned int& val)  { mace::serialize(str, &val); }
inline int  deserializeRef(std::istream& in, unsigned int& val)      { return mace::deserialize(in, &val); }
template<typename T> int deserializeRef(std::istream& in, const unsigned int& val,  T& tval)  { unsigned int v;  int rv = mace::deserialize(in, &v); tval = (T)v; return rv;}
#else
// inline void printRef(std::ostream& out, const long int& val)     { out << val; }
// inline void serializeRef(std::string& str, const long int& val)  { mace::serialize(str, &val); }
// inline int  deserializeRef(std::istream& in, long int& val)      { return mace::deserialize(in, &val); }
// template<typename T> int deserializeRef(std::istream& in, const long int& val,  T& tval)  { long int v;  int rv = mace::deserialize(in, &v); tval = (T)v; return rv;}

// inline void printRef(std::ostream& out, const unsigned long int& val)     { out << val; }
// inline void serializeRef(std::string& str, const unsigned long int& val)  { mace::serialize(str, &val); }
// inline int  deserializeRef(std::istream& in, unsigned long int& val)      { return mace::deserialize(in, &val); }
// template<typename T> int deserializeRef(std::istream& in, const unsigned long int& val,  T& tval)  { unsigned long int v;  int rv = mace::deserialize(in, &v); tval = (T)v; return rv;}
#endif

} //namespace EnumNS

/**
 * \brief A template class for enumerations which allows them to be printable and serializable.
 *
 * Should work transparently in place of an enum when templated on that enum
 * type.  It auto-casts to the enum type and can be compared or assigned based
 * on the enum type.
 *
 * Example:
 * \code
 * enum _state_type { init, joined, joining };
 * typedef Enum<_state_type> state_type;
 * state_type s = init;
 * maceout << "The state s is " << s << Log::endl;
 * \endcode
 */
template<class E> class Enum : public mace::PrintPrintable, public mace::Serializable {
private:
  E e; ///< internally stores an enum object.
public:
#ifndef __clang__
  operator E() { return e; } ///< auto-cast to the templated enumeration type
#endif //__clang__
  operator E() const { return e; } ///< const auto-cast to the templated enumeration type
#ifndef __clang__
#if (__GNUC__ * 100 + __GNUC_MINOR__) <= 404
  operator int64_t() const { return e; } // int overload up till gcc-4.4
#endif
#endif //__clang__
  Enum& operator=(const Enum& oe) { e = oe.e; return *this; } ///< assignment operator for another Enum object
  Enum& operator=(const E& oe) { e = oe; return *this; } ///< assignment operator that takes a templated enumeration type
  bool operator==(const E& oe) const { return e == oe; } ///< equality with another Enum object
  bool operator!=(const E& oe) const { return e != oe; } ///< inequality with another Enum object
  Enum() : e() {} ///< default constructor
  Enum(const E& oe) : e(oe) {} ///< constructor which takes a templated enumeration type object
  Enum(const Enum& oe) : e(oe.e) {} ///< copy-constructor
  void print(std::ostream& out) const { 
    EnumNS::printRef(out, e);
  }

  int deserialize(std::istream& in) throw(mace::SerializationException) {
    return EnumNS::deserializeRef(in, e, e);
  }
  void serialize(std::string& str) const {
    EnumNS::serializeRef(str, e);
  }
  
  void sqlize(mace::LogNode* node) const {
    EnumNS::sqlizeRef(node, e);
  }
  
  int deserializeXML_RPC(std::istream& in) throw(mace::SerializationException) {
    return EnumNS::deserializeRef(in, e, e);
  }
  void serializeXML_RPC(std::string& str) const throw(mace::SerializationException) {
    EnumNS::serializeRef(str, e);
  }
};

#endif //__ENUM_H

