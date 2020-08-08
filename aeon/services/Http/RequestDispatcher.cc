/* 
 * RequestDispatcher.cc : part of the Mace toolkit for building distributed systems
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
#include "RequestDispatcher.h"
#include "TimeUtil.h"

using namespace std;

void RequestDispatcher::addHandler(const string& path, UrlHandler* h) {
  handlers[path] = h;
  urls[path.size()].push_back(path);
} // addHandler

void RequestDispatcher::clearWebObjects(int s) {
  ConnectionUrlHandlerMap::const_iterator ri = requests.find(s);
  if (ri != requests.end()) {
    const RequestUrlHandlerMap& m = ri->second;
    RequestUrlHandlerMap::const_iterator mi = m.begin();
    while (mi != m.end()) {
      uint64_t id = mi->first;
      UrlHandler* h = mi->second;
      h->discardWebObject(id);
      mi++;
    }
  }
  requests.erase(s);
} // clearWebObjects

void RequestDispatcher::pollWebObjects(const HttpConnection::ConnectionMap& connections) {
  for (HttpConnection::ConnectionMap::const_iterator i = connections.begin();
       i != connections.end(); i++) {
    int s = i->first;
    ConnectionUrlHandlerMap::iterator ri = requests.find(s);
    if (ri != requests.end()) {
      RequestUrlHandlerMap& m = ri->second;
      assert(!m.empty());
      RequestUrlHandlerMap::iterator mi = m.begin();
      uint64_t id = mi->first;
      UrlHandler* h = mi->second;
      if (h->hasWebObject(id)) {
	WebObject* w;
	bool error;
	int errorCode;
	h->getWebObject(id, &w, error, errorCode);
	HttpConnectionPtr c = i->second;
	if (error) {
	  c->sendError(errorCode);
	}
	else {
	  c->setWebObject(w);
	}
	m.erase(id);
	if (m.empty()) {
	  requests.erase(s);
	}
      }
    }
  }
} // pollWebObjects

bool RequestDispatcher::dispatch(const string& url, const HttpRequestContext& req, int s,
				 HttpConnection& c) {
  // need to do a longest prefix match on path
  bool match = false;
  string p;
  for (UrlLengthMap::const_iterator i = urls.begin(); i != urls.end() && !match; i++) {
    if (i->first > url.size()) {
      continue;
    }
    
    for (StringList::const_iterator li = i->second.begin();
	 li != i->second.end() && !match; li++) {
      if (url.find(*li) == 0) {
	match = true;
	p = *li;
      }
    }
  }

  if (match) {
    UrlHandlerMap::iterator i = handlers.find(p);
    assert(i != handlers.end());
    UrlHandler* h = i->second;
    uint64_t now = TimeUtil::timeu();
    if (h->requestWebObject(url, req, now, c)) {
      requests[s][now] = h;
    }
  }
  
  return match;
} // dispatch

