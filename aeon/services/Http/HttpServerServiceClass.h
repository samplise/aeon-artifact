/* 
 * HttpServerServiceClass.h : part of the Mace toolkit for building distributed systems
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
#ifndef HTTP_SERVER_SERVICE_CLASS_H
#define HTTP_SERVER_SERVICE_CLASS_H

#include "NumberGen.h"

#include "ServiceClass.h"
#include "UrlHandler.h"

class HttpServerServiceClass : public virtual ServiceClass {
public:
  static HttpServerServiceClass& NULL_;

  virtual ~HttpServerServiceClass() { }

  virtual registration_uid_t registerHandler(UrlHandler& h, registration_uid_t rid = -1) {
    if (rid == -1) {
      rid = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal();
    }
    urlHandlers[rid] = &h;
    return rid;
  }

  void setUrlHandlerPath(const std::string& path, registration_uid_t rid) {
    if (urlHandlers.containsKey(rid)) {
      registerUrlHandler(path, urlHandlers[rid]);
      urlHandlers.erase(rid);
    }
  }

  virtual void registerUrlHandler(const std::string& path, UrlHandler* h) {
    assert(0);
  }

protected:
  mace::hash_map<registration_uid_t, UrlHandler*, mace::SoftState> urlHandlers;
  
}; // HttpServerServiceClass

#endif // HTTP_SERVER_SERVICE_CLASS_H
