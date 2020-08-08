/* 
 * HttpResponse.cc : part of the Mace toolkit for building distributed systems
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
#include <sstream>
#include <time.h>
#include "HttpResponse.h"

using namespace std;

mace::hash_map<int, std::string> HttpResponse::returnStrings;

const int HttpResponse::CONTINUE;
const int HttpResponse::OK;
const int HttpResponse::MOVED_PERM;
const int HttpResponse::MOVED;
const int HttpResponse::BAD_REQUEST;
const int HttpResponse::UNAUTHORIZED;
const int HttpResponse::FORBIDDEN;
const int HttpResponse::NOT_FOUND;
const int HttpResponse::SERVER_ERROR;

HttpResponse::HttpResponse(const string& h, int p, const string& url,
			   int rc, const string& rm, const string& content,
			   const string& vers, bool keepAlive,
			   const string& rawheaders, bool persist) :
  host(h), port(p), url(url), responseCode(rc), responseMessage(rm),
  content(content),
  httpVersion(vers), keepAlive(keepAlive), rawheaders(rawheaders),
  persistent(persist),
  requestBegin(0), requestComplete(0), responseBegin(0), responseComplete(0) {
  
} // HttpResponse

string HttpResponse::getResponseHeader(int c, const string& vers) {
  if (returnStrings.empty()) {
    returnStrings[CONTINUE] = "Continue";
    returnStrings[OK] = "OK";
    returnStrings[MOVED_PERM] = "Moved Permanently";
    returnStrings[MOVED] = "Moved";
    returnStrings[BAD_REQUEST] = "Bad request";
    returnStrings[UNAUTHORIZED] = "Unauthorized";
    returnStrings[FORBIDDEN] = "Forbidden";
    returnStrings[NOT_FOUND] = "Not found";
    returnStrings[SERVER_ERROR] = "Server error";
  }
  char rc[4];
  string r;

  // send continue if client sent expect continue -- not currently supported
//   if (vers == "1.1") {
//     r += "HTTP/" + vers + " ";
//     snprintf(rc, 4, "%d", CONTINUE);
//     r += string(rc) + " " + returnStrings[CONTINUE] + "\r\n\r\n";
//   }

  r += "HTTP/" + vers + " ";
  snprintf(rc, 4, "%d", c);
  r += string(rc) + " " + returnStrings[c] + "\r\n";

  return r;
} // getResponseHeader

string HttpResponse::getBodyHeader(string type, uint len, bool keepAlive,
				   bool chunked, const string& location) {
 // set date Date: Fri, 31 Dec 1999 23:59:59 GMT
  time_t t;
  char slen[15];
  time(&t);
  string s = asctime(gmtime(&t));
  string st;
  string weekday, day, month, Time, year;
  istringstream is(s);

  is >> weekday >> month >> day >> Time >> year;
  st = "Date: " + weekday + ", " + day + " " + month + " " + year + " " + Time + " GMT\n";
  if (!location.empty()) {
    st += "Location: " + location + "\r\n";
  }
  
  if (keepAlive) {
    st += "Connection: Keep-Alive\r\n";
  }
  else {
    st += "Connection: close\r\n";
  }

  st += "Content-Type: " + type + "\r\n";

  if (len > 0) {
    snprintf(slen, 15, "%u", len);
    st += "Content-Length: " + string(slen) + "\r\n";
  }
  if (chunked) {
    st += "Transfer-Encoding: chunked\r\n";
  }
  st += "\r\n";
  return st;
} // getBodyHeader

