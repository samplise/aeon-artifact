/* 
 * HttpConnection.h : part of the Mace toolkit for building distributed systems
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
#ifndef HTTP_CONNECTION_H
#define HTTP_CONNECTION_H

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include "m_net.h"

#include <boost/shared_ptr.hpp>
#include <string>

#include "m_map.h"

#include "HttpResponse.h"
#include "WebObject.h"

class HttpConnection;

typedef boost::shared_ptr<HttpConnection> HttpConnectionPtr;

class HttpConnection {
public:
  typedef mace::map<int, HttpConnectionPtr, mace::SoftState> ConnectionMap;

public:
  HttpConnection(int s);
  virtual ~HttpConnection();
  bool isReadable() const { return readable; }
  bool isWriteable();
  bool isOpen() { return open; }
  int read(std::string& buf);
  int timeOpen() { return opentime; }
  void setHttpVersion(std::string v) { httpVersion = v; }
  void setKeepAlive() { keepAlive = true; }
  void clearKeepAlive() { keepAlive = false; }
  void sendError(int c);
  void sendRedirect(int c, const std::string& location);
  void setWebObject(WebObject *w);
  bool hasWebObject() const { return (wobj != 0); }
  void close(bool reset = false);
  int write();
  void advanceReadBuffer(uint pos);
  void useSocket() { readFromSocket = true; }
  void useBuffer() { readFromSocket = false; }
  bool usingSocket() const { return readFromSocket; }
  bool readBufferEmpty() const { return rbuf.empty(); }
  int sockfd() const { return client; }

  int getRemoteAddr();
  int getRemotePort();

protected:
  int readSocket(std::string& buf);
  int readBuffer(std::string& buf);
  void sendHeader();
  void queueData();

private:
  int client;
  bool readable;
  bool writeable;
  bool open;
  bool keepAlive;
  bool readFromSocket;
  WebObject *wobj;
  uint wqueued;
  int raddr;
  int rport;
  std::string httpVersion;
  std::string rbuf;
  std::string wbuf;
  time_t opentime;
  uint64_t start;
  static const size_t MAX_REQUEST_SIZE = 65536;
  static const size_t BLOCK_SIZE = 8192;

}; // HttpConnection

#endif // HTTP_CONNECTION_H
