/* 
 * TcpConnection.h : part of the Mace toolkit for building distributed systems
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
#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <pthread.h>
#include <boost/shared_ptr.hpp>

#include "SockUtil.h"
#include "Collections.h"
#include "CircularQueue.h"
#include "CircularQueueList.h"
#include "mace-macros.h"

#include "BufferedTransportServiceClass.h"
#include "BaseTransport.h"
#include "NetworkErrorHandler.h"
#include "TransportHeader.h"
#include "TransportCryptoServiceClass.h"
#include "ConnectionAcceptanceServiceClass.h"

class TcpConnection {
public:
  static const bool USE_DEBUGGING_TIMERS = false;

  typedef CircularQueueList<std::string> StringQueue;

  struct StatusCallbackArgs {
    StatusCallbackArgs() : rid(0), ts(TimeUtil::timeu()) { }
    StatusCallbackArgs(const MaceKey& dest, registration_uid_t rid) :
      dest(dest), rid(rid), ts(TimeUtil::timeu()) { }

    MaceKey dest;
    registration_uid_t rid;
    uint64_t ts;
  };

  typedef CircularQueueList<StatusCallbackArgs> StatusCallbackArgsQueue;

public:
  TcpConnection(int s, TransportServiceClass::Connection::type t,
		TransportCryptoServiceClass::type cryptoFlags, SSL_CTX* ctx,
		const mace::string& token,
		size_t queueSize, size_t thresholdSize,
		const SockAddr& id = SockUtil::NULL_MSOCKADDR, 
		const SockAddr& srcid = SockUtil::NULL_MSOCKADDR,
		uint64_t wsize = BaseTransport::DEFAULT_WINDOW_SIZE);
  virtual ~TcpConnection();
  TransportServiceClass::Connection::type directionType() const {
    return direction;
  }
  bool isReadable() const {
    return (open && (direction == TransportServiceClass::Connection::INCOMING ||
		     direction == TransportServiceClass::Connection::NAT) &&
	    !suspended);
  }
  bool isWriteable() const {
    return (open && (direction == TransportServiceClass::Connection::OUTGOING ||
		     direction == TransportServiceClass::Connection::NAT) &&
	    (wqueued > wsent));
  }
  bool isOpen() const { return open; }
  bool isConnecting() const {
    return connecting || (idsa == SockUtil::NULL_MSOCKADDR);
  }
  bool hasError() const { return (error != TransportError::NON_ERROR); }
  TransportError::type getError() const { return error; }
  const std::string& getErrorString() const { return errstr; }
  void read();
  void write();
  void close(TransportError::type err = TransportError::NON_ERROR,
	     const std::string& m = "", bool reset = false);
  void setQueueSize(size_t qs, size_t thresh) {
    qsize = qs;
    threshold = thresh;
  }
  void setWindowSize(uint64_t microsec) {
    bufStats->window = microsec;
  }
  size_t queueSize() const { return qsize; }
  size_t size() const { return wqueued - wsent + rqueued; }
  size_t rsize() const { return rqueued + rqBytes; }
  uint32_t rqsize() const { return rq.size(); }
  size_t wsize() const { return wqueued - wsent; }
  bool rqempty() const { return rq.empty(); }
  void enqueue(TransportHeader& h, const std::string& ph, const std::string& buf);
  void dequeue(std::string& h, std::string& buf);
  uint32_t localAddr() const { return laddr; }
  uint16_t localPort() const { return ntohs(lport); }
  uint32_t remoteAddr() const { return raddr; }
  uint16_t remotePort() const { return ntohs(rport); }
  mace::string x509CommonName() const { return peerCN; }
  EVP_PKEY* publicKey() const { return peerPubkey; }
  const NodeSet& remoteKeys() const { return rkeys; }
  void addRemoteKey(const MaceKey& k) { rkeys.insert(k); }
  const SockAddr& id() const { return idsa; }
  int sockfd() const { return c; }
  const BufferStatisticsPtr stats() {
    bufStats->update(TimeUtil::timeu());
    return bufStats;
  }
  void addRTS(const MaceKey& dest, registration_uid_t rid) {
    rtsList.push(StatusCallbackArgs(dest, rid));
  }
  bool hasRTS() { return !rtsList.empty(); }
  StatusCallbackArgsQueue& getRTS() { return rtsList; }
  bool canSend() const { return sendable; }
  uint64_t lastActivityTime() const { return lastActivity; }
  uint64_t idleTime() const { return (TimeUtil::timeu() - lastActivity); }
  bool idle() const { return (rqempty() && !isWriteable() && (rqueued == 0)); }
  uint64_t timeOpened() const { return start; }
  void notifyMessageErrors(PipelineElement* pipeline, BaseTransport::NetworkHandlerMap& handlers);
  void suspend() {
    ASSERT(direction == TransportServiceClass::Connection::INCOMING);
    suspended = true;
  }
  void resume() {
    ASSERT(direction == TransportServiceClass::Connection::INCOMING);
    suspended = false;
  }
  void markDeliverable() { deliverable = true; }
  bool isDeliverable() const { return !suspended && deliverable && !rqempty(); }
  bool isSuspended() const { return suspended; }
  bool acceptedConnection() const { return connectionAccepted; }
  void acceptConnection() { connectionAccepted = true; }
  const mace::string& getToken() const { return token; }
  const MaceAddr& getTokenMaceAddr() const { return tokenMaceAddr; }
  SSL* getSSL() { return ssl; }

protected:
  void initSSL();
  bool connectSSL();
  void growReadbuf(size_t min);
  bool readAddr();
  ssize_t read(int s, void* buf, size_t count);
  ssize_t write(int s, const void* buf, size_t count);
  int write(const std::string& buf);
  void connect();
  bool noVerifyHostname(const std::string& host);

  void readString(std::string& str, size_t len) {
    str.clear();
    if (len == 0) {
      return;
    }
    ASSERT(readbufstart != readbufend);
    if (readbufstart > readbufend) {
      if (readbuflen - readbufstart >= len) {
	       str.append(readbuf + readbufstart, len);
	       readbufstart = (readbufstart + len) % readbuflen;
      } else {
	       str.append(readbuf + readbufstart, readbuflen - readbufstart);
	       str.append(readbuf, len - (readbuflen - readbufstart));
	       readbufstart = len - (readbuflen - readbufstart);
	       ASSERT(readbufstart <= readbufend);
      }
    } else {
      str.append(readbuf + readbufstart, len);
      readbufstart += len;
      ASSERT(readbufstart <= readbufend);
    }
    ASSERT(readbufstart < readbuflen);
  } // readString

  void wqRemove() {
    ADD_SELECTORS("TcpConnection::wqRemove");
    buffed.push(wq.front());
    wq.pop();
  } // wqRemove

  void updateBuffed() {
    ADD_SELECTORS("TcpConnection::updateBuffed");
    size_t dsz = TransportHeader::deserializeSize(buffed.front());
    size_t sz = TransportHeader::ssize() + dsz;
    uint8_t flags = TransportHeader::deserializeFlags(buffed.front());

    while ((wbuffed + sz) <= wsent) {
      wbuffed += sz;
      buffed.pop();
      if (flags & TransportHeader::NAT_CONNECTION) {
	       ASSERT(dsz == 0);
	       ASSERT(direction == TransportServiceClass::Connection::NAT);
	       return;
      }
      if (flags & TransportHeader::SOURCE_ID) {
	       ASSERT(dsz == 0);
	       ASSERT(direction == TransportServiceClass::Connection::OUTGOING);
	       return;
      }

      size_t poppedsz = buffed.front().size();
      if (poppedsz < dsz) {
        buffed.pop();
        poppedsz += buffed.front().size();
      }
      ASSERT(poppedsz == dsz);
      buffed.pop();

      if (buffed.empty()) {
	return;
      }
      dsz = TransportHeader::deserializeSize(buffed.front());
      ASSERT(dsz > 0);
      sz = TransportHeader::ssize() + dsz;
    }
  } // updateBuffed

private:
  socket_t c;
  TransportServiceClass::Connection::type direction;
  TransportCryptoServiceClass::type cryptoFlags;
  SSL_CTX* ctx;
  SSL* ssl;
  mace::string peerCN;
  EVP_PKEY* peerPubkey;
  mace::string token;
  bool connectionAccepted;
  MaceAddr tokenMaceAddr;
  BufferStatisticsPtr bufStats;
  SockAddr idsa;
  bool open;
  bool connecting;
  bool waitingForHeader;
  bool sendable;
  bool suspended;
  bool deliverable;
  bool sslWantWrite;
  TransportError::type error;
  size_t qsize;
  size_t threshold;
  uint64_t byteCount;
  uint64_t wsent;
  uint64_t wqueued;
  uint64_t rqueued;
  uint64_t rqBytes;
  uint64_t lastActivity;

  uint32_t laddr;
  uint16_t lport;
  uint32_t raddr;
  uint16_t rport;
  NodeSet rkeys;

  pthread_mutex_t tlock;
  uint64_t start;
  uint64_t connectTime;
  uint64_t writeTime;
  uint64_t socketWriteTime;
  uint64_t enqueueTime;
  uint64_t readTime;
  uint64_t socketReadTime;
  uint64_t dequeueTime;

  std::string errstr;

  StringQueue rhq;
  StringQueue rq;
  size_t rhdrsize;
  std::string rhdrstr;
  StringQueue wq;
  std::string rbuf;
  std::string wbuf;
  static const size_t QUEUE_SIZE;
  static const size_t BLOCK_SIZE;
  size_t readbuflen;
  char* readbuf;
  size_t readbufstart;
  size_t readbufend;
  size_t wchunki;
  bool isChunking;

  StringQueue buffed;
  bool buffedhdr;
  uint64_t wbuffed;

  uint64_t maxwqlen;
  uint64_t wqcount;
  uint64_t maxrqlen;
  uint64_t rqcount;
  size_t bufcopies;
  size_t chunkcopies;

  StatusCallbackArgsQueue rtsList;
}; // TcpConnection

typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

#endif // TCP_CONNECTION_H
