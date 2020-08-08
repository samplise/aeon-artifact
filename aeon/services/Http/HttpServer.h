/* 
 * HttpServer.h : part of the Mace toolkit for building distributed systems
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
#ifndef _HTTP_SERVER_H
#define _HTTP_SERVER_H

#include <sys/types.h>
#include <sys/time.h>
// #include <sys/stat.h>
#include <sys/types.h>
// #include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "m_net.h"
#include "HttpServerServiceClass.h"

#include "RequestDispatcher.h"
#include "HttpParser.h"
#include "HttpResponse.h"
#include "HttpConnection.h"
#include "WebObject.h"
#include "UrlHandler.h"
#include "FileUrlHandler.h"

class HttpServer : public HttpServerServiceClass {

public:
  HttpServer(uint16_t port, int bl = SOMAXCONN);
  virtual ~HttpServer();
  virtual void registerUrlHandler(const std::string& path, UrlHandler* h);
  virtual void run();
  virtual void shutdown();
  virtual void setDocumentRoot(const std::string& path);

protected:
  static void* startServerThread(void* arg);
  virtual void runServerThread();
  std::string canonicalizePath(const std::string& p);

private:
  void printProfilingResults();
  void setupSocket();
  void setNonblock(socket_t s);
  void handleAccept();
  void handleRead(HttpConnection& c, socket_t s);
  void handleWrite(HttpConnection& c);

public:
  static int writeCount;
  static int diskCount;
  static long long diskTime;
  static long long writeTime;
  static const bool profile;

private:
  uint16_t port;
  int backlog;
  bool running;

  std::string serverName;
  int serverAddr;

  static const int IDLE_TIMEOUT;
  static const int SELECT_TIMEOUT_MILLI;
  bool stopServer;
  socket_t serverSocket;
  std::string root;
  HttpConnection::ConnectionMap connections;
  RequestDispatcher dispatcher;
  pthread_t serverThread;

  // profiling variables
  unsigned int selectCount;
  unsigned int acceptCount;
  unsigned int parseCount;
  unsigned int readCount;
  long long connectionCount;
  long long parseTime;
  long long readTime;
  long long processTime;
  long long authTime;
  long long selectTime;
  unsigned long long bytesSent;
  timeval sentTimer;
}; // Server

#endif // _HTTP_SERVER_H
