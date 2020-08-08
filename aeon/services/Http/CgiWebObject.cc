/* 
 * CgiWebObject.cc : part of the Mace toolkit for building distributed systems
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

#include <signal.h>
#include <errno.h>
#include <boost/lexical_cast.hpp>

#include "Util.h"
#include "FileUtil.h"
#include "StrUtil.h"

#include "CgiWebObject.h"

using namespace std;
using boost::lexical_cast;

const size_t CgiWebObject::BLOCK_SIZE;

CgiWebObject::CgiWebObject(const string& p, const string& r, const string& pinfo,
			   const string& qs, const HttpRequestContext& req,
			   bool headRequest) :
  WebObject(headRequest, true), path(p), root(r), pathInfo(pinfo), queryString(qs),
  childpid(0), isOpen(true), parseHeader(true) {

  exec(req);
} // CgiWebObject

CgiWebObject::~CgiWebObject() {
  close();
  kill(childpid, SIGKILL);
} // ~CgiWebObject

void CgiWebObject::close() {
  isOpen = false;
} // close

pid_t CgiWebObject::getChildPid() const {
  return childpid;
} // getChildPid

void CgiWebObject::exec(const HttpRequestContext& req) {
  pipefdpair_t p2c;
  pipefdpair_t c2p;
  
  if (pipe(c2p) < 0) {
    Log::perror("pipe");
    ABORT("pipe");
  }

  if (pipe(p2c) < 0) {
    Log::perror("pipe");
    ABORT("pipe");
  }

  if ((childpid = fork()) < 0) {
    Log::perror("fork");
    ABORT("fork");
  }

  if (childpid == 0) {
    // child

    FileUtil::remapPipeStdinStdout(p2c, c2p);

    StringList env;

    if (!req.content.empty()) {
      env.push_back("CONTENT_LENGTH=" + lexical_cast<std::string>(req.content.size()));

      if (req.headers.containsKey("Content-Type")) {
	env.push_back("CONTENT_TYPE=" + req.headers.get("Content-Type"));
      }
    }

    env.push_back("DOCUMENT_ROOT=" + root);
    env.push_back("GATEWAY_INTERFACE=1.1");

    for (StringHMap::const_iterator i = req.headers.begin(); i != req.headers.end(); i++) {
      string k = StrUtil::toUpper(i->first);
      string v = i->second;
      if ((k == "CONTENT-TYPE") || (k == "CONTENT-LENGTH")) {
	continue;
      }

      k = StrUtil::replaceAll(k, "-", "_");
      k.insert(0, "HTTP_");
      env.push_back(k + "=" + v);
    }

//     int envi = 0;
//     char envpath[1024];
//     do {
//       if (memcmp(environ[envi], "PATH", 4) == 0) {
// 	strncpy(envpath, environ[envi], 1023);
// 	envpath[1023] = '\0';
// 	break;
//       }
//       envi++;
//     } while (environ[envi] != 0);
    
//     string envpath(getenv("PATH"));
//     env.push_back("PATH=" + envpath);
    env.push_back("PATH=/usr/local/bin:/bin:/usr/bin");
//     env.back().append(envpath, strlen(envpath));
    if (!pathInfo.empty()) {
      env.push_back("PATH_INFO=" + pathInfo);
      env.push_back("PATH_TRANSLATED=" + root + pathInfo);
    }
    env.push_back("QUERY_STRING=" + queryString);
    // fix these
    env.push_back("REMOTE_ADDR=");
    env.back().append(Util::getAddrString(req.remoteAddr));
    env.push_back("REMOTE_HOST=" + req.remoteHost);
    env.push_back("REMOTE_PORT=" + lexical_cast<std::string>(req.remotePort));
    env.push_back("REQUEST_METHOD=" + req.method);
    env.push_back("REQUEST_URI=" + req.path);
    env.push_back("SCRIPT_FILENAME=" + path);
    env.push_back("SCRIPT_NAME=" + path.substr(root.size()));
    env.push_back("SERVER_ADDR=");
    env.back().append(Util::getAddrString(req.serverAddr));
    env.push_back("SERVER_NAME=" + req.serverName);
    env.push_back("SERVER_PORT=" + lexical_cast<std::string>(req.serverPort));
    env.push_back("SERVER_PROTOCOL=HTTP/" + req.httpVersion); // HTTP/1.1
    env.push_back("SERVER_SIGNATURE=");
    env.push_back("SERVER_SOFTWARE=MaceHttpServer/1.0");

    const char* envp[env.size() + 1];

    StrUtil::toCStr(env, envp);
    envp[env.size()] = 0;

    // need to pass query string as argument for ISINDEX queries---not
    // currently supported

    if (execle(path.c_str(), path.c_str(), NULL, envp) < 0) {
      if (errno == ENOEXEC) {
        execle ("/bin/sh", "sh", "-c", path.c_str(), (char *)0, envp);
      }
      Log::perror("execle");
    }
    ABORT("execle");
  }
  else {
    // parent
    ::close(p2c[FileUtil::READ_INDEX]);
    ::close(c2p[FileUtil::WRITE_INDEX]);

    rpipe = c2p[FileUtil::READ_INDEX];
    wpipe = p2c[FileUtil::WRITE_INDEX];

    inputBuf = req.content;
    writeBlock();
  }
} // exec

void CgiWebObject::writeBlock() {
  size_t wlen = inputBuf.size();
  if (wlen == 0) {
    ::close(wpipe);
    inputBuf.clear();
    return;
  }

  if (wlen > BLOCK_SIZE) {
    wlen = BLOCK_SIZE;
  }

  int w = write(wpipe, inputBuf.data(), wlen);
  int err = 0;
  if (w < 0) {
    err = errno;
    if (err == EPIPE) {
      close();
      return;
    }
    Log::perror("write");
    ABORT("write");
  }

  if (((size_t)w == wlen) && ((size_t)w < BLOCK_SIZE)) {
    ::close(wpipe);
    inputBuf.clear();
  }
  else {
    inputBuf = inputBuf.substr(w);
  }
} // writeBlock

bool CgiWebObject::hasMoreChunks() {
  if (isChunkReady()) {
    return true;
  }
  
  if (!isOpen) {
    return false;
  }

  fd_set rset;
  fd_set wset;

  FD_ZERO(&rset);
  FD_ZERO(&wset);

  FD_SET(rpipe, &rset);
  if (!inputBuf.empty()) {
    FD_SET(wpipe, &wset);
  }

  int selectMax = max(rpipe, wpipe) + 1;

  struct timeval polltv = { 0, 0 };
  int n = select(selectMax, &rset, &wset, 0, &polltv);

  if (n < 0) {
    Log::perror("select");
    ABORT("select");
  }
  
  if (n == 0) {
    return true;
  }

  if (FD_ISSET(wpipe, &wset)) {
    writeBlock();
  }

  if (FD_ISSET(rpipe, &rset)) {
    char tmp[BLOCK_SIZE];
    int r = ::read(rpipe, tmp, BLOCK_SIZE);
    if (r < 0) {
      Log::perror("read");
      ABORT("read");
    }
    if (r == 0) {
      isOpen = false;
      if (parseHeader) {
        setContentType("text/plain");
        parseHeader = false;
        chunk = "ERROR! No Content-type Header in CGI Result";
      }
      return false;
    }

    chunk.append(tmp, r);

    if (parseHeader) {
      bool readBlank = false;
      size_t rl = 0;
      do {
	string s;
	rl = StrUtil::readLine(chunk, s);
	if (rl > 0) {
	  if (s.empty()) {
	    readBlank = true;
	  }
	  else {
	    StringList m = StrUtil::match("Content-type:\\s+(.*)", s, true);
	    if (!m.empty()) {
	      setContentType(m[0]);
	    }
	  }
	}
	
      } while (rl > 0 && !chunk.empty() && !readBlank);

      if (readBlank) {
	parseHeader = false;
      }
    }
  }
  
  return true;
} // hasMoreChunks

bool CgiWebObject::isChunkReady() {
  return !chunk.empty();
} // isChunkReady

string CgiWebObject::readChunk() throw (HttpServerException) {
  const string c = chunk;
  chunk.clear();
  return c;
} // readChunk

#endif
