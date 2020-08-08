/* 
 * XmlRpcUrlHandler.h : part of the Mace toolkit for building distributed systems
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
#ifndef XML_RPC_URL_HANDLER_H
#define XML_RPC_URL_HANDLER_H

#include <sstream>

#include "Collections.h"
#include "StrUtil.h"
#include "DynamicProcessingPool.h"

#include "Serializable.h"
#include "XmlRpcWebObject.h"
#include "XmlRpcHandler.h"
#include "UrlHandler.h"

class XmlRpcUrlHandler : public UrlHandler {
  struct XmlRpcUrlHandlerState {
    XmlRpcHandler* handler;
    StringSet* methods;
  };

  struct XmlRpcRequestState {
    uint64_t id;
    XmlRpcUrlHandlerState hs;
    std::string method;
    std::istringstream is;
    std::string result;
  };

  typedef mace::map<std::string, XmlRpcUrlHandlerState, mace::SoftState> XmlRpcHandlerMap;

  typedef DynamicProcessingPoolCallbacks::callback_copy<XmlRpcUrlHandler*, XmlRpcUrlHandler, XmlRpcRequestState*> XmlRpcUrlHandlerCallback;

  typedef DynamicProcessingPool<XmlRpcUrlHandlerCallback, XmlRpcRequestState*> XmlRpcRequestQueue;

public:
  XmlRpcUrlHandler();
  virtual ~XmlRpcUrlHandler();
  virtual void registerHandler(const std::string& className,
			       XmlRpcHandler* handler,
			       StringSet* methods = 0);
  virtual bool requestWebObject(const std::string& url, const HttpRequestContext& req,
				uint64_t id, HttpConnection& c);
  virtual void execute(XmlRpcRequestState* req);
  virtual void shutdown();

protected:
  void sendFault(HttpConnection& c, int code, const std::string& error,
		 XmlRpcRequestState* s);

private:
  bool isMethodRegistered(const std::string& className,
			  const std::string& methodName) const;
  // returns empty strings if not parsed
  StringPair parseMethodName(std::istringstream& s);

private:
  XmlRpcRequestQueue* requests;
  XmlRpcHandlerMap handlers;
  
}; // XmlRpcUrlHandler

#endif // XML_RPC_URL_HANDLER_H
