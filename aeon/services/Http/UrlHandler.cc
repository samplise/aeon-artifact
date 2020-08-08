/* 
 * UrlHandler.cc : part of the Mace toolkit for building distributed systems
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

#include "UrlHandler.h"
#include "massert.h"

using namespace std;

UrlHandler::UrlHandler() {
  pthread_mutexattr_t a;
  ASSERT(pthread_mutexattr_init(&a) == 0);
  ASSERT(pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE) == 0);
  ASSERT(pthread_mutex_init(&lock, &a) == 0);
  ASSERT(pthread_mutexattr_destroy(&a) == 0);
} // UrlHandler

UrlHandler::~UrlHandler() {
  for (WebObjectMap::iterator i = readyObjects.begin();
       i != readyObjects.end(); i++) {
    WebObjectResult& r = i->second;
    delete r.w;
  }
  ASSERT(pthread_mutex_destroy(&lock) == 0);
} // ~UrlHandler

void UrlHandler::getWebObject(uint64_t id, WebObject** w, bool& error, int& errorCode) {
  ScopedLock sl(lock);

  WebObjectMap::iterator i = readyObjects.find(id);
  ASSERT(i != readyObjects.end());

  struct WebObjectResult& r = i->second;

  ASSERT(id == r.id);

  *w = r.w;
  error = r.error;
  errorCode = r.errorCode;

  readyObjects.erase(id);
} // getWebObject

void UrlHandler::setWebObject(uint64_t id, WebObject* w, bool error, int errorCode) {
  ScopedLock sl(lock);

  if (discarded.contains(id)) {
    discarded.erase(id);
    return;
  }

  struct WebObjectResult r = { id, w, error, errorCode };

  readyObjects[id] = r;
} // setWebObject

bool UrlHandler::requestWebObject(const std::string& url, const HttpRequestContext& req, uint64_t id, HttpConnection& c) { ABORT("requestWebObject base class called, not overridden!"); }

void UrlHandler::discardWebObject(uint64_t id) {
  discarded.insert(id);
}
