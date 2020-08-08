/* 
 * StrUtil.cc : part of the Mace toolkit for building distributed systems
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
#include <cassert>

#include "StrUtil.h"

using std::string;
using std::ostringstream;

StringIntHMap StrUtil::cflagMap;
RegexCache StrUtil::regexCache;

string StrUtil::trim(const string& s) {
  size_t start = 0;
  while (isspace(s[start])) {
    start++;
  }

  size_t end = s.size();
  while (end >= start && isspace(s[end - 1])) {
    end--;
  }
    
  if (end < start) {
    return "";
  }

  return s.substr(start, end - start);
} // trim

string StrUtil::trimFront(const string& s) {
  size_t start = 0;
  size_t sz = s.size();
  while (start < sz && isspace(s[start])) {
    start++;
  }

  return s.substr(start);
} // trim

string StrUtil::trimBack(const string& s, char c) {
  size_t end = s.size();
  while (end > 0 && s[end - 1] == c) {
    end--;
  }
    
  return s.substr(0, end);
} // trimBack

string StrUtil::toLower(string s) {
  for (uint i = 0; i < s.size(); i++) {
    s[i] = ::tolower(s[i]);
  }
  return s;
} // toLower

string StrUtil::toUpper(string s) {
  for (uint i = 0; i < s.size(); i++) {
    s[i] = ::toupper(s[i]);
  }
  return s;
} // toLower

string StrUtil::increment(string s) {
  char& c = s.at(s.size() - 1);
  
  if ((unsigned char) c < std::numeric_limits<uint8_t>::max()) {
    c += 1;
  } else {
    s.push_back((char) 0);
  }

  return s;
}

size_t StrUtil::read(string& src, string& buf, size_t n) {
  buf.clear();
  if (src.size() < n) {
    return 0;
  }

  buf = src.substr(0, n);
  src = src.substr(n);
  return n;
} // read

size_t StrUtil::readLine(string& src, string& buf) {
  string::size_type rn = src.find("\r\n");
  string::size_type n = src.find("\n");
  size_t bytesRead = 0;

  buf.clear();

  if (rn != string::npos) {
    buf = src.substr(0, rn);
    bytesRead = buf.size() + 2;
  }
  else if (n != string::npos) {
    buf = src.substr(0, n);
    bytesRead = buf.size() + 1;
  }

  if (bytesRead > 0) {
    src = src.substr(bytesRead);
  }

  return bytesRead;
} // parseLine

string StrUtil::replaceAll(string s, const string& t, const string& r) {
  while (true) {
    string::size_type i = s.find(t);
    if (i == string::npos) {
      return s;
    }
    s.replace(i, t.size(), r);
  }
} // replaceAll

bool StrUtil::isRegex(const string& s) {
  if (s.find('\\') != string::npos ||
      s.find('.') != string::npos ||
      s.find('+') != string::npos ||
      s.find('*') != string::npos ||
      s.find('?') != string::npos) {
    return true;
  }
  return false;
} // isRegex

bool StrUtil::matches(const string& re, const string& s, bool ignoreCase,
		      bool matchNewline) throw (RegexException) {

  regex_t* rex = compileRegex(re, ignoreCase, matchNewline, true);

  int match = regexec(rex, s.c_str(), 0, 0, 0);

  return (match == 0);
} // matches

StringList StrUtil::match(const string& re, const string& s, bool ignoreCase,
			  bool matchNewline) throw (RegexException) {
  StringList r;
  match(re, s, r, ignoreCase, matchNewline);
  return r;
} // match

bool StrUtil::match(const string& re, const string& s, StringList& r,
		    bool ignoreCase, bool matchNewline) throw (RegexException) {
  regex_t* rex = compileRegex(re, ignoreCase, matchNewline, false);

  size_t nmatch = 10;
  regmatch_t pmatch[nmatch];
  int match = regexec(rex, s.c_str(), nmatch, pmatch, 0);

  if (match == 0) {
    // build matches -- ignore first entry
//     for (size_t i = 1; (i < nmatch) && (pmatch[i].rm_so != -1); i++) {
    size_t last;
    for (last = nmatch; pmatch[last - 1].rm_so == -1; last--) { }

    for (size_t i = 1; i < last; i++) {
      regmatch_t offsets = pmatch[i];
//       printf("offsets[%d]: %d %d\n", i, offsets.rm_so, offsets.rm_eo);
      if (pmatch[i].rm_so != -1) {
	r.push_back(s.substr(offsets.rm_so, offsets.rm_eo - offsets.rm_so));
      }
      else {
	r.push_back(string());
      }
    }
    return true;
  }
  ASSERT(match == REG_NOMATCH);
  return false;
} // match

void StrUtil::throwRegexException(const string& re, int error, regex_t* rex)
  throw (RegexException) {
  if (error != 0) {
    size_t len = regerror(error, rex, 0, 0);
    char errbuf[len];
    regerror(error, rex, errbuf, len);
    string errstr = "regex error " + re + ": ";
    errstr.append(errbuf);
    throw RegexException(errstr);
  }
} // throwRegexException

regex_t* StrUtil::compileRegex(string re, bool ignoreCase, bool matchNewline,
			       bool nosub) throw (RegexException) {

  int cflags = REG_EXTENDED;
  if (nosub) {
    cflags |= REG_NOSUB;
  }
  if (ignoreCase) {
    cflags |= REG_ICASE;
  }
  if (!matchNewline) {
    cflags |= REG_NEWLINE;
  }


  if (regexCache.containsKey(re)) {
    if (cflagMap[re] == cflags) {
      return regexCache[re];
    }
    else {
      regex_t* delrex = regexCache[re];
      regfree(delrex);
      delete delrex;
      delrex = 0;
      regexCache.remove(re);
      cflagMap.erase(re);
    }
  }
  else if (regexCache.isFullDirty()) {
    string del = regexCache.getLastDirtyKey();
    regex_t* delrex = regexCache[del];

    assert(delrex);
    regfree(delrex);
    cflagMap.erase(del);
    delete delrex;
    delrex = 0;
  }

  string tre = translatePerlRE(re);

  regex_t* rex = new regex_t;
  int error = regcomp(rex, tre.c_str(), cflags);
  if (error != 0) {
    regfree(rex);
    delete rex;
    rex = 0;
    throwRegexException(re, error, rex);
  }

  regexCache.addDirty(re, rex);
  cflagMap[re] = cflags;
  return rex;
} // compileRegex

string StrUtil::translatePerlRE(string re) {
  re = replaceAll(re, "\\w", "[[:alnum:]_]");
  re = replaceAll(re, "\\W", "[^[:alnum:]_]");
  re = replaceAll(re, "\\d", "[[:digit:]]");
  re = replaceAll(re, "\\D", "[^[:digit:]]");
  re = replaceAll(re, "\\s", "[[:space:]]");
  re = replaceAll(re, "\\S", "[^[:space:]]");
  return re;
} // translatePerlRE

void StrUtil::toCStr(const StringList& l, const char* a[]) {
  for (size_t i = 0; i < l.size(); i++) {
    a[i] = l[i].c_str();
  }
} // toCStr

bool StrUtil::isPrintable(const string& s) {
  size_t i = 0;
  while (i < s.size() && isprint(s[i])) {
    i++;
  }
  return (i == s.size());
} // isPrintable

StringList StrUtil::split(const string& delim, string s, bool returnEmpty) {
  StringList r;
  while (!s.empty()) {
    while (s.find(delim) == 0) {
      s = s.substr(delim.size());
      if (returnEmpty) {
        r.push_back("");
      }
    }
    size_t i = s.find(delim);
    if (i != string::npos) {
      r.push_back(s.substr(0, i));
      s = s.substr(i + delim.size());
    }
    else {
      // no more delims
      if (!s.empty()) {
	r.push_back(s);
	return r;
      }
    }
  }

  if (returnEmpty) {
    r.push_back("");
  }

  return r;
} // split

string StrUtil::join(const string& delim, const StringList& l) {
  string r = "";
  if (!l.empty()) {
    size_t i = 0;
    while (i < l.size() - 1) {
      r.append(l[i]);
      r.append(delim);
      i++;
    }
    r.append(l[i]);
  }
  return r;
} // join

string StrUtil::spaces(size_t w, size_t l) {
  if (l >= w) {
    return " ";
  }
  else {
    return string(w - l, ' ');
  }
} // spaces

namespace StrUtilNamespace {
StdStringList getTypeFromTemplate(const std::string& fn, const char* typeVar[]) {
  StdStringList ret;
  string find = "[with ";
  find += typeVar[0];
  find += " = ";
  size_t start = std::string::npos;
  size_t end = std::string::npos;

  start = fn.find(find);
  if(start == std::string::npos) {
    ASSERT(0);
  }
  start = start + 6 + strlen(typeVar[0]) + 3;

  int i = 0;
  //iterate over typevar
  while(typeVar[i+1] != 0) {
    //look for ", $next = "
    find = ", " + std::string(typeVar[i+1]) + " = ";
    end = fn.find(find);
    if(end == std::string::npos) {
      ASSERT(0);
    }
    ret.push_back(fn.substr(start, end-start));
    start = end + 2 + strlen(typeVar[i+1]) + 3;
    i++;
  }
  //else look for "]"
  find = "]";
  end = fn.find(find);
  if(end == std::string::npos) {
    ASSERT(0);
  }
  ret.push_back(fn.substr(start, end-start));
  
  return ret;
}
}

