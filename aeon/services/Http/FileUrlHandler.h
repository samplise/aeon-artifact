/* 
 * FileUrlHandler.h : part of the Mace toolkit for building distributed systems
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
#ifndef FILE_URL_HANDLER_H
#define FILE_URL_HANDLER_H

#include <fstream>
#include <sstream>
#include <iostream>

#include "mhash_map.h"
#include "hash_string.h"

#include "FileWebObject.h"
#include "UrlHandler.h"

enum HTACCESS { HT_NONE, HT_ALLOW, HT_DENY };

class FileUrlHandler : public UrlHandler {
  typedef mace::map<std::string, HTACCESS, mace::SoftState> HtStatusMap;
  struct htaddr {
    unsigned int addr;
    unsigned int length;
  };
  typedef mace::map<std::string, htaddr, mace::SoftState> HtAddrMap;

public:
  FileUrlHandler(std::string root);
  virtual ~FileUrlHandler();
  virtual bool requestWebObject(const std::string& url, const HttpRequestContext& req,
				uint64_t id, HttpConnection& c);

protected:
  virtual bool isRequestOK(const std::string& path, const std::string& url, HttpConnection& c,
			   int permissions);
  virtual bool validPermissions(const std::string& p, int permission);
  virtual bool validAuthorization(std::string p, int addr);

protected:
  std::string defaultIndex;
  std::string root;

private:
  HtStatusMap htStatus;
  HtAddrMap htAllow;
  HtAddrMap htDeny;
}; // FileUrlHandler

#endif // FILE_URL_HANDLER_H
