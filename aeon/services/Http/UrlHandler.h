/* 
 * UrlHandler.h : part of the Mace toolkit for building distributed systems
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
#ifndef URL_HANDLER_H
#define URL_HANDLER_H

#include <cassert>

#include "ScopedLock.h"
#include "mdeque.h"
#include "mhash_map.h"

#include "HttpRequestContext.h"
#include "HttpResponse.h"
#include "WebObject.h"
#include "HttpConnection.h"

class UrlHandler {
public:
  static UrlHandler& NULL_;
  UrlHandler();
  virtual ~UrlHandler();
  virtual bool requestWebObject(const std::string& url, const HttpRequestContext& req,
				uint64_t id, HttpConnection& c);
  virtual bool hasWebObject(uint64_t id) {
    ScopedLock sl(lock);
    return readyObjects.containsKey(id);
  }

  virtual void getWebObject(uint64_t id, WebObject** w, bool& error, int& errorCode);
  virtual void discardWebObject(uint64_t id);

protected:
  virtual void setWebObject(uint64_t id, WebObject* w, bool error = false, int errorCode = 0);
  void slock() { assert(pthread_mutex_lock(&lock) == 0); }
  void sunlock() { assert(pthread_mutex_unlock(&lock) == 0); }

  struct WebObjectResult {
    uint64_t id;
    WebObject* w;
    bool error;
    int errorCode;
  };

protected:
  pthread_mutex_t lock;

private:
  typedef mace::map<uint64_t, WebObjectResult, mace::SoftState> WebObjectMap;

  WebObjectMap readyObjects;

  typedef mace::set<uint64_t> IdSet;
  IdSet discarded;
}; // UrlHandler

#endif // URL_HANDLER_H
