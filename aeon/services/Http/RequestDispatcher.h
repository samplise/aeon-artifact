/* 
 * RequestDispatcher.h : part of the Mace toolkit for building distributed systems
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
#ifndef REQUEST_DISPATCHER_H
#define REQUEST_DISPATCHER_H

#include "mhash_map.h"
#include "hash_string.h"
#include "mdeque.h"
#include "Collections.h"

#include "HttpRequestContext.h"
#include "UrlHandler.h"
#include "HttpConnection.h"

class RequestDispatcher {
  typedef mace::map<std::string, UrlHandler*, mace::SoftState> UrlHandlerMap;
  typedef mace::map<uint, StringList, mace::SoftState, std::greater<uint> > UrlLengthMap;
  
public:
  RequestDispatcher() { }
  void addHandler(const std::string& path, UrlHandler* h);
  void pollWebObjects(const HttpConnection::ConnectionMap& connections);
  bool dispatch(const std::string& path, const HttpRequestContext& req, int s,
		HttpConnection& c);
  void clearWebObjects(int s);

private:
  UrlHandlerMap handlers;
  UrlLengthMap urls;

  typedef mace::map<uint64_t, UrlHandler*, mace::SoftState> RequestUrlHandlerMap;
  typedef mace::map<int, RequestUrlHandlerMap, mace::SoftState> ConnectionUrlHandlerMap;
  ConnectionUrlHandlerMap requests;

}; // RequestDispatcher

#endif // REQUEST_DISPATCHER_H
