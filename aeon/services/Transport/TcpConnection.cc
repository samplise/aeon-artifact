/* 
 * TcpConnection.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, James W. Anderson, Charles Killian
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
#include <errno.h>

#include <boost/format.hpp>

#include "mace-macros.h"
#include "params.h"
#include "Accumulator.h"
#include "Log.h"
#include "Util.h"
#include "StrUtil.h"
#include "ScopedTimer.h"
#include "TransportScheduler.h"

#include "TcpConnection.h"
#include "m_net.h"

#if !(defined _WINSOCK2_H || defined _WINSOCK_H)
#define SOCKOPT_VOID_CAST
#else
#define SOCKOPT_VOID_CAST (char*)
#define SHUT_RDWR SD_BOTH
#endif

using std::min;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::ostream;
using std::istringstream;
using std::ostringstream;

// const size_t TcpConnection::QUEUE_SIZE = 65536;
const size_t TcpConnection::QUEUE_SIZE = 4096;
// const size_t TcpConnection::BLOCK_SIZE = 8192;
// const size_t TcpConnection::BLOCK_SIZE = 16384;
const size_t TcpConnection::BLOCK_SIZE = 32768;
// const size_t TcpConnection::BLOCK_SIZE = 65536;
// const size_t TcpConnection::BLOCK_SIZE = 131072;

TcpConnection::TcpConnection(int s, TransportServiceClass::Connection::type d,
			     TransportCryptoServiceClass::type cryptoFlags,
			     SSL_CTX* sctl, const mace::string& token,
			     size_t queueSize, size_t thresholdSize,
			     const SockAddr& id, const SockAddr& srcid,
			     uint64_t wsize) :
  c(s),
  direction(d),
  cryptoFlags(cryptoFlags),
  ctx(sctl),
  ssl(0),
  peerPubkey(0),
  token(token),
  connectionAccepted(false),
  bufStats(new BufferStatistics(wsize)),
  //bufStats( ),
  idsa(id),
  open(true),
  connecting(true),
  waitingForHeader(true),
  sendable(true),
  suspended(false),
  deliverable(false),
  sslWantWrite(false),
  error(TransportError::NON_ERROR),
  qsize(queueSize),
  threshold(thresholdSize),
  byteCount(0),
  wsent(0),
  wqueued(0),
  rqueued(0),
  rqBytes(0),
  lastActivity(TimeUtil::timeu()),
  raddr(INADDR_NONE),
  rport(0),
  start(TimeUtil::timeu()),
  connectTime(0),
  writeTime(0),
  socketWriteTime(0),
  enqueueTime(0),
  readTime(0),
  socketReadTime(0),
  dequeueTime(0),
  rhq(QUEUE_SIZE),
  rq(QUEUE_SIZE),
  wq(QUEUE_SIZE),
  readbuflen(0),
  readbuf(0),
  readbufstart(0),
  readbufend(0),
  wchunki(0),
  isChunking(false),
  wbuffed(0),
  maxwqlen(0),
  wqcount(0),
  maxrqlen(0),
  rqcount(0),
  bufcopies(0),
  chunkcopies(0)
{
  ADD_SELECTORS("TcpConnection::TcpConnection");
  pthread_mutex_init(&tlock, 0);

  //bufStats.window = wsize;

  if (c == 0) {
    return;
  }

  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  socklen_t slen = sizeof(sa);
  if (getsockname(c, (sockaddr*)&sa, &slen) < 0) {
    Log::perror("getsockname");
    ASSERT(0);
  }

  laddr = sa.sin_addr.s_addr;
  lport = sa.sin_port;
  if (!srcid.isNull()) {
    ASSERT(direction == TransportServiceClass::Connection::OUTGOING);

//     macedbg(1) << "srcid.addr=" << srcid.addr << " srcid.port=" << srcid.port
// 	       << " laddr=" << laddr << " lport=" << ntohs(lport)
// 	       << Log::endl;

//     ASSERT(srcid.addr == laddr);
//     ASSERT(srcid.port == ntohs(lport));
  }

  if (cryptoFlags & TransportCryptoServiceClass::TLS) {
    initSSL();
  }

  if (direction == TransportServiceClass::Connection::INCOMING ||
      direction == TransportServiceClass::Connection::NAT) {
    readbuflen = 1024*1024;
    readbuf = new char[readbuflen];
  }
  else {
    ASSERT(direction == TransportServiceClass::Connection::OUTGOING);
    if (!token.empty()) {
      TransportHeader th(Util::getMaceAddr(), SockUtil::NULL_MACEADDR, 0,
			 TransportHeader::ACCEPTANCE_TOKEN, token.size());
      std::string hdr;
      th.serialize(hdr);
      wq.push(hdr);
      wqcount++;
      wqueued += hdr.size();
      wq.push(token);
      wqcount++;
      wqueued += token.size();
    }
    if (!srcid.isNull()) {
//       macedbg(1) << "sending srcid=" << srcid << Log::endl;

      TransportHeader th(MaceAddr(srcid, SockUtil::NULL_MSOCKADDR),
			 SockUtil::NULL_MACEADDR, 0,
			 TransportHeader::SOURCE_ID, 0);
      std::string hdr;
      th.serialize(hdr);
      wq.push(hdr);
      wqcount++;
      wqueued += hdr.size();
    }
  }

  if (direction == TransportServiceClass::Connection::NAT) {
    TransportHeader th(Util::getMaceAddr(), SockUtil::NULL_MACEADDR, 0,
		       TransportHeader::NAT_CONNECTION, 0);
    std::string hdr;
    th.serialize(hdr);
    wq.push(hdr);
    wqcount++;
  //   cerr << Log::toHex(hdr) << endl;
    wqueued += hdr.size();
  }

} // TcpConnection

TcpConnection::~TcpConnection() {
//   Log::log("TcpConnection::~TcpConnection") << "closing" << Log::endl;
  close();
  if (readbuf) {
    delete [] readbuf;
    readbuf = 0;
  }
  if (ssl) {
    SSL_free(ssl);
    ssl = 0;
  }
} // ~TcpConnection

void TcpConnection::initSSL() {
//   ADD_SELECTORS("TcpConnection::initSSL");
  if (ssl) {
    return;
  }
  
  ssl = SSL_new(ctx);
  ASSERT(ssl != NULL);
  SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
  SSL_set_fd(ssl, c);
//   maceout << "set " << c << " with " << ssl << Log::endl;
} // initSSL

bool TcpConnection::connectSSL() {
  ADD_SELECTORS("TcpConnection::connectSSL");
  static bool dieOnError = params::get("TRANSPORT_ASSERT_SSL", false);

  int r = 0;
  if (direction == TransportServiceClass::Connection::INCOMING) {
//     maceout << "accepting ssl connection" << Log::endl;
    r = SSL_accept(ssl);
  }
  else {
    ASSERT(direction == TransportServiceClass::Connection::OUTGOING ||
	   direction == TransportServiceClass::Connection::NAT);
//     maceout << "connecting ssl connection" << Log::endl;
    r = SSL_connect(ssl);
  }
  if (r <= 0) {
    switch (SSL_get_error(ssl, r)) {
    case SSL_ERROR_WANT_READ:
      break;
    case SSL_ERROR_WANT_WRITE:
      break;
    default:
      close(TransportError::TLS_ERROR, Util::getSslErrorString() + (errno ? " " +
	    Util::getErrorString(errno) : ""));
      // XXX
      ASSERT(!dieOnError);
// 	maceout << "unknown accept error" << Log::endl;
// 	ERR_print_errors_fp(stderr);
// 	ASSERT(0);
    }
//       Log::err() << "SSL_accept error" << Util::getSslErrorString() << Log::endl;
    return false;
  }

//   maceout << "ssl connection established" << Log::endl;

  string peername = StrUtil::toLower(Util::getHostname(raddr));

  // perform verification
  if (SSL_get_verify_result(ssl) != X509_V_OK) {
    string m = "certificate for " + peername + " does not verify";
    maceerr << m << Log::endl;
    close(TransportError::TLS_ERROR, m);
    ASSERT(!dieOnError);
  }


  // check the common name
  X509* peer = SSL_get_peer_certificate(ssl);
  if (peer == NULL) {
    string m = "no peer certificate for " + peername;
    maceerr << m << Log::endl;
    close(TransportError::TLS_ERROR, m);
    ASSERT(!dieOnError);
  }
  size_t len = 512;
  char peerCNbuf[len];
  r = X509_NAME_get_text_by_NID(X509_get_subject_name(peer),
				NID_commonName, peerCNbuf, len);
  if (r == -1) {
    ostringstream m;
    m << "common name not found for " << peername;
    maceerr << m.str() << Log::endl;
    close(TransportError::TLS_ERROR, m.str());
    ASSERT(!dieOnError);
  }
  peerCN = StrUtil::toLower(peerCNbuf);
  if (peerCN != peername && !noVerifyHostname(peerCN)) {
    ostringstream m; 
    m << "common name " << peerCN << " does not match host name "
      << peername;
    maceerr << m.str() << Log::endl;
    close(TransportError::TLS_ERROR, m.str());
    ASSERT(!dieOnError);
  }

  peerPubkey = X509_get_pubkey(peer);
  if (peerPubkey == NULL) {
    ostringstream m;
    m << "could not read public key from " << peername;
    maceerr << m.str() << Log::endl;
    close(TransportError::TLS_ERROR, m.str());
    ASSERT(!dieOnError);
  }

  return true;
} // connectSSL

void TcpConnection::connect() {
  ADD_SELECTORS("TcpConnection::connect");
  ASSERT(connecting);
  lastActivity = TimeUtil::timeu();
  static const bool SET_TCP_KEEPALIVE = params::get("SET_TCP_KEEPALIVE", false);

  if ((raddr == INADDR_NONE) && (rport == 0)) {
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    socklen_t slen = sizeof(sa);
    if (getpeername(c, (sockaddr*)&sa, &slen) < 0) {
      if (SockUtil::errorNotConnected()) {
	// connect failed
	int err;
	socklen_t len = sizeof(err);
	if (getsockopt(c, SOL_SOCKET, SO_ERROR, SOCKOPT_VOID_CAST &err, &len) < 0) {
	  Log::perror("getsockopt");
          ABORT("getsockopt error");
	}
	close(TransportError::CONNECT_ERROR, Util::getErrorString(err));
	return;
      }
      Log::perror("getpeername");
      ABORT("getpeername error");
    }
    {
      int err;
      socklen_t len = sizeof(err);
      ASSERT(getsockopt(c, SOL_SOCKET, SO_ERROR, SOCKOPT_VOID_CAST &err, &len) == 0);
    }

    raddr = sa.sin_addr.s_addr;
    rport = sa.sin_port;

//     if (idsa.addr == INADDR_NONE) {
//       idsa.addr = raddr;
//       idsa.port = rport;
//     }
  }

  if (cryptoFlags & TransportCryptoServiceClass::TLS) {
    if (!connectSSL()) {
      return;
    }
  }

  if (SET_TCP_KEEPALIVE) {
    SockUtil::setKeepalive(c);
  }

  if (!macedbg(1).isNoop()) {
    macedbg(1) << "Finished connecting to " << idsa << " socket " << c
	       << " raddr=" << raddr << " rport=" << rport
	       << Log::endl;
  }

  connecting = false;
  connectTime = TimeUtil::timeu() - start;
} // connect

ssize_t TcpConnection::read(int s, void* buf, size_t count) {
  ScopedTimer st(socketReadTime, USE_DEBUGGING_TIMERS);
  ADD_SELECTORS("TcpConnection::read#2");
  lastActivity = TimeUtil::timeu();
  int r = 0;
  if (cryptoFlags & TransportCryptoServiceClass::TLS) {
     macedbg(1) << "calling SSL_read on " << s << Log::endl;
    r = SSL_read(ssl, buf, count);
     macedbg(1) << "SSL_read on " << s << " returned " << r << Log::endl;
    if (r <= 0) {
      switch (SSL_get_error(ssl, r)) {
      case SSL_ERROR_WANT_READ:
	break;
//       case SSL_ERROR_NONE:
//       case SSL_ERROR_ZERO_RETURN:
//       case SSL_ERROR_WANT_CONNECT:
//       case SSL_ERROR_WANT_ACCEPT:
//       case SSL_ERROR_SYSCALL:
//       case SSL_ERROR_SSL:
      default:
// 	maceout << "closing connection on SSL error" << Log::endl;
	close(TransportError::READ_ERROR, Util::getSslErrorString()  + " " +
	      Util::getErrorString(errno));
      }
    }
//     else {
//       maceout << Log::toHex(string((const char*)buf, r)) << Log::endl;
//     }
  }
  else {
     
    r = ::recv(s, (char*)buf, count, 0);
    //macedbg(1) << "Reading from socket " << s << " and read bytes: "<< r <<Log::endl;
    int err = SockUtil::getErrno();
    if (r == 0) {
      close(TransportError::READ_ERROR, "socket closed");
    }
    else if ((r < 0) && (!SockUtil::errorWouldBlock(err))) {
      close(TransportError::READ_ERROR, Util::getErrorString(err));
    }
  }

  if (!macedbg(2).isNoop()) {
    macedbg(2) << "read from socket " << s
	       << " (" << Util::getHostname(raddr) << "):"
	       << " read " << r << " bytes" << Log::endl;
  }

  return r;
} // read

ssize_t TcpConnection::write(int s, const void* buf, size_t count) {
  ScopedTimer st(socketWriteTime, USE_DEBUGGING_TIMERS);
  ADD_SELECTORS("TcpConnection::write#3");
  lastActivity = TimeUtil::timeu();
  int r = 0;
  if (cryptoFlags & TransportCryptoServiceClass::TLS) {
//     macedbg(1) << "calling SSL_write on " << s << Log::endl;
    sslWantWrite = false;
    r = SSL_write(ssl, buf, count);
//     macedbg(1) << "SSL_write on " << s << " returned " << r << Log::endl;
    if (r <= 0) {
      switch (SSL_get_error(ssl, r)) {
      case SSL_ERROR_WANT_WRITE:
	maceout << "SSL_ERROR_WANT_WRITE on " << s << Log::endl;
	sslWantWrite = true;
	break;
//       case SSL_ERROR_NONE:
// 	ASSERT(0);
// 	break;
//       case SSL_ERROR_ZERO_RETURN:
// 	maceout << "closing connection on SSL_ERROR_ZERO_RETURN" << Log::endl;
// 	close(TransportError::WRITE_ERROR, Util::getSslErrorString());
// 	break;
//       case SSL_ERROR_WANT_CONNECT:
// 	Log::err() << "want connect error" << Log::endl;
// 	ASSERT(0);
//       case SSL_ERROR_WANT_ACCEPT:
// 	Log::err() << "want accept error" << Log::endl;
// 	ASSERT(0);
//       case SSL_ERROR_SYSCALL:
// 	Log::err() << "syscall error" << Log::endl;
// 	ASSERT(0);
//       case SSL_ERROR_SSL:
// 	Log::err() << "ssl error" << Log::endl;
// 	ASSERT(0);
//       default:
// 	ERR_print_errors_fp(stderr);
// 	Log::err() << "TcpConnection::write: SSL write error "
// 		   << Util::getSslErrorString() << Log::endl;
// 	ASSERT(0);
      default:
 	maceout << "closing connection on SSL error" << Log::endl;
	close(TransportError::WRITE_ERROR, Util::getSslErrorString()  + " " +
	      Util::getErrorString(errno));
      }
    }
//     else {
//       maceout << Log::toHex(string((const char*)buf, r)) << Log::endl;
//     }
  }
  else {
    //     r = ::write(s, buf, count);
//     macedbg(1) << "Sending " << count << " bytes on socket " << s << Log::endl;
    //macedbg(1) << "Send message to destination!" << Log::endl;
    r = ::send(s, (char*)buf, count, 0);
    if (r < 0) {
      if (! SockUtil::errorWouldBlock()) {
        close(TransportError::WRITE_ERROR,
	      Util::getErrorString(SockUtil::getErrno()));
      }
    }
  }

  if (!macedbg(2).isNoop()) {
    macedbg(2) << "write " << count << " bytes on socket " << s
	       << " (" << Util::getHostname(raddr) << "):"
	       << " sent " << r << " bytes " << " wsize=" << wsize()
	       << Log::endl;
  }

  return r;
} // write

void TcpConnection::read() {
  ADD_SELECTORS("TcpConnection::read#1");
  static bool connectionAccum = params::get("TCP_CONNECTION_ACCUMULATOR", false);
  static Accumulator* connaccum = 0;
  if (connectionAccum && connaccum == 0) {
    string host = Util::getAddrString(raddr) + ":" +
      StrUtil::toString(ntohs(rport));
    connaccum = Accumulator::Instance("TCP_READ::" + host);
  }
  static Accumulator* readaccum = Accumulator::Instance(Accumulator::TCP_READ);
  static Accumulator* netaccum = Accumulator::Instance(Accumulator::NETWORK_READ);
  static size_t thssz = TransportHeader::ssize();

  ScopedTimer st(readTime, USE_DEBUGGING_TIMERS);

  ASSERT(open);

  if (connecting) {
    connect();
    if (connecting) {
      return;
    }
  }

  size_t ramount = BLOCK_SIZE;
  ASSERT(readbufend != readbuflen);
  if (readbuflen - readbufend < BLOCK_SIZE) {
    ramount = readbuflen - readbufend;
  }
  if (readbufend < readbufstart && readbufstart - readbufend - 1 < ramount) {
    ramount = readbufstart - readbufend - 1;
  }
  ASSERT(ramount > 0);
  //macedbg(1) << "Try to read data!" << Log::endl;
  int r = read(c, readbuf + readbufend, ramount);
  //macedbg(1) << "Read " << r << " bytes" << Log::endl;


  if (r <= 0) {
    return;
  }

  rqueued += r;
  readbufend = (readbufend + r) % readbuflen;

  if (connaccum) {
    connaccum->accumulate(r);
  }
  readaccum->accumulate(r);
  netaccum->accumulate(r);
  bufStats->append(r);
  byteCount += r;

  while (open) {
    if (waitingForHeader) {
      if (rqueued < thssz) {
        //macedbg(1) << "Wrong!!! rqueued="<< rqueued << ", thssz="<< rhdrsize << Log::endl;
	      return;
      }
      waitingForHeader = false;
      readString(rhdrstr, thssz);
      rqueued -= thssz;
      rqBytes += thssz;

      rhdrsize = TransportHeader::deserializeSize(rhdrstr);
      if (!macedbg(2).isNoop()) {
	       macedbg(2) << "Read header " << thssz << " bytes, " << " reading message of size " << rhdrsize << Log::endl;
      }

      if (readbuflen < rhdrsize) {
	       growReadbuf(rhdrsize);
      }
    }

    if (rqueued < rhdrsize) {
      // macedbg(1) << "Wrong!!! rqueued="<< rqueued << ", rhdrsize="<< rhdrsize << Log::endl;
      return;
    }

    readString(rbuf, rhdrsize);
    rqueued -= rhdrsize;
    rqBytes += rhdrsize;
    waitingForHeader = true;

    if (idsa.addr == INADDR_NONE) {
      if (!readAddr()) {
	       continue;
      }
    }

    rhq.push(rhdrstr);
    if (!macedbg(2).isNoop()) {
      macedbg(2) << "enqueuing " << HashString::hash(rbuf) << " for deliver" << Log::endl;
    }
    rq.push(rbuf);
    maxrqlen = std::max(maxrqlen, rq.size());
    rqcount++;
  }
} // read

void TcpConnection::growReadbuf(size_t min) {
  size_t ns = readbuflen;
  while (ns < min) {
    ns *= 2;
  }
  rbuf.reserve(ns);

  char* old = readbuf;
  readbuf = new char[ns];

  if (readbufstart > readbufend) {
    memcpy(readbuf, old + readbufstart, readbuflen - readbufstart);
    memcpy(readbuf + readbufstart, old, readbufend);
    readbufend += readbuflen - readbufstart;
  }
  else {
    memcpy(readbuf, old + readbufstart, readbufend - readbufstart);
    readbufend -= readbufstart;
  }

  delete [] old;
  readbuflen = ns;
  readbufstart = 0;
} // growReadbuf

bool TcpConnection::readAddr() {
  ADD_SELECTORS("TcpConnection::readAddr");
  
  TransportHeader thdr;
  istringstream in(rhdrstr);
  thdr.deserialize(in);

  if (thdr.flags & TransportHeader::ACCEPTANCE_TOKEN) {
    token = rbuf;
    tokenMaceAddr = thdr.src;
    return false;
  }

  if (thdr.src.isUnroutable() &&
      thdr.flags & TransportHeader::NAT_CONNECTION) {
    direction = TransportServiceClass::Connection::OUTGOING;
    idsa = thdr.src.local;
    if (!macedbg(1).isNoop()) {
      macedbg(1) << "marking connection " << c << " " << idsa << " as OUTGOING"
		 << Log::endl;
    }
    return false;
  }

  if (thdr.flags & TransportHeader::SOURCE_ID) {
    idsa = thdr.src.local;
    if (!macedbg(1).isNoop()) {
      macedbg(1) << "read source id " << idsa
		 << " for connection " << c
		 << Log::endl;
    }
    return false;
  }

  if (!thdr.src.proxy.isNull()) {
    idsa = thdr.src.proxy;
  }
  else {
    idsa = thdr.src.local;
  }
  return true;
} // readAddr

void TcpConnection::write() {
  ADD_SELECTORS("TcpConnection::write#1");
  ScopedTimer st(writeTime, USE_DEBUGGING_TIMERS);
  ASSERT(open);
  ASSERT(wqueued > wsent);
  if (connecting) {
    connect();
    if (connecting || !open) {
      return;
    }
  }

  while (!sslWantWrite && (wbuf.size() < BLOCK_SIZE) && !wq.empty()) {
    string& s = wq.front();
    size_t fsz = s.size();
    if (fsz > BLOCK_SIZE) {
      isChunking = true;
    }
    if (isChunking) {
      chunkcopies++;
      size_t sz = std::min(fsz - wchunki, BLOCK_SIZE);
      wbuf.append(s.data() + wchunki, sz);
      wchunki += sz;
      if (wchunki == fsz) {
	       isChunking = false;
	       wchunki = 0;
	       wqRemove();
      }
    }
    else {
      bufcopies++;
      wbuf.append(s);
      wqRemove();
    }
  }

  //macedbg(1) << "Try to write size=" << wbuf.size() << Log::endl;
  int r = write(wbuf);
  if (r <= 0) {
    return;
  }
  wbuf = wbuf.substr(r);

  wsent += r;
  updateBuffed();

  if (!sendable && (wsize() < std::min(threshold, qsize))) {
    sendable = true;
  }
} // write

int TcpConnection::write(const std::string& buf) {
  ADD_SELECTORS("TcpConnection::write#2");
  static bool connectionAccum = params::get("TCP_CONNECTION_ACCUMULATOR", false);
  static Accumulator* connaccum = 0;
  if (connectionAccum && connaccum == 0) {
    string host = Util::getAddrString(raddr) + ":" +
      StrUtil::toString(ntohs(rport));
    connaccum = Accumulator::Instance("TCP_WRITE::" + host);
  }
  static Accumulator* writeaccum = Accumulator::Instance(Accumulator::TCP_WRITE);
  static Accumulator* netaccum = Accumulator::Instance(Accumulator::NETWORK_WRITE);
  //macedbg(1) << "Try to write the message!" << Log::endl;
  int r = write(c, buf.data(), buf.size());

//   if (!macedbg(1).isNoop()) {
//     macedbg(1) << "wrote " << r << " of " << wbuf.size() << " bytes" << Log::endl;
//   }

  if (r == 0) {
    Log::warn() << "TcpConnection::write wrote 0 bytes"
		<< " buf.size=" << buf.size()
		<< " wqueued=" << wqueued << " wsent=" << wsent
		<< " wq=" << wq.size() << Log::endl;
  }

  if (r <= 0) {
    return r;
  }

  if (connaccum) {
    connaccum->accumulate(r);
  }
  writeaccum->accumulate(r);
  netaccum->accumulate(r);
  bufStats->append(r);

  byteCount += r;
  return r;
} // write

void TcpConnection::close(TransportError::type err, const std::string& m,
			  bool reset) {
  ADD_SELECTORS("TcpConnection::close");
  ScopedLock sl(tlock);

  if (open) {
    open = false;

    error = err;
    errstr = m;

    if (c != 0) {
      if (reset) {
	maceout << "sending reset on " << c << Log::endl;
	struct linger ling;
	ling.l_onoff = 1;
	ling.l_linger = 0;
	if (setsockopt(c, SOL_SOCKET, SO_LINGER, SOCKOPT_VOID_CAST &ling, sizeof(ling)) < 0) {
	  Log::perror("setsockopt");
	}
      }

      if (cryptoFlags & TransportCryptoServiceClass::TLS &&
	  error == TransportError::NON_ERROR) {
	maceout << "shutting down ssl on " << c << Log::endl;
	SSL_shutdown(ssl);
      }
    
      ::shutdown(c, SHUT_RDWR);
      ::close(c);
    }

    uint64_t end = TimeUtil::timeu();
    uint64_t idle = end - lastActivity;
    uint64_t t = end - start - idle - connectTime;

    double bw = 0.0;
    string unit = "B";
    
    if (t > 0) {
      bw = byteCount * 1000 * 1000 / t;
      if (bw > 1024) {
        unit = "KB";
        bw /= 1024;
      }
      if (bw > 1024) {
        unit = "MB";
        bw /= 1024;
      }
    }
    
    maceout << c << " open for " << (t / 1000) << " ms "
	    << "connect " << (connectTime / 1000) << " ms "
	    << "idle " << (idle / 1000) << " ms "
	    << byteCount << " bytes "
	    << boost::format("%.3f") % bw << " " << unit << "/sec" << Log::endl;
    maceout << "  wbuf=" << wbuf.size() << " rbuf=" << rbuf.size()
	    << " rq=" << rq.size() << " wq=" << wq.size() << " wsent=" << wsent
	    << " wqd=" << wqueued << " rqd=" << rqueued
	    << " rqb=" << rqBytes
	    << " wqc=" << wqcount << " rqc=" << rqcount
	    << " rts=" << rtsList.size() << Log::endl;
    maceout << "  maxwqlen=" << maxwqlen << " maxrqlen=" << maxrqlen
	    << " bc=" << bufcopies
	    << " cc=" << chunkcopies
	    << " conn=" << connecting
	    << " err=" << err << " " << m << Log::endl;
    maceout << "  raddr=" << Util::getAddrString(raddr) << ":" << ntohs(rport)
	    << " id=" << idsa;
    for (NodeSet::const_iterator i = rkeys.begin(); i != rkeys.end(); i++) {
      maceout << " " << *i;
    }
    maceout << Log::endl;

    maceout << "  writeTime=" << writeTime << " enqueueTime=" << enqueueTime
	    << " socketWriteTime=" << socketWriteTime
	    << Log::endl;
    maceout << "  readTime=" << readTime << " dequeueTime=" << dequeueTime
	    << " socketReadTime=" << socketReadTime
	    << Log::endl;

  }
} // close

void TcpConnection::enqueue(TransportHeader& th, const std::string& ph, const std::string& buf) {
  ADD_SELECTORS("TcpConnection::enqueue");
//   ASSERT(open);
  static bool PRINT_QUEUE = params::get("TRANSPORT_LOG_QUEUE_LEN", false);

  ScopedTimer st(enqueueTime, USE_DEBUGGING_TIMERS);

  bool doSignal = (wsize() == 0) || (wqueued == 0);
  uint64_t prevwqueued = wqueued;

  std::string hdr;
  th.serialize(hdr);

  bool printQueueLen = PRINT_QUEUE;
  if (open && !connecting && wqueued == wsent && buffed.empty() &&
      hdr.size() + ph.size() + buf.size() < BLOCK_SIZE) {
    ASSERT(!sslWantWrite);
    ASSERT(wbuf.empty());
    ASSERT(wq.empty());
    wbuf = hdr;
    if (!ph.empty()) {
      wbuf.append(ph);
    }
    wbuf.append(buf);
    int r = write(wbuf);
    int tqsz = (int)wbuf.size();
    if (r < tqsz) {
      buffed.push(hdr);
      if (!ph.empty()) {
        buffed.push(ph);
      }
      buffed.push(buf);
      if (r > 0) {
        wbuf = wbuf.substr(r);
        tqsz -= r;
      }
      wqueued += tqsz;
      bufcopies++;
    }
    else {
      ASSERT(r == tqsz);
      wbuf.clear();
      doSignal = false;
      printQueueLen = false;
    }
  }
  else {
    macedbg(1) << "Pushing message into queue!" << Log::endl;
    wq.push(hdr);
    wqcount++;
    //   cerr << Log::toHex(hdr) << endl;
    wqueued += hdr.size();

    if (!ph.empty()) {
      wq.push(ph);
      wqueued += ph.size();
      wqcount++;
    }

    wq.push(buf);
    wqueued += buf.size();
    wqcount++;
  }

  if (doSignal) {
    TransportScheduler::signal(wqueued - prevwqueued, false);
  }

  if (printQueueLen) {
    maceout << "buffed=" << buffed.size() << " wq=" << wq.size() << Log::endl;
  }

  maxwqlen = std::max(wq.size(), maxwqlen);

  if (sendable && (wsize() > qsize)) {
    sendable = false;
  }
} // enqueue

void TcpConnection::dequeue(string& hdr, string& buf) {
  static bool PRINT_QUEUE = params::get("TRANSPORT_LOG_QUEUE_LEN", false);
  ScopedTimer st(dequeueTime, USE_DEBUGGING_TIMERS);

  ADD_SELECTORS("TcpConnection::dequeue");
  ASSERT(!rq.empty() && !rhq.empty());

  buf = rq.front();
  rq.pop();
  hdr = rhq.front();
  rhq.pop();

  if (PRINT_QUEUE) {
    maceout << "inqueue=" << rq.size() << Log::endl;
  }

  rqBytes -= (buf.size() + hdr.size());
} // dequeue

bool TcpConnection::noVerifyHostname(const std::string& host) {
  static bool set = false;
  static bool noverify = false;
  static bool check = false;
  static StringList allowed;
  if (!set) {
    set = true;
    if (params::containsKey(params::MACE_NO_VERIFY_HOSTNAMES)) {
      std::string tmp = params::get<string>(params::MACE_NO_VERIFY_HOSTNAMES);
      if (StrUtil::tryCast(tmp, noverify)) {
	return true;
      }
      allowed = StrUtil::split(" ", tmp);
      for (size_t i = 0; i < allowed.size(); i++) {
	allowed[i] = StrUtil::toLower(allowed[i]);
      }
      check = true;
    }
  }

  if (noverify) {
    return true;
  }

  if (check) {
    for (size_t i = 0; i < allowed.size(); i++) {
      if (host == allowed[i]) {
	return true;
      }
    }
  }

  return false;
} // noVerifyHostname

void TcpConnection::notifyMessageErrors(PipelineElement* pipeline, BaseTransport::NetworkHandlerMap& handlers) {
  ADD_SELECTORS("TcpConnection::notifyMessageErrors");

  while (!wq.empty()) {
    buffed.push(wq.front());
    wq.pop();
  }

  TransportHeader hdr;
  while (!buffed.empty()) {
    istringstream in(buffed.front());
    hdr.deserialize(in);

    buffed.pop();

    if (hdr.flags & TransportHeader::NAT_CONNECTION) {
      ASSERT(hdr.size == 0);
      ASSERT(direction == TransportServiceClass::Connection::NAT);
      continue;
    }
    if (hdr.flags & TransportHeader::SOURCE_ID) {
      ASSERT(hdr.size == 0);
      ASSERT(direction == TransportServiceClass::Connection::OUTGOING);
      continue;
    }

    std::string ph;
    std::string buf;

    if (buffed.front().size() < hdr.size) {
      ph = buffed.front();
      buffed.pop();
    }

    buf = buffed.front();
    buffed.pop();

    ASSERT(hdr.size == (buf.size() + ph.size()));

    MaceKey dest(ipv4, hdr.dest);
    if (pipeline) {
      pipeline->messageError(dest, ph, buf, hdr.rid);
    }

    ASSERT(!buf.empty());

    BaseTransport::NetworkHandlerMap::iterator i = handlers.find(hdr.rid);
    if (i != handlers.end()) {
      i->second->messageError(dest, error, buf, hdr.rid);
    } 
    else {
      maceerr << "Could not find registered handler for message error!" << Log::end;
    }
  }
//   traceout << false << Log::end;
  
}
