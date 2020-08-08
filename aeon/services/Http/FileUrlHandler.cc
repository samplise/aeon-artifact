/* 
 * FileUrlHandler.cc : part of the Mace toolkit for building distributed systems
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
#include "FileUrlHandler.h"
#include "FileUtil.h"

using namespace std;

FileUrlHandler::FileUrlHandler(string path) : defaultIndex("index.html") {
  while (path[path.size() - 1] == '/') {
    path = path.substr(0, path.size() - 1);
  }

  root = path;
  
  struct stat sbuf;
  try {
    FileUtil::statWithErr(root, sbuf);
  }
  catch (const FileException& e) {
    fprintf(stderr, "error: directory %s does not exist\n", root.c_str());
    exit(-1);
  }

  if (!S_ISDIR(sbuf.st_mode)) {
    fprintf(stderr, "error: %s is not a directory\n", root.c_str());
    exit(-1);
  }
} // FileUrlHandler

FileUrlHandler::~FileUrlHandler() {

} // ~FileUrlHandler

bool FileUrlHandler::isRequestOK(const string& path, const string& url, HttpConnection& c,
				 int permissions) {
  // check authorization
  if (!validAuthorization(path, c.getRemoteAddr())) {
    c.sendError(HttpResponse::UNAUTHORIZED);
    return false;
  }

  if (FileUtil::fileIsDir(path)) {
    string nurl = url;
    if (nurl[nurl.size() - 1] != '/') {
      nurl += "/";
    }
    c.sendRedirect(HttpResponse::MOVED_PERM, nurl + defaultIndex);
    return false;
  }

  int errorCode = 0;
  FileUtil::AccessResult p;
  try {
    p = FileUtil::canAccess(path, permissions);
  }
  catch (const FileException& e) {
    errorCode = HttpResponse::NOT_FOUND;
  }

  if (!p.access && p.exists) {
    errorCode = HttpResponse::FORBIDDEN;
  }
  else if (!p.exists) {
    errorCode = HttpResponse::NOT_FOUND;
  }

  if (errorCode) {
    c.sendError(errorCode);
    return false;
  }

  return true;
} // isRequestOK

bool FileUrlHandler::requestWebObject(const string& url, const HttpRequestContext& req,
				      uint64_t id, HttpConnection& c) {
  string path = root + url;

  if (!isRequestOK(path, url, c, R_OK)) {
    return false;
  }

  FileWebObject* w = 0;
  try {
    w = new FileWebObject(path, req.method == "HEAD");
  }
  catch (const HttpServerException& e) {
    c.sendError(HttpResponse::SERVER_ERROR);
    return false;
  }
  setWebObject(id, w);
  return true;
} // requestWebObject

bool FileUrlHandler::validPermissions(const string& p, int permissions) {
  struct stat sbuf;
  try {
    FileUtil::statWithErr(p, sbuf);
  }
  catch (const PermissionAccessException& e) {
    return false;
  }
  catch (const FileNotFoundException& e) {
    return true;
  }
  catch (const InvalidPathException& e) {
    return true;
  }
  catch (const FileException& e) {
    return true;
  }

  // require global read permission on file
  if (!(sbuf.st_mode & permissions)) {
    return false;
  }

  return true;
} // validPermissions

bool FileUrlHandler::validAuthorization(string p, int caddr) {
  if (htStatus.find(p) == htStatus.end()) {
    string f = p.substr(0, p.find_last_of("/"));
    if (p.find_last_of("/") == string::npos) {
      f = ".htaccess";
    }
    else {
      f += "/.htaccess";
    }
    if (FileUtil::fileIsReg(f)) {
      fstream fp(f.c_str(), fstream::in);
      string auth;
      string addrfull;
      fp >> auth >> addrfull;
      for (size_t i = 0; i < auth.size(); i++) {
	auth[i] = tolower(auth[i]);
      }

      htaddr htd;
      string addr = addrfull.substr(0, addrfull.find("/"));
      string mask = addrfull.substr(addrfull.find("/") + 1);

      htd.addr = inet_addr(addr.c_str());
      htd.length = atoi(mask.c_str());

      if (auth == "allow") {
	htAllow[p] = htd;
	htStatus[p] = HT_ALLOW;
      }
      else if (auth == "deny") {
	htDeny[p] = htd;
	htStatus[p] = HT_DENY;
      }
      else {
	htStatus[p] = HT_NONE;
      }
    }
    else {
      htStatus[p] = HT_NONE;
    }
  }

  if (htStatus[p] == HT_NONE) {
    return true;
  }

  htaddr pd;
  if (htStatus[p] == HT_ALLOW) {
    assert(htAllow.find(p) != htAllow.end());
    pd = htAllow[p];
    return (((ntohl(caddr) ^ ntohl(pd.addr)) >> (32 - pd.length)) == 0);
  }
  else {
    assert(htStatus[p] == HT_DENY);
    assert(htDeny.find(p) != htAllow.end());
    pd = htDeny[p];
    return (((ntohl(caddr) ^ ntohl(pd.addr)) >> (32 - pd.length)) != 0);
  }
} // validAuthorization

