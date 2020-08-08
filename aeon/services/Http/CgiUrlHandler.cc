/* 
 * CgiUrlHandler.cc : part of the Mace toolkit for building distributed systems
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
#include "stdint.h"
#include "maceConfig.h"

#ifdef INCLUDE_CGI
#include "m_wait.h"

#include "FileUtil.h"
#include "CgiUrlHandler.h"

using namespace std;

CgiUrlHandler::CgiUrlHandler(string path) : FileUrlHandler(path) {

  defaultIndex = "index.cgi";
  reaper = new CgiReapQueue(CgiUrlHandlerCallback(this, &CgiUrlHandler::reapChild));
} // CgiUrlHandler

CgiUrlHandler::~CgiUrlHandler() {
  delete reaper;
  reaper = 0;
} // ~CgiUrlHandler

bool CgiUrlHandler::hasWebObject(uint64_t id) {
  ScopedLock sl(lock);

  CgiWebObjectMap::iterator i = wobjs.find(id);
  if (i != wobjs.end()) {
    CgiWebObject* w = i->second;
    if (w->hasMoreChunks() && w->isChunkReady()) {
      setWebObject(id, w);
      wobjs.erase(id);
    }
  }
  return UrlHandler::hasWebObject(id);
} // hasWebObjects

bool CgiUrlHandler::requestWebObject(const string& url, const HttpRequestContext& req,
				     uint64_t id, HttpConnection& c) {
  reaper->clear();

  string queryString;
  size_t qsi = url.find('?');
  if (qsi != string::npos) {
    queryString = url.substr(qsi + 1);
  }

  string scriptName;
  size_t sni = 0;
  do {
    sni++;
    sni = url.find('/', sni);
    if (sni == string::npos) {
      if (qsi != string::npos) {
	scriptName = url.substr(0, qsi);
      }
      else {
	scriptName = url;
      }

      break;
    }

    scriptName = url.substr(0, sni);

    if (FileUtil::fileIsReg(root + scriptName)) {
      break;
    }
  } while (1);

  string pathInfo;
  if (sni != string::npos) {
    if (qsi != string::npos) {
      assert(qsi > sni);
      pathInfo = url.substr(sni, qsi - sni);
    }
    else {
      pathInfo = url.substr(sni);
    }
  }
  
  string path = root + scriptName;

  if (!isRequestOK(path, url, c, R_OK | X_OK)) {
    return false;
  }

  CgiWebObject* w = 0;
  try {
    w = new CgiWebObject(path, root, pathInfo, queryString, req, req.method == "HEAD");
  }
  catch (const HttpServerException& e) {
    c.sendError(HttpResponse::SERVER_ERROR);
    return false;
  }

  reaper->push(w->getChildPid());

  ScopedLock sl(lock);
  wobjs[id] = w;
  return true;
} // requestWebObject

int CgiUrlHandler::reapChild(pid_t c) {
  int status;
# ifdef HAVE_WAITPID
  ASSERT(c == waitpid(c, &status, 0));
# else
#   ifdef HAVE__CWAIT
      ASSERT(_cwait(&status, c, 0));
#   endif
# endif
//   cerr << "reaped child " << c << " with status " << WEXITSTATUS(status) << endl;

  return status;
} // reapChild

#endif
