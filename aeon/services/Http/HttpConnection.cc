/* 
 * HttpConnection.cc : part of the Mace toolkit for building distributed systems
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
#include <inttypes.h>
#include "Log.h"
#include "Util.h"
#include "Accumulator.h"

#include "HttpConnection.h"
#include "HttpServer.h"

#include "m_net.h"

#include "mace-macros.h"

#if !(defined _WINSOCK2_H || defined _WINSOCK_H)
#define SOCKOPT_VOID_CAST
#else
#define SOCKOPT_VOID_CAST (char*)
#define SHUT_RDWR SD_BOTH
#endif

using std::cerr;
using std::endl;
using std::string;

HttpConnection::HttpConnection(int s) :
  client(s), readable(true), writeable(false), open(true), keepAlive(false),
  readFromSocket(true), wobj(0), wqueued(0) {

  struct sockaddr_in sa;
  socklen_t slen = sizeof(sa);
  if (getpeername(client, (sockaddr*)&sa, &slen) < 0) {
    Log::perror("getpeername");
    ABORT("getpeername");
  }
  
  raddr = sa.sin_addr.s_addr;
  rport = sa.sin_port;

  opentime = TimeUtil::time();
  start = TimeUtil::timeu();
} // HttpConnection

HttpConnection::~HttpConnection() {
  close();
} // ~HttpConnection

int HttpConnection::getRemoteAddr() {
  return raddr;
} // getAddr

int HttpConnection::getRemotePort() {
  return rport;
} // getPort

void HttpConnection::setWebObject(WebObject *w) {
  ASSERT(wobj == 0);
  wobj = w;
  writeable = true;

  if (!w->isRaw()) {
    sendHeader();
  }
  queueData();
} // setWebObject

void HttpConnection::sendHeader() {
  ASSERT(open);
  writeable = true;
//   readable = false;
  wbuf = HttpResponse::getResponseHeader(HttpResponse::OK, httpVersion);
  wbuf += HttpResponse::getBodyHeader(wobj->getContentType(), wobj->getLength(),
				      keepAlive,
				      wobj->isChunked() && httpVersion == "1.1");
} // sendHeader

void HttpConnection::sendRedirect(int c, const string& location) {
  ASSERT(open);
  writeable = true;
  readable = false;
  wobj = new WebObject();
  wbuf = HttpResponse::getResponseHeader(c, httpVersion);
  wbuf += HttpResponse::getBodyHeader("text/html", 0, false, false, location);
  keepAlive = false;
} // sendRedirect

void HttpConnection::sendError(int c) {
  ASSERT(open);
  writeable = true;
  readable = false;
  wobj = new WebObject();
  wbuf = HttpResponse::getResponseHeader(c, "1.0");
  wbuf += "\r\n";
  keepAlive = false;
} // sendError

void HttpConnection::close(bool reset) {
  ADD_SELECTORS("HttpConnection::close");
  if (open) {
    if (reset) {
      maceout << "sending reset on " << client << Log::endl;
      // the connection timed-out or had an error --- send a RST
      struct linger ling;
      ling.l_onoff = 1;
      ling.l_linger = 0;
      if (setsockopt(client, SOL_SOCKET, SO_LINGER, SOCKOPT_VOID_CAST &ling, sizeof(ling)) < 0) {
	Log::perror("setsockopt");
      }
    }

    shutdown(client, SHUT_RDWR);
    ::close(client);
    if (wobj) {
      wobj->close();
      delete wobj;
      wobj = 0;
    }

    uint64_t end = TimeUtil::timeu();
    suseconds_t t = end - start;
    maceout << client << " open for " << (t / 1000) << " ms "
	    << "wbuf.size=" << wbuf.size()
	    << " raddr=" << Util::getAddrString(raddr) << ":" << ntohs(rport)
	    << Log::endl;

    open = false;

    rbuf.clear();
    wbuf.clear();
  }

  readable = false;
  writeable = false;
} // close

int HttpConnection::read(string& buf) {
  ASSERT(open);
  if (usingSocket()) {
    return readSocket(buf);
  }
  else {
    return readBuffer(buf);
  }
} // read

int HttpConnection::readSocket(string& buf) {
  ASSERT(open);
  char tmp[BLOCK_SIZE];
  int r = ::read(client, tmp, sizeof(tmp));
  if (r <= 0) {
//     Log::log("HttpConnection::readSocket") << "read " << r << " from " << client
// 					   << ", closing" << Log::endl;
    close();
    return r;
  }
  Accumulator::Instance(Accumulator::HTTP_SERVER_READ)->accumulate(r);

  rbuf.append(tmp, r);
  if (0) {
    // XXX - really need a way of enabling this if desired
    if (rbuf.size() > MAX_REQUEST_SIZE) {
      close();
      return 0;
    }
  }

  buf = rbuf;
  return r;
} // readSocket

int HttpConnection::readBuffer(string& buf) {
  ASSERT(open);
  buf = rbuf;
  return rbuf.size();
} // readBuffer

int HttpConnection::write() {
  ASSERT(open);
  if (wbuf.empty() && wobj && wobj->isChunked()) {
    queueData();
    if (wbuf.empty()) {
      ASSERT(wobj->hasMoreChunks());
      return 0;
    }
//change in conflict.  Keeping for prosperity
//if (!ret || wbuf.empty()) {                                             |      if (wbuf.empty()) {                                                     |      return 0;                                                              
//  return 0;                                                             |        ASSERT(wobj->hasMoreChunks());                                        |  ---------------------------------------------------------------------------
//}                                                                       |        return 0;                                                             |  ---------------------------------------------------------------------------

  }

  int r = ::write(client, wbuf.data(), wbuf.size());
  if (r <= 0) {
//     Log::log("HttpConnection::write") << "write to " << client
// 				      << " returned " << r << ", closing" << Log::endl;
    close();
    return r;
  }
  Accumulator::Instance(Accumulator::HTTP_SERVER_WRITE)->accumulate(r);

  wbuf = wbuf.substr(r);
//   Log::log("HttpConnection::write") << "wbuf.size=" << wbuf.size()
// 				    << " wqueued=" << wqueued
// 				    << " wobj.length=" << wobj->getLength() << Log::endl;
  if (wobj && ((wqueued < wobj->getLength()) || wobj->hasMoreChunks())) {
    queueData();
    return r;
  }

  if (!wbuf.empty()) {
    return r;
  }

  if (keepAlive) {
    // we have sent all the data, so try to read another request
    if (wobj) {
      wobj->close();
      delete wobj;
      wobj = 0;
    }
    wqueued = 0;
    readable = true;
    writeable = false;
    opentime = TimeUtil::time();
    return r;
  }

  // we have sent all the data, the connection should not be maintained

//   Log::log("HttpConnection::write") << "all data sent, closing " << client
// 				    << " keepAlive=" << keepAlive << Log::endl;
  close();
  return r;
} // write

void HttpConnection::advanceReadBuffer(uint pos) {
  ASSERT(pos <= rbuf.size());
  rbuf = rbuf.substr(pos);
} // advanceReadBuffer

bool HttpConnection::isWriteable() {
  if (writeable) {
//     fprintf(stderr, "isWriteable connection %d hasMoreChunks=%d isChunkReady=%d "
// 	    "wqueued=%d wbuf=%d\n",
// 	    client, wobj->hasMoreChunks(), wobj->isChunkReady(), wqueued, wbuf.size());
    if (!wbuf.empty()) {
      return true;
    }
    if (wobj->isChunked()) {
      // !hasMoreChunks is needed to send the termination sequence
      if (wobj->isChunkReady() || !wobj->hasMoreChunks()) {
	return true;
      }
    }
  }
  return false;
} // isWriteable

void HttpConnection::queueData() {
  ASSERT(wobj);
  try {

    if (wobj->isHeadRequest()) {
      wqueued = wobj->getLength();
      return;
    }
    if (wobj->isChunked()) {
      size_t req = 0;
//       fprintf(stderr, "connection %d hasMoreChunks=%d isChunkReady=%d\n", client,
// 	      wobj->hasMoreChunks(), wobj->isChunkReady());
      while (wobj->hasMoreChunks() && wobj->isChunkReady() && (req < BLOCK_SIZE)) {
	string chunk = wobj->readChunk();
	if (httpVersion == "1.1") {
	  char hex[32];
	  if (wqueued == 0) {
	    sprintf(hex, "%zx\r\n", chunk.size());
	  }
	  else {
	    sprintf(hex, "\r\n%zx\r\n", chunk.size());
	  }
	  wbuf.append(hex);
	  wqueued += strlen(hex);
	}
	wbuf.append(chunk);
	wqueued += chunk.size();
	req += chunk.size();
      }
      if (!wobj->hasMoreChunks() && (httpVersion == "1.1")) {
	if (wqueued != 0) {
	  wbuf.append("\r\n");
	  wqueued += 2;
	}
	
	const char* final = "0\r\n\r\n";
	wbuf.append(final);
	wqueued += strlen(final);
      }
      return;
    }
    ASSERT(wqueued < wobj->getLength());
    uint req = BLOCK_SIZE;
    if (wobj->getLength() - wqueued < req) {
      req = wobj->getLength() - wqueued;
    }
    char buf[BLOCK_SIZE] ;
    wobj->read(buf, req);
    wbuf.append(buf, req);
    wqueued += req;
//     Log::log("HttpConnection::queueData") << "queued " << wqueued << Log::endl;
  } catch (const HttpServerException& e) {
    Log::err() << "HttpConnection::queueData caught exception " << e << Log::endl;
    close();
    return;
  }

} // queueData
