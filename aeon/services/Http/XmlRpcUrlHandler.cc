/* 
 * XmlRpcUrlHandler.cc : part of the Mace toolkit for building distributed systems
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
#include "XmlRpcUrlHandler.h"
#include <iostream>

using namespace std;
using mace::SerializationUtil;

XmlRpcUrlHandler::XmlRpcUrlHandler() {
  requests = new XmlRpcRequestQueue(
    XmlRpcUrlHandlerCallback(this, &XmlRpcUrlHandler::execute), 2, 16);
} // XmlRpcUrlHandler

XmlRpcUrlHandler::~XmlRpcUrlHandler() {
  for (XmlRpcHandlerMap::iterator i = handlers.begin(); i != handlers.end(); i++) {
    struct XmlRpcUrlHandlerState s = i->second;
    delete s.handler;
    s.handler = 0;
    if (s.methods) {
      delete s.methods;
      s.methods = 0;
    }
  }
  delete requests;
  requests = 0;
} // ~XmlRpcUrlHandler

void XmlRpcUrlHandler::shutdown() {
  assert(requests);
  requests->halt();
} // shutdown

void XmlRpcUrlHandler::registerHandler(const string& className,
				       XmlRpcHandler* handler,
				       StringSet* methods) {
  struct XmlRpcUrlHandlerState& s = handlers[className];
  s.handler = handler;
  s.methods = methods;
} // registerHandler

void XmlRpcUrlHandler::execute(XmlRpcRequestState* req) {
  string r = req->hs.handler->execute(req->method, req->is);
  XmlRpcWebObject* w = new XmlRpcWebObject();
  w->setResult(r);
  uint64_t id = req->id;
  setWebObject(id, w);
  delete req;
} // execute

bool XmlRpcUrlHandler::requestWebObject(const string& url, const HttpRequestContext& req,
					uint64_t id, HttpConnection& c) {

  if (req.method != "POST") {
    c.sendError(HttpResponse::BAD_REQUEST);
    return false;
  }
  
  struct XmlRpcRequestState* rs = new XmlRpcRequestState();
  rs->is.str(req.content);

  StringPair p = parseMethodName(rs->is);

  if (p.first.empty() || p.second.empty()) {
    sendFault(c, HttpResponse::BAD_REQUEST, "BAD REQUEST", rs);
    return false;
  }
  else if (!isMethodRegistered(p.first, p.second)) {
    sendFault(c, HttpResponse::NOT_FOUND, "METHOD NOT FOUND", rs);
    return false;
  }

  struct XmlRpcUrlHandlerState& s = handlers[p.first];
  assert(s.handler);

  rs->id = id;
  rs->hs = s;
  rs->method = p.second;

  requests->execute(rs);
  return true;
} // requestWebObject

void XmlRpcUrlHandler::sendFault(HttpConnection& c, int code, const string& m,
				 XmlRpcRequestState* s) {
  XmlRpcWebObject* w = new XmlRpcWebObject();
  w->setFault(code, m);
  c.setWebObject(w);
  delete s;
  s = 0;
} // sendFault

bool XmlRpcUrlHandler::isMethodRegistered(const string& cname,
					  const string& mname) const {

  XmlRpcHandlerMap::const_iterator i = handlers.find(cname);
  if (i != handlers.end()) {
    const struct XmlRpcUrlHandlerState& s = i->second;
    return (s.handler->getMethods().contains(mname) && (s.methods == 0 || 
							s.methods->contains(mname)));
  }
  return false;
} // isMethodRegistered


StringPair XmlRpcUrlHandler::parseMethodName(istringstream& is) {
  string s = is.str();

  string::size_type i = s.find("?>");
  if ((s.find("<?xml") != 0) || (i == string::npos))  {
    return StringPair("", "");
  }
  s = s.substr(i + 2);
  s = StrUtil::trim(s);
  
  is.str(s);
  string call;
  try {
    SerializationUtil::expectTag(is, "<methodCall>");
    SerializationUtil::expectTag(is, "<methodName>");
    call = SerializationUtil::get(is, '<');
    SerializationUtil::expectTag(is, "</methodName>");
  } catch (const mace::SerializationException& e) {
    return StringPair("", "");
  }

  StringList m = StrUtil::match("(.*)\\.(.*)", call);
  if (m.empty()) {
    return StringPair("", "");
  }

  return StringPair(m[0], m[1]);
} // parseMethodName

