/* 
 * HttpServer.cc : part of the Mace toolkit for building distributed systems
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
#include "HttpServer.h"
#include "ThreadCreate.h"
#include "Util.h"
#include "StrUtil.h"
#include "SysUtil.h"
#include "SockUtil.h"
#include "TimeUtil.h"
#include <signal.h>

using namespace std;

const bool HttpServer::profile = false;
int HttpServer::writeCount = 0;
long long HttpServer::writeTime = 0;
int HttpServer::diskCount = 0;
long long HttpServer::diskTime = 0;
// const int HttpServer::IDLE_TIMEOUT = 90; // time (seconds) to keep connections open
const int HttpServer::IDLE_TIMEOUT = 0; // time (seconds) to keep connections open
const int HttpServer::SELECT_TIMEOUT_MILLI = 1;

HttpServer::HttpServer(uint16_t p, int bl) : port(p), backlog(bl), running(false) {

  serverAddr = Util::getAddr();
  serverName = Util::getHostByAddr(serverAddr);

  // initialize profiling variables
  parseCount = 0;
  parseTime = 0;
  readTime = 0;
  bytesSent = 0;
  authTime = 0;
  processTime = 0;
  selectTime = 0;
  readCount = 0;
} // HttpServer

HttpServer::~HttpServer() {
  shutdown();
} // ~HttpServer

void HttpServer::shutdown() {
  ADD_SELECTORS("HttpServer::shutdown");
  maceout << "httpd server shutting down" << Log::endl;
  stopServer = true;
} // shutdown

void HttpServer::run() {
  if (running) {
    return;
  }
  stopServer = false;
  runNewThread(&serverThread, HttpServer::startServerThread, this, 0);
} // run

void HttpServer::registerUrlHandler(const string& path, UrlHandler* h) {
  dispatcher.addHandler(path, h);
} // registerUrlHandler

void HttpServer::setDocumentRoot(const string& path) {
  registerUrlHandler("/", new FileUrlHandler(path));
} // setDocumentRoot

////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////

void* HttpServer::startServerThread(void* arg) {
  HttpServer* server = (HttpServer*)arg;
  server->runServerThread();
  return 0;
} // startServerThread

void HttpServer::runServerThread() {
  ADD_SELECTORS("HttpServer::runServerThread");
  #ifndef NO_SIGPIPE
  SysUtil::signal(SIGPIPE, SIG_IGN);
  #endif

  setupSocket();

  assert(serverSocket);
  if (listen(serverSocket, backlog) < 0) {
    Log::perror("listen");
    assert(false);
  }

  fd_set rset;
  fd_set wset;
  FD_ZERO(&rset);

  selectCount = 0;
  acceptCount = 0;

  HttpConnection::ConnectionMap unwriteableConnections;
  while (!stopServer) {
    // XXX -- gracefully close all connections
    FD_ZERO(&rset);
    FD_ZERO(&wset);

    int n = 0;

    int acceptedCount = 0;
    if (1) {
      do {
	// accept loop
      
//       FD_CLR(serverSocket, &rset);
	FD_SET(serverSocket, &rset);

//       cerr << "waiting for accept select\n";
	struct timeval polltv = { 0, 0 };
	n = select(serverSocket + 1, &rset, 0, 0, &polltv);

	if (n < 0) {
	  Log::perror("select");
	  assert(0);
	}
	else if (n > 0 && FD_ISSET(serverSocket, &rset)) {
	  handleAccept();
	  acceptedCount++;
	}
      } while (!stopServer && (n > 0));

      if (stopServer) {
	continue;
      }

      if (acceptedCount) {
	acceptCount += acceptedCount;
//       cout << "accepted " << acceptCount << " new client(s), "
// 	   << acceptTotal << " total\n";
      }
    }

    // check for ready web objects
    
    int now = time(0);

    //     bool bufferedRead = false;
    
    FD_SET(serverSocket, &rset);
    socket_t selectMax = serverSocket + 1;

    unwriteableConnections.clear();

    for (HttpConnection::ConnectionMap::const_iterator i = connections.begin();
	 i != connections.end(); i++) {
      socket_t s = i->first;
      if (s > selectMax) {
	selectMax = s;
      }
      HttpConnectionPtr c = i->second;
      
      if (c->isReadable() && c->usingSocket()) {
	if (IDLE_TIMEOUT && (now - c->timeOpen() > IDLE_TIMEOUT)) {
// 	  cerr << "closing idle connection " << s << endl;
	  c->close(true);
	  continue;
	}

// 	cerr << "adding " << s << " to read set\n";
	FD_SET(s, &rset);
      }
      else {
        // 	bufferedRead = true;
      }

      if (c->isWriteable()) {
// 	cerr << "adding " << s << " to write set\n";
	FD_SET(s, &wset);
      }
      else {
	if (c->usingSocket() && !c->hasWebObject()) {
	  unwriteableConnections[s] = c;
	}
      }
    }
    selectMax++;

    if (!unwriteableConnections.empty()) {
      dispatcher.pollWebObjects(unwriteableConnections);

      for (HttpConnection::ConnectionMap::const_iterator i = unwriteableConnections.begin();
	   i != unwriteableConnections.end(); i++) {

	socket_t s = i->first;
	if (i->second->isWriteable()) {
// 	cerr << "adding " << s << " to write set\n";
	  FD_SET(s, &wset);
	}
      }
    }

    timeval selstart;
    timeval selend;
    if (profile) {
      gettimeofday(&selstart, 0);
    }

    n = 0;
    struct timeval polltv = { 0, SELECT_TIMEOUT_MILLI * 1000 };
    n = SysUtil::select(selectMax, &rset, &wset, 0, &polltv);

    if (profile) {
      gettimeofday(&selend, 0);

      if (selectCount > 0) {
	selectTime += TimeUtil::timediff(selstart, selend);
      }
    }

    if (n < 0) {
      Log::perror("select");
      assert(0);
    }
    else if (n == 0) {
      FD_ZERO(&rset);
      FD_ZERO(&wset);
    }

    selectCount++;

    int closed[FD_SETSIZE];
    int closedCount = 0;

    if (FD_ISSET(serverSocket, &rset)) {
      handleAccept();
      acceptedCount++;
    }

    for (HttpConnection::ConnectionMap::const_iterator i = connections.begin();
	 i != connections.end(); i++) {
      socket_t s = i->first;
      HttpConnectionPtr c = i->second;

//       printf("%d isopen=%d isreadable=%d iswriteable=%d fd_isset=%d usingSocket=%d\n",
// 	     s, c->isOpen(), c->isReadable(), c->isWriteable(), FD_ISSET(s, &rset),
// 	     c->usingSocket());
      if (c->isOpen() && c->isReadable() &&
	  (FD_ISSET(s, &rset) || !c->usingSocket())) {

	handleRead(*c, s);
      }

      if (c->isOpen() && FD_ISSET(s, &wset)) {
	handleWrite(*c);
      }
	
      if (!c->isOpen()) {
	closed[closedCount] = s;
	closedCount++;
      }
    }
    
    for (int i = 0; i < closedCount; i++) {
      dispatcher.clearWebObjects(closed[i]);
      connections.erase(closed[i]);
    }

    if (connections.empty()) {
      if (profile) {
	printProfilingResults();
      }
    }
  }

  for (HttpConnection::ConnectionMap::const_iterator i = connections.begin();
       i != connections.end(); i++) {
    HttpConnectionPtr c = i->second;
    c->close();
  }

  connections.clear();
  maceout << "httpd server halting" << Log::endl;
} // runServerThread

////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////

void HttpServer::printProfilingResults() {
  if (1) {
    timeval now;
    gettimeofday(&now, 0);
    long long sentTime = TimeUtil::timediff(sentTimer, now);

    cout << "total requests " << parseCount << " total time " << sentTime << endl;
    cout << "total select calls " << selectCount << endl;
    cout << "total read calls " << readCount << endl;
    cout << "total write calls " << writeCount << endl;
    cout << "total disk reads " << diskCount << endl;
      
    cout << "total select time " << selectTime << " avg "
	 << selectTime / selectCount << endl;
    cout << "total parse time " << parseTime << " avg "
	 << parseTime / parseCount << endl;
    cout << "total auth time " << authTime << " avg "
	 << authTime / parseCount << endl;
    cout << "total disk time " << diskTime << " avg " << diskTime / diskCount << endl;
    cout << "total read time " << readTime << " avg " << readTime / readCount << endl;
    cout << "total write time " << writeTime << " avg "
	 << writeTime / writeCount << endl;

    printf("total bytes sent %lld throughput %.3f MB/s\n", bytesSent,
	   bytesSent / (sentTime / 1000000.0) / 1000000);
    printf("%.3f requests/sec\n", (double)parseCount / (sentTime / 1000000.0));

    long long totalProf = ((selectTime / selectCount) +
			   (parseTime / parseCount) +
			   (authTime / parseCount) +
			   (diskTime / diskCount) +
			   (readTime / readCount) +
			   (writeTime / writeCount));
    cout << "avg sum profiling times " << totalProf << endl;

    selectCount = 0;
    parseCount = 0;
    readCount = 0;
    writeCount = 0;
    diskCount = 0;
    selectTime = 0;
    parseTime = 0;
    diskTime = 0;
    readTime = 0;
    writeTime = 0;
    bytesSent = 0;
    authTime = 0;
    processTime = 0;
    acceptCount = 0;
  }
  else {
    // summary profiling results disabled
    timeval now;
    gettimeofday(&now, 0);
    long long sentTime = TimeUtil::timediff(sentTimer, now);

    printf("total bytes sent %lld in %lld throughput %.3f MB/s\n", bytesSent,
	   sentTime, bytesSent / (sentTime / 1000000.0) / 1000000);
    printf("%.3f requests/sec (%d requests total)\n",
	   (double)parseCount / (sentTime / 1000000.0), parseCount);

    bytesSent = 0;
    parseCount = 0;
    acceptCount = 0;
  }
} // printProfilingResults

void HttpServer::setupSocket() {
  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket < 0) {
    Log::perror("socket");
    assert(false);
  }

  int n = 1;
  if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0) {
    Log::perror("setsockopt");
    assert(false);
  }

  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_port = htons(port);
  std::string bstr = params::get<std::string>("HTTP_BIND_ADDRESS", "ANY");
  if (bstr == "LOOPBACK") {
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  } else if (bstr == "ANY") {
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
  } else {
    sa.sin_addr.s_addr = Util::getAddr(bstr);
  }
  if (bind(serverSocket, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
    Log::perror("bind");
    close(serverSocket);
    assert(false);
  }

  setNonblock(serverSocket);
} // setupSocket

void HttpServer::setNonblock(socket_t s) {
  SockUtil::setNonblock(s);
} // setNonblock

void HttpServer::handleAccept() {
  sockaddr_in sin;
  socklen_t sinlen = sizeof(sin);
  memset(&sin, 0, sinlen); 

  socket_t s;
  if ((s = accept(serverSocket, (sockaddr*) &sin, &sinlen)) < 0) {
    if (SockUtil::errorWouldBlock() || SockUtil::errorConnFail()) {
      return;
    }
    Log::perror("accept");
    assert(false);
  }

  setNonblock(s);
  HttpConnectionPtr c = HttpConnectionPtr(new HttpConnection(s));
  connections[s] = c;
} // handleAccept

void HttpServer::handleRead(HttpConnection& c, socket_t s) {
  string buf;
  timeval stv;
  timeval etv;

  int rv = 0;
  if (profile) {
    gettimeofday(&stv, 0);
  }
  rv = c.read(buf);
  if (profile) {
    gettimeofday(&etv, 0);
    readTime += TimeUtil::timediff(stv, etv);
  }
  if (rv <= 0) {
    return;
  }
  readCount++;

  assert(rv > 0);

  if (profile) {
    gettimeofday(&stv, 0);
  }

  HttpRequestContext req;
  size_t lengthUsed;
  bool isParseValid;
  bool isParseDone;

  HttpParser::parseRequest(buf, req, lengthUsed, isParseValid, isParseDone);
  if (profile) {
    gettimeofday(&etv, 0);
    parseTime += TimeUtil::timediff(stv, etv);
  }
  
//   cout << "isDone=" << isParseDone << " isValid=" << isParseValid
//        << " isKeepAlive=" << req.keepAlive << " path=" << req.url << endl;

//   cout << buf << endl;

  if (isParseDone) {
    if (isParseValid) {

      if (req.keepAlive) {
	c.setKeepAlive();
      }
      else {
	c.clearKeepAlive();
      }

      parseCount++;
      c.setHttpVersion(req.httpVersion);
      c.advanceReadBuffer(lengthUsed);

      if (c.readBufferEmpty()) {
	c.useSocket();
      }
      else {
	c.useBuffer();
      }

      req.remoteHost = Util::getHostByAddr(c.getRemoteAddr());
      req.remoteAddr = c.getRemoteAddr();
      req.remotePort = c.getRemotePort();

      req.serverName = serverName;
      req.serverAddr = serverAddr;
      req.serverPort = port;

      req.path = canonicalizePath(req.url);
//       cout << "path=" << path << endl;

      // request web object
      if (!dispatcher.dispatch(req.path, req, s, c)) {
	c.sendError(HttpResponse::NOT_FOUND);
	return;
      }
    }
    else {
      c.sendError(HttpResponse::BAD_REQUEST);
      return;
    }
  }
  else {
    c.useSocket();
  }
} // handleRead

void HttpServer::handleWrite(HttpConnection& c) {
  if (profile && bytesSent == 0) {
    gettimeofday(&sentTimer, 0);
  }

  bytesSent += c.write();
} // handleWrite

string HttpServer::canonicalizePath(const string& url) {
  string p = url;
  if (p.find("http://") == 0) {
    unsigned int i = p.find('/', 8);
    p = p.substr(i);
  }

  StringList l = StrUtil::split("/", p);
  StringList c;

  while (!l.empty()) {
    string t = l.front();
    l.pop_front();
    if (t == "..") {
      if (!c.empty()) {
	c.pop_back();
      }
    }
    else {
      c.push_back(t);
    }
  }

  bool addTrailing = (p[p.size() - 1] == '/');
  p = "/" + StrUtil::join("/", c);

  if (p != "/" && addTrailing) {
    p.append("/");
  }

  return p;
} // canonicalizePath
