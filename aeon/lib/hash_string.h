/* 
 * hash_string.h : part of the Mace toolkit for building distributed systems
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
#ifndef HASH_STRING_H
#define HASH_STRING_H

#include <sys/types.h>
#include <cstring>
#include <string>
#include <stdint.h>

/**
 * \file hash_string.h
 * \brief defines hash functors to support using strings in hash maps.
 */

#if __GNUC__ >= 3
  #if __GNUC__ >= 4
    #if __GNUC_MINOR__ >= 3
      #include <tr1/unordered_map>
#      define __MACE_USE_UNORDERED__
#      define __MACE_BASE_HASH__ std::tr1::hash
    #else 
      #include <ext/hash_map>
#      define __MACE_BASE_HASH__ __gnu_cxx::hash
    #endif
  #else
    #include <ext/hash_map>
#    define __MACE_BASE_HASH__ __gnu_cxx::hash
  #endif
#else
  #include <hash_map>
#  define __MACE_BASE_HASH__ __gnu_cxx::hash
#endif

/**
 * \brief simple template class for hashing the bytes of arbitrary objects
 *
 * Treats T like a string of sizeof(T) bytes.  Computes the hash using:
 * \f$ 31 \times h + s_i (mod 256) \f$
 * for each byte \c i.
 */
template<class T>
struct hash_bytes {
  uint32_t operator() (const T __x) const {
    unsigned long __h = 0;
    char* __s = (char*)&__x;
    for (unsigned int i = 0 ; i < sizeof(T); i++) {
      __h = 31 * __h + __s[i];
    }
    return size_t(__h);
  }
};

/**
 * \brief a stuct (functor) for computing the hash of a string for hash maps 
 *
 * Computes the hash using:
 * \f$ 31 \times h + s_i (mod 256) \f$
 * for each byte \c i in the string.
 */
struct hash_string {
  uint32_t operator() (const std::string& __x) const {
    unsigned long __h = 0;
    const char* __s = __x.data();
    for (unsigned int i = 0 ; i < __x.size(); i++) {
      __h = 31 * __h + __s[i];
    }
    return uint32_t(__h);
  }
};

/// implements a functor for string less than
struct ltstring {
  bool operator()(const std::string& s1, const std::string& s2) const
  {
    return strcmp(s1.c_str(), s2.c_str()) < 0;
  }
};

/// implements a functor for string greater than
struct gtstring {
  bool operator()(const std::string& s1, const std::string& s2) const
  {
    return strcmp(s1.c_str(), s2.c_str()) > 0;
  }
};

class HashString {
public:
  static uint32_t hash(const std::string& x) {
    static hash_string hasher;
    return hasher(x);
  }
};

// struct hash_string {
//   size_t operator() (const std::string& __s) const {
//     return __stl_hash_string(__s.c_str());
//   }
// };

#endif // HASH_STRING_H
