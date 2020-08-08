/* 
 * HttpParser.cc : part of the Mace toolkit for building distributed systems
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
#include <iostream>
#include <sstream>
#include <ctype.h>
#include <boost/lexical_cast.hpp>

#include "StrUtil.h"

#include "HttpParser.h"

using namespace std;
using boost::lexical_cast;

// XXX - always set type to POST for responses


bool HttpParser::readHeader(string& request, StringList& req, size_t& lengthUsed) {
  bool readBlank = false;
  size_t r = 0;
  req.clear();
  do {
//     cerr << ".";
    string buf;
    r = StrUtil::readLine(request, buf);
    if (r > 0) {
      if (buf.empty()) {
	readBlank = true;
      }
      else {
	req.push_back(buf);
      }
      lengthUsed += r;
    }
  } while (r > 0 && !request.empty() && !readBlank);

  return readBlank;
} // readHeader

bool HttpParser::parseHeaders(StringList& req, HttpRequestContext& context,
			      size_t& contentLength) {
  while (!req.empty()) {
//     cerr << "*";
    string buf = req.front();
    req.pop_front();
    StringList m = StrUtil::match("(\\S+):\\s+(.*)", buf);
    if (m.empty()) {
      // bad request
      return false;
    }

    string name = m[0];
    string value = m[1];
    context.headers[name] = value;

    string ln = StrUtil::toLower(name);
    if (ln == "connection") {
      string lv = StrUtil::toLower(value);
      if (lv == "close") {
	context.keepAlive = false;
      }
      else if (lv == "keep-alive") {
	context.keepAlive = true;
      }
    }
    else if (ln == "content-length") {
      long l;
      try {
        l = lexical_cast<long>(value);
      } catch(exception e) {
        return false;
      }
      if ((context.method != "POST") || (l < 0)) {
	// bad request
	return false;
      }
      contentLength = (size_t)l;
    }
    else if (ln == "transfer-encoding") {
      if (StrUtil::toLower(value) == "chunked") {
	context.chunked = true;
      }
    }
    // add other headers here
  }

  // headers are good
  return true;
} // parseHeaders

void HttpParser::readChunked(string& request, string& content,
			     size_t& lengthUsed, bool& valid, bool& done) {
  do {
    string buf;
    string sizeBuf;
    size_t sizeRead = 0;
    do {
      // there might be a leading newline, which we should discard
      sizeRead = StrUtil::readLine(request, sizeBuf);
      if ((sizeRead > 0) && sizeBuf.empty()) {
	lengthUsed += sizeRead;
      }
    } while ((sizeRead > 0) && sizeBuf.empty());

    if (sizeRead == 0) {
      // not done
      return;
    }
    lengthUsed += sizeRead;
    int size = 0;
    int success = sscanf(sizeBuf.c_str(), "%x", &size);
    if (!success) {
      // bad request
      done = true;
      return;
    }

    if (size == 0) {
      // try to read a newline
      size_t r = StrUtil::readLine(request, buf);
      lengthUsed += r;
      if ((r > 0) && buf.empty()) {
	done = true;
	valid = true;
	return;
      }
      else if (r == 0) {
	// not done - need to add buf, with the last length, back onto request
	request.insert(0, sizeBuf + "\r\n");
	lengthUsed -= sizeRead;
	return;
      }
    }
    if (StrUtil::read(request, buf, (size_t)size) == (size_t)size) {
      lengthUsed += size;
      content += buf;
    }
    else {
      // not done - need to add buf, with the last length, back onto request
      request.insert(0, sizeBuf + "\r\n");
      lengthUsed -= sizeRead;
      return;
    }
  } while (1);
} // readChunked

bool HttpParser::readContent(string& request, HttpRequestContext& context,
			     size_t contentLength, size_t& lengthUsed,
			     bool& valid, bool& done) {

  if (context.chunked) {
    readChunked(request, context.content, lengthUsed, valid, done);
    return true;
  }

  if (contentLength == 0) {
    valid = true;
    done = true;
    return true;
  }

  if (StrUtil::read(request, context.content, contentLength) == contentLength) {
    lengthUsed += contentLength;
    valid = true;
    done = true;
    return true;
  }

  // could not read all the content
  return false;
} // readContent

bool HttpParser::parseResponseHeaderLine(StringList& req, HttpResponse& response) {
  if (req.empty()) {
//     std::cerr << "req empty" << std::endl;
    return false;
  }
  string buf = req.front();
  req.pop_front();
  long responseCode;
  StringList m = StrUtil::match("^HTTP/(\\d\\.\\d)\\s+(\\d{3})\\s*(.*)?", buf);
  if (m.empty() ||
      ((m[0] != "1.0") && (m[0] != "1.1"))) {
    // bad response
//     std::cerr << "bad response - HTTP: " << buf << std::endl;
    return false;
  }
  try {
    responseCode = lexical_cast<long>(m[1]);
  } catch (exception e) {
//     std::cerr << "bad response code: " << m[1] << std::endl;
    return false;
  }

  response.responseCode = responseCode;
  response.httpVersion = m[0];
  response.responseMessage = m[2];
  response.keepAlive = (response.httpVersion == "1.0" ? false : true);
  return true;
} // parseResponseHeaderLine

bool HttpParser::parseRequestHeaderLine(StringList& req, HttpRequestContext& context) {
  if (req.empty()) {
    return false;
  }
  string buf = req.front();
  req.pop_front();
  StringList m = StrUtil::match("^(GET|POST|HEAD)\\s+(\\S+)\\s+HTTP/(\\d\\.\\d)", buf);
  if (m.empty() ||
      !validPath(m[1]) ||
      ((m[2] != "1.0") && (m[2] != "1.1"))) {
    // bad request
    return false;
  }

  context.method = m[0];
  context.url = m[1];
  context.httpVersion = m[2];
  context.keepAlive = (context.httpVersion == "1.0" ? false : true);
  return true;
} // parseRequestHeaderLine

void HttpParser::parseRequest(string request, HttpRequestContext& context,
			      size_t& lengthUsed, bool& valid, bool& done) {

  StringList req;
  lengthUsed = 0;
  done = false;
  valid = false;

  if (!readHeader(request, req, lengthUsed)) {
    // not done
    return;
  }

  if (!parseRequestHeaderLine(req, context)) {
    // bad request
    done = true;
    return;
  }

  size_t contentLength = 0;
  bool headerOK = parseHeaders(req, context, contentLength);
  if (context.keepAlive && context.httpVersion == "1.0") {
    context.keepAlive = false;
  }

  if (context.keepAlive && context.httpVersion == "1.0") {
    context.keepAlive = false;
  }

  if (!headerOK ||
      ((context.httpVersion == "1.1") && !context.headers.containsKey("Host"))) {
    done = true;
    return;
  }

  if (readContent(request, context, contentLength, lengthUsed, valid, done)) {
    return;
  }

  // request is not complete, done is false
  assert(!done);
} // parseRequest

bool HttpParser::validPath(string p) {
  if (p[0] == '/') {
    return true;
  }
  if (p.find("http://") == 0) {
    return (p.find('/', 8) != string::npos);
  }
  return false;
} // validPath

