/* 
 * StrUtil.h : part of the Mace toolkit for building distributed systems
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
#include <stdint.h>

#include <sys/types.h>
#include <regex.h>
#include <ctype.h>
#include <string>
#include <sstream>

#include "LRUCache.h"
#include "mhash_map.h"
#include "mdeque.h"
#include "hash_string.h"
#include "Exception.h"
#include "Collections.h"
#include "StrUtilNamespace.h"

#ifndef STRUTIL_H
#define STRUTIL_H

/**
 * \file StrUtil.h
 * \brief string utilities defined in StrUtil
 */

// typedef mace::LRUCache<std::string, regex_t*, hash_string> RegexCache;
typedef mace::LRUCache<std::string, regex_t*> RegexCache;

class RegexException : public Exception {
public:
  RegexException(const std::string& m) : Exception(m) { }
  virtual ~RegexException() throw() {}
  void rethrow() const { throw *this; }
}; // RegexException

/**
 * \addtogroup Utils
 * @{
 */

/// Helper class for operating on and with strings
class StrUtil {
public:
  static std::string trim(const std::string& s); ///< trim whitespace from the front and back of strings
  static std::string trimFront(const std::string& s); ///< trim whitespace from the front of the string
  static std::string trimBack(const std::string& s, char c); ///< trim whitespace from the back of the string
  static std::string toLower(std::string s); ///< convert all characters to lowercase
  static std::string toUpper(std::string s); ///< convert all characters to uppercase

  /// Treat src as a string and "increment" it, returning a string that is 
  /// strictly larger than it.
  static std::string increment(std::string src);

  /// replace all occurrences of target in src with replacement
  static std::string replaceAll(std::string src, const std::string& target,
				const std::string& replacement); 

  /**
   * \brief reads a line terminated with \verbatim '\n' \endverbatim or \verbatim '\r\n' \endverbatim from src into dst,
   * returns number of bytes extracted from src
   */
  static size_t readLine(std::string& src, std::string& dst);

  /**
   * \brief reads from src into dest n bytes 
   */
  static size_t read(std::string& src, std::string& dst, size_t n);

  /// returns true if the string contains certain regex characters (\.*)
  static bool isRegex(const std::string& s);

  /// matches string s against regular expression regexp
  static bool matches(const std::string& regexp, const std::string& s,
		      bool ignoreCase = false, bool matchNewline = false)
    throw (RegexException);

  /// \todo JWA please document
  static StringList match(const std::string& regexp, const std::string& s,
			  bool ignoreCase = false, bool matchNewline = false)
    throw (RegexException);

  /// \todo JWA please document
  static bool match(const std::string& regexp, const std::string& s,
		    StringList& matchList,
		    bool ignoreCase = false, bool matchNewline = false)
    throw (RegexException);

  /// translates a perl regular expression into a regex regular expression
  static std::string translatePerlRE(std::string re);

  /// \todo JWA please document
  static void toCStr(const StringList& l, const char* a[]);

  /// splits string \c s on delimiter \c delim, optionally returning empty elements
  static StringList split(const std::string& delim, std::string s, bool returnEmpty = false);
  /// joins string list \c l using delimiter \c delim
  static std::string join(const std::string& delim, const StringList& l);

  /// returns true if the string contains only printable characters
  static bool isPrintable(const std::string& s);
  /// \todo JWA please document
  static std::string spaces(size_t w, size_t l);
  /// convert arbirtary types \c T to string, using boost::lexical_cast
  template <typename T>
  static mace::string toString(T v) {
    return boost::lexical_cast<mace::string>(v);
  }
  /// uses boost::lexical_cast to try to convert a string to an arbitrary type, returning true if it can
  template <typename T>
  static bool isType(const std::string& s) {
    try {
      boost::lexical_cast<T>(s);
    }
    catch (const boost::bad_lexical_cast& e) {
      return false;
    }
    return true;
  }
  /// uses boost::lexical_cast to try to convert a string to an arbitrary type, storing in \c v, returning true if it can
  template <typename T>
  static bool tryCast(const std::string& s, T& v) {
    try {
      v = boost::lexical_cast<T>(s);
    }
    catch (const boost::bad_lexical_cast& e) {
      return false;
    }
    return true;
  }

private:
  static void throwRegexException(const std::string& re, int error, regex_t* rex)
    throw (RegexException);
  static regex_t* compileRegex(std::string re, bool ignoreCase, bool matchNewline,
			       bool nosub) throw (RegexException);
  static void freeRegex(const std::string& re, regex_t& rex);

private:

  static StringIntHMap cflagMap;
  static RegexCache regexCache;

};

/** @} */

#endif // STRUTIL_H
