/* 
 * HttpResponse.h : part of the Mace toolkit for building distributed systems
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
#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <string>

#include "TimeUtil.h"
#include "Collections.h"
#include "mhash_map.h"

class HttpResponse {
public:
  HttpResponse(const std::string& host = "", int port = 0,
	       const std::string& url = "",
	       int rc = 0, const std::string& rm = "",
	       const std::string& content = "",
	       const std::string& httpVersion = "1.0",
	       bool keepAlive = false,
	       const std::string& rawheaders = "",
	       bool persistent = false);
  bool isSuccess() const { return responseCode == OK; }

  static std::string getResponseHeader(int c, const std::string& vers);
  static std::string getBodyHeader(std::string type, uint len, bool keepAlive,
				   bool chunked = false,
				   const std::string& location = "");

public:
  static const int CONTINUE = 100;
  static const int OK = 200;
  static const int MOVED_PERM = 301;
  static const int MOVED = 302;
  static const int BAD_REQUEST = 400;
  static const int UNAUTHORIZED = 401;
  static const int FORBIDDEN = 403;
  static const int NOT_FOUND = 404;
  static const int REQUEST_TIMEOUT = 408;
  static const int SERVER_ERROR = 500;

  mace::string host;
  int port;
  mace::string url;
  int responseCode;
  mace::string responseMessage;
  mace::string content;
  mace::string httpVersion;
  bool keepAlive;
  mace::string rawheaders;
  StringHMap headers;
  bool persistent;
  uint64_t requestBegin;
  uint64_t requestComplete;
  uint64_t responseBegin;
  uint64_t responseComplete;

  uint64_t requestTime() const { return requestComplete - requestBegin; }
  uint64_t responseTime() const { return responseComplete - responseBegin; }
  uint64_t totalTime() const { return responseComplete - requestBegin; }
  void beginRequest() { requestBegin = TimeUtil::timeu(); }
  void completeRequest() { requestComplete = TimeUtil::timeu(); }
  void beginResponse() { responseBegin = TimeUtil::timeu(); }
  void completeResponse() { responseComplete = TimeUtil::timeu(); }

private:
  static mace::hash_map<int, std::string> returnStrings;

};

#endif // HTTPRESPONSE_H
