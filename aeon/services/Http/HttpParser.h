/* 
 * HttpParser.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson, Ryan Braud, Charles Killian
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
#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include <sys/types.h>
#include <string>

#include "Collections.h"

#include "HttpResponse.h"
#include "HttpRequestContext.h"

class HttpParser {
public:
  static void parseRequest(std::string buf, HttpRequestContext& context,
			   size_t& lengthUsed, bool& valid, bool& done);
  static bool readHeader(std::string& request, StringList& req, size_t& lengthUsed);
  static bool parseHeaders(StringList& req, HttpRequestContext& context,
			   size_t& contentLength);
  static void readChunked(std::string& request, std::string& content,
			  size_t& lengthUsed, bool& valid, bool& done);
  static bool readContent(std::string& request, HttpRequestContext& context,
			  size_t contentLength, size_t& lengthUsed,
			  bool& valid, bool& done);
  static bool parseRequestHeaderLine(StringList& req, HttpRequestContext& context);
  static bool parseResponseHeaderLine(StringList& req, HttpResponse& response);
  static bool validPath(std::string p);
}; // HttpParser

#endif // HTTPPARSER_H
