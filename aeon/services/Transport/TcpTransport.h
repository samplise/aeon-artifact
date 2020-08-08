/* 
 * TcpTransport.h : part of the Mace toolkit for building distributed systems
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
#ifndef TCP_TRANSPORT_H
#define TCP_TRANSPORT_H

#include "Accumulator.h"
#include "mdeque.h"
#include "m_map.h"
#include "MaceTypes.h"
#include "Collections.h"
#include "SockUtil.h"
#include "CircularUniqueQueue.h"

#include "TcpTransport-init.h"
#include "TcpConnection.h"
#include "BaseTransport.h"

#include "SimulatorBasics.h"

/**
 * Note: the queueSize parameter is now in bytes.  If a message is
 * larger than the maxQueueSize, then the transport will
 * unconditionally accept it to prevent starvation.
 */

class TcpTransport;

typedef boost::shared_ptr<TcpTransport> TcpTransportPtr;

class TcpTransport : public virtual BaseTransport {
//   typedef mace::hash_map<int, TcpConnectionPtr> ConnectionMap;
  typedef mace::map<socket_t, TcpConnectionPtr, mace::SoftState> ConnectionMap;

  struct SendDataStruct {
    MaceAddr src;
    MaceKey dest;
    MaceAddr nextHop;
    registration_uid_t rid;
    std::string ph;
    std::string s;
    bool checkQueueSize;
    bool rts;

    SendDataStruct(const MaceAddr& src, const MaceKey& dest,
		   const MaceAddr& nextHop, registration_uid_t rid,
		   const std::string& ph, const std::string& s,
		   bool checkQueueSize, bool rts) :
      src(src), dest(dest), nextHop(nextHop), rid(rid), ph(ph), s(s),
      checkQueueSize(checkQueueSize), rts(rts) {
    }
  }; // struct SendDataStruct

  struct RequestToSendStruct {
    MaceKey dest;
    registration_uid_t rid;

    RequestToSendStruct(const MaceKey& dest, registration_uid_t rid) :
      dest(dest), rid(rid) {
    }
  }; // struct RequestToSendStruct

  struct SetQueueSizeStruct {
    MaceKey peer;
    uint32_t size;
    uint32_t threshold;

    SetQueueSizeStruct(const MaceKey& peer, uint32_t size, uint32_t threshold) :
      peer(peer), size(size), threshold(threshold) {
    }
  }; // struct SetQueueSizeStruct

  struct SetWindowSizeStruct {
    MaceKey peer;
    uint64_t microsec;

    SetWindowSizeStruct(const MaceKey& peer, uint64_t microsec) :
      peer(peer), microsec(microsec) {
    }
  }; // struct SetWindowSizeStruct

public:
  static TcpTransportPtr create(const TcpTransport_namespace::OptionsMap& o);
  virtual ~TcpTransport();

  virtual void requestToSend(const MaceKey& dest, registration_uid_t rid = -1) {
    ScopedLock sl(tlock);
    TcpConnectionPtr c = connect(dest);
    if (c == 0) {
      deferredRTS.push_back(RequestToSendStruct(dest, rid));
      return;
    }
    if (!c->isOpen()) {
      return;
    }
    c->addRTS(dest, rid);
    if (c->canSend()) {
      sendable.push(c);
      signalDeliver();
    }
  } // requestToSend

  virtual bool canSend(const MaceKey& dest) const {
    ADD_SELECTORS("TcpTransport::canSend");
    ScopedLock sl(tlock);
    bool r = canSend(getNextHop(dest.getMaceAddr()));
//     traceout << r << Log::end;
    return r;
  } // canSend

  virtual uint32_t availableBufferSize(const MaceKey& dest) const {
    ADD_SELECTORS("TcpTransport::availableBufferSize");
    ScopedLock sl(tlock);
    uint32_t r = availableBufferSize(getNextHop(dest.getMaceAddr()));
//     traceout << r << Log::end;
    return r;
  } // availableBufferSize

  virtual uint32_t bufferedDataSize() const {
    ADD_SELECTORS("TcpTransport::bufferedDataSize");
    ScopedLock sl(tlock);
    uint32_t r = 0;
    for (ConnectionMap::const_iterator i = connections.begin();
	 i != connections.end(); i++) {
      r += i->second->size();
    }
//     traceout << r << Log::end;
    return r;
  } // bufferedDataSize

  virtual bool hasBufferedData() const {
    ADD_SELECTORS("TcpTransport::hasBufferedData");
    bool r = (bufferedDataSize() != 0);
//     traceout << r << Log::end;
    return r;
  } // hasBufferedData

  virtual bool hasOutgoingBufferedData() const {
    ADD_SELECTORS("TcpTransport::hasOutgoingBufferedData");
    bool r = (outgoingBufferedDataSize() != 0);
//     traceout << r << Log::end;
    return r;
  } // hasOutgoingBufferedData

  virtual uint32_t outgoingBufferedDataSize() const {
    ADD_SELECTORS("TcpTransport::outgoingBufferedDataSize");
    ScopedLock sl(tlock);
    uint32_t r = 0;
    for (ConnectionMap::const_iterator i = connections.begin();
	 i != connections.end(); i++) {
      r += i->second->wsize();
    }
//     traceout << r << Log::end;
    return r;
  } // outgoingBufferedDataSize

  virtual uint32_t outgoingBufferedDataSize(const MaceKey& peer) const {
    ADD_SELECTORS("TcpTransport::outgoingBufferedDataSize");
    ScopedLock l(tlock);
    DestinationMap::const_iterator i = out.find(getNextHop(peer.getMaceAddr()));
    uint32_t r = 0;
    if (i != out.end()) {
      r = i->second->wsize();
    }
//     traceout << r << Log::end;
    return r;
  } // outgoingBufferedDataSize
  
  virtual uint32_t incomingBufferedDataSize(const MaceKey& peer) const {
    ADD_SELECTORS("TcpTransport::incomingBufferedDataSize");
    ScopedLock l(tlock);
    DestinationMap::const_iterator i = in.find(getNextHop(peer.getMaceAddr()));
    uint32_t r = 0;
    if (i != in.end()) {
      r = i->second->rsize();
    }
//     traceout << r << Log::end;
    return r;
  } // incomingBufferedDataSize

  virtual uint32_t incomingMessageQueueSize(const MaceKey& peer) const {
    ADD_SELECTORS("TcpTransport::incomingMessageQueueSize");
    ScopedLock l(tlock);
    DestinationMap::const_iterator i = in.find(getNextHop(peer.getMaceAddr()));
    uint32_t r = 0;
    if (i != in.end()) {
      r = i->second->rqsize();
    }
//     traceout << r << Log::end;
    return r;
  } // incomingMessageQueueSize

  virtual uint32_t incomingMessageQueueSize() const {
    ADD_SELECTORS("TcpTransport::incomingMessageQueueSize");
    ScopedLock sl(tlock);
    uint32_t r = 0;
    for (ConnectionMap::const_iterator i = connections.begin();
	 i != connections.end(); i++) {
      r += i->second->rqsize();
    }
//     traceout << r << Log::end;
    return r;
  } // incomingMessageQueueSize
  
  virtual const BufferStatisticsPtr getStatistics(const MaceKey& peer,
						  TransportServiceClass::Connection::type d) const {
    ADD_SELECTORS("TcpTransport::getStatistics");
    ScopedLock l(tlock);
    if (d == TransportServiceClass::Connection::OUTGOING) {
      DestinationMap::const_iterator i = out.find(getNextHop(peer.getMaceAddr()));
      if (i != out.end()) {
// 	traceout << true << *(i->second->stats()) << Log::end;
	return i->second->stats();
      }
    }
    else {
      assert(d == TransportServiceClass::Connection::INCOMING);
      DestinationMap::const_iterator i = in.find(getNextHop(peer.getMaceAddr()));
      if (i != in.end()) {
// 	traceout << true << *(i->second->stats()) << Log::end;
	return i->second->stats();
      }
    }
//     traceout << false << Log::end;
    return BufferStatisticsPtr();
  } // getStatistics

  virtual void setQueueSize(const MaceKey& peer, uint32_t size,
			    uint32_t threshold = std::numeric_limits<uint32_t>::max()) {
    ScopedLock sl(tlock);
    TcpConnectionPtr c = connect(peer);
    if (c == 0) {
      deferredSetQueueSize.push_back(SetQueueSizeStruct(peer, size, threshold));
      return;
    }
    if (threshold == std::numeric_limits<uint32_t>::max()) {
      threshold = ((size / 2) ? (size / 2) : 1);
    }
    c->setQueueSize(size, threshold);
  } // setQueueSize

  virtual void setWindowSize(const MaceKey& peer, uint64_t microsec) {
    ScopedLock l(tlock);
    TcpConnectionPtr c = connect(peer);
    if (c == 0) {
      deferredSetWindowSize.push_back(SetWindowSizeStruct(peer, microsec));
      return;
    }
    c->setWindowSize(microsec);
  } // setWindowSize

  virtual void requestFlushedNotification(registration_uid_t rid = -1) {
    flushedNotificationRequests.push(rid);
  } // requestFlushedNotification

  virtual void suspendDeliver(registration_uid_t rid) {
    ASSERT(dataHandlers.size() <= 1);
    ScopedLock l(tlock);
    for (DestinationMap::iterator i = in.begin(); i != in.end(); i++) {
      i->second->suspend();
    }
  } // suspendDeliver

  virtual void suspendDeliver(const MaceKey& peer, registration_uid_t rid) {
    ASSERT(dataHandlers.size() <= 1);
    ScopedLock l(tlock);
    DestinationMap::iterator i = in.find(getNextHop(peer.getMaceAddr()));
    if (i != in.end()) {
      i->second->suspend();
    }
  } // suspendDeliver

  virtual void resumeDeliver(registration_uid_t rid) {
    ASSERT(dataHandlers.size() <= 1);
    ScopedLock l(tlock);
    for (DestinationMap::iterator i = in.begin(); i != in.end(); i++) {
      i->second->resume();
    }
  } // resumeDeliver

  virtual void resumeDeliver(const MaceKey& peer, registration_uid_t rid) {
    ASSERT(dataHandlers.size() <= 1);
    ScopedLock l(tlock);
    DestinationMap::iterator i = in.find(getNextHop(peer.getMaceAddr()));
    if (i != in.end()) {
      TcpConnectionPtr c = i->second;
      if (c->isSuspended()) {
	c->resume();
	if (c->isDeliverable()) {
	  deliverable.push(c);
	  signalDeliver();
	}
// 	for (ConnectionQueue::iterator q = suspended.begin(); q != suspended.end(); q++) {
// 	  if (*q == c) {
// 	    deliverable.push(c);
// 	    suspended.erase(q);
// 	    signalDeliver();
// 	    return;
// 	  }
// 	}
      }
    }
  } // resumeDeliver

  virtual void close(const MaceKey& dest);

  virtual mace::string getX509CommonName(const MaceKey& peer) const {
    if (peer == localKey) {
      return localCommonName;
    }

    ScopedLock sl(tlock);

    TcpConnectionPtr p = findConnection(peer);
    if (p) {
      return p->x509CommonName();
    }
    return mace::string();
  } // getX509CommonName

  virtual EVP_PKEY* getPublicKey(const MaceKey& peer) const {
    if (peer == localAddress()) {
      return localPubkey;
    }

    TcpConnectionPtr p = findConnection(peer);
    if (p) {
      return p->publicKey();
    }
    return 0;
  } // getPubkey

  /*** methods internal to transport ***/
  virtual void addSockets(fd_set& rset, fd_set& wset, socket_t& selectMax) {
    //     ADD_SELECTORS("TcpTransport::addSockets");
    ScopedLock sl(tlock);
    //     macedbg(1) << "addSockets() called" << Log::endl;
    if (!running) {
      return;
    }
    BaseTransport::addSockets(rset, wset, selectMax);
    uint64_t buffered = 0;
    for (ConnectionMap::const_iterator i = connections.begin();
	 i != connections.end(); i++) {
      socket_t s = i->first;
      //       macedbg(1) << "Considering socket: " << s << " idsa: " << i->second->id();
      selectMax = std::max(s, selectMax);

      if (i->second->isReadable()) {
	FD_SET(s, &rset);
        //         macedbg(1) << " READABLE ";
      }

      if (i->second->isWriteable()) {
	FD_SET(s, &wset);
        //         macedbg(1) << " WRITEABLE ";
      }

      buffered += i->second->wsize();
      //       macedbg(1) << " bytes: " << i->second->wsize() << Log::endl;
    }

    if (buffered == 0) {
      uint32_t flushcount = 0;
      while (!flushedNotificationRequests.empty()) {
	pendingFlushedNotifications.push(flushedNotificationRequests.front());
	flushedNotificationRequests.pop();
	flushcount++;
      }
      if (flushcount) {
	signalDeliver();
      }
    }
  } // addSockets

  virtual void doIO(CONST_ISSET fd_set& rset, CONST_ISSET fd_set& wset,
		    uint64_t selectTime);

  virtual void closeConnections();

  virtual void freeSockets() { 
    ScopedLock sl(tlock);
    garbageCollectSockets();
  } // freeSockets

  bool runDeliverCondition(ThreadPoolType* tp, uint threadId);
  void runDeliverSetup(ThreadPoolType* tp, uint threadId);
  void runDeliverProcessUnlocked(ThreadPoolType* tp, uint threadId);
  void runDeliverFinish(ThreadPoolType* tp, uint threadId);

protected:

  //These variables added for ThreadPool delivery...
  typedef std::vector<TcpConnectionPtr> ConnectionVector;
  ConnectionVector resend;
  TcpConnectionPtr deliver_c;
  uint deliver_dcount;

  int getSockType() { return SOCK_STREAM; }
  //   virtual void runDeliverThread();
  virtual bool sendData(const MaceAddr& src, const MaceKey& dest,
			const MaceAddr& nextHop, registration_uid_t rid,
			const std::string& ph, const std::string& s,
			bool checkQueueSize, bool rts);
  virtual void notifyError(TcpConnectionPtr c, NetworkHandlerMap& handlers);
  virtual void clearToSend(const MaceKey& dest, registration_uid_t rid, ConnectionStatusHandler* h);
  virtual void notifyFlushed(registration_uid_t rid, ConnectionStatusHandler* h);
  virtual bool canSend(const SockAddr& dest) const {
    DestinationMap::const_iterator i = out.find(dest);
    if (i != out.end()) {
      return i->second->canSend();
    }
    return true;
  } // canSend

  virtual uint32_t availableBufferSize(const SockAddr& dest) const {
    DestinationMap::const_iterator i = out.find(dest);
    if (i == out.end()) {
      return maxQueueSize;
    }
    else if (!i->second->canSend()) {
      return 0;
    }
    return i->second->queueSize() - i->second->wsize();
  } // availableBufferSize

private:
  TcpTransport(TransportCryptoServiceClass::type cryptoFlags = TransportCryptoServiceClass::NONE,
	       bool upcallMessageErrors = false,
	       bool setNodelay = false,
	       uint32_t queueSize = std::numeric_limits<uint32_t>::max(),
	       uint32_t threshold = std::numeric_limits<uint32_t>::max(),
	       int portoffset = std::numeric_limits<int32_t>::max(),
	       MaceAddr addr = SockUtil::NULL_MACEADDR,
	       int bl = SOMAXCONN,
	       SockAddr forwarder = SockUtil::NULL_MSOCKADDR,
	       SockAddr localHost = SockUtil::NULL_MSOCKADDR,
	       bool rejectRouteRts = false,
	       uint32_t maxDeliver = 0,
	       int numDeliveryThreads = 1 );
  void initSSL();
  void accept();
  void flush();
  int socket();

  TcpConnectionPtr connect(const MaceKey& dest) {
    const SockAddr& ma = getNextHop(dest.getMaceAddr());
    return connect(dest, ma);
  }

  TcpConnectionPtr connect(const MaceKey& dest, const SockAddr& ma) {
    bool newConnection = false;
    TcpConnectionPtr c = connect(ma, newConnection);
    if (c == 0) {
      return c;
    }
    if (proxying || newConnection) {
      c->addRemoteKey(dest);
    }
    return c;
  }

  TcpConnectionPtr connect(const SockAddr& dest, bool& newConnection);

  TcpConnectionPtr findConnection(const MaceKey& peer) const {
    const SockAddr& sa = getNextHop(peer.getMaceAddr());
    DestinationMap::const_iterator i = in.find(sa);
    if (i != in.end()) {
      return i->second;
    }
    else {
      for (ConnectionQueue::const_iterator ci = deferredIncomingConnections.begin();
	   ci != deferredIncomingConnections.end(); ci++) {
	if (sa == (*ci)->id()) {
	  return *ci;
	}
      }
    }
    return TcpConnectionPtr();
  } // findConnection

  void garbageCollectSockets();
  void removeClosedSockets();
  void closeBidirectionalConnection(TcpConnectionPtr c);

// private:
//   class ErrorNotification {
//   public:
//     ErrorNotification() { }
//     ErrorNotification(const NodeSet& ns, TransportError::type e, const std::string& m)
//       : nodes(ns), error(e), message(m) { }
//     NodeSet nodes;
//     TransportError::type error;
//     std::string message;
//   };

private:
  bool upcallMessageError;
  bool setNodelay;
  TransportCryptoServiceClass::type cryptoFlags;
  uint32_t maxQueueSize;
  uint32_t queueThreshold;
  bool rejectOnRouteRts;
  const uint32_t MAX_CONSECUTIVE_DELIVER;

  SSL_CTX* ctx;

  mace::string localCommonName;
  EVP_PKEY* localPubkey;

  SockAddr forwardingHost;

  typedef CircularUniqueQueue<TcpConnectionPtr> ConnectionList;
  ConnectionList deliverable;
  ConnectionList sendable;
  ConnectionList errors;
  ConnectionList deliverthr;

  typedef mace::deque<TcpConnectionPtr, mace::SoftState> ConnectionQueue;
  ConnectionQueue deferredIncomingConnections;
//   ConnectionQueue suspended;

  typedef CircularUniqueQueue<registration_uid_t> RegIdList;
  RegIdList flushedNotificationRequests;
  RegIdList pendingFlushedNotifications;

  typedef mace::map<SockAddr, TcpConnectionPtr, mace::SoftState> DestinationMap;
  DestinationMap out;
  DestinationMap in;

  ConnectionMap connections;

  typedef mace::hash_set<int, mace::SoftState> SocketSet;
  SocketSet closed;

  typedef mace::set<SockAddr> SockAddrSet;
  SockAddrSet deferredDeliver;

  typedef mace::deque<SendDataStruct, mace::SoftState> SendDataList;
  SendDataList deferredSendData;

  typedef mace::deque<RequestToSendStruct, mace::SoftState> RTSList;
  RTSList deferredRTS;

  typedef mace::deque<SetQueueSizeStruct, mace::SoftState> SetQueueSizeList;
  SetQueueSizeList deferredSetQueueSize;

  typedef mace::deque<SetWindowSizeStruct, mace::SoftState> SetWindowSizeList;
  SetWindowSizeList deferredSetWindowSize;
  
}; // TcpTransport

namespace TcpTransport_namespace {

class TcpTransportService : public virtual BufferedTransportServiceClass,
			    public virtual TransportCryptoServiceClass,
			    public virtual ConnectionAcceptanceServiceClass {

public:
  TcpTransportService(const TcpTransport_namespace::OptionsMap& m = TcpTransport_namespace::OptionsMap()) {
    ASSERTMSG( ( (!macesim::SimulatorFlags::simulated()) || params::containsKey("ALLOW_LIVE_TRANSPORT_IN_SIMULATOR") ), "Not safe to create TCP Transport when using Simulator.  Set ALLOW_LIVE_TRANSPORT_IN_SIMULATOR to override.");
    t = TcpTransport::create(m);
  }
  virtual ~TcpTransportService();
  bool route(const MaceKey& dest, const std::string& s,
	     registration_uid_t rid = -1) {
    return t->route(dest, s, false, rid);
  }
  const MaceKey&  localAddress() const {
    return t->localAddress();
  }
  bool route(const MaceKey& src, const MaceKey& dest, const std::string& s,
	     registration_uid_t rid = -1) {
    return t->route(src, dest, s, false, rid);
  }
  void requestToSend(const MaceKey& peer, registration_uid_t rid = -1) {
    t->requestToSend(peer, rid);
  }
  bool routeRTS(const MaceKey& dest, const std::string& s, registration_uid_t rid = -1) {
    return t->route(dest, s, true, rid);
  }
  bool routeRTS(const MaceKey& src, const MaceKey& dest, const std::string& s, registration_uid_t rid = -1) {
    return t->route(src, dest, s, true, rid);
  }
  bool canSend(const MaceKey& peer, registration_uid_t rid = -1) {
    return t->canSend(peer);
  }
  uint32_t availableBufferSize(const MaceKey& peer, registration_uid_t rid = -1) {
    return t->availableBufferSize(peer);
  }
  uint32_t outgoingBufferedDataSize(const MaceKey& peer, 
				  registration_uid_t rid = -1) {
    return t->outgoingBufferedDataSize(peer);
  }
  uint32_t incomingBufferedDataSize(const MaceKey& peer, 
				    registration_uid_t rid = -1) {
    return t->incomingBufferedDataSize(peer);
  }
  uint32_t incomingMessageQueueSize(const MaceKey& peer, 
				    registration_uid_t rid = -1) {
    return t->incomingMessageQueueSize(peer);
  }
  uint32_t incomingMessageQueueSize(registration_uid_t rid = -1) {
    return t->incomingMessageQueueSize();
  }
  const BufferStatisticsPtr getStatistics(const MaceKey& peer, Connection::type d,
					  registration_uid_t rid = -1) {
    return t->getStatistics(peer, d);
  }
  uint32_t bufferedDataSize(registration_uid_t rid = -1) {
    return t->bufferedDataSize();
  }
  bool hasBufferedData(registration_uid_t rid = -1) {
    return t->hasBufferedData();
  }
  bool hasOutgoingBufferedData(registration_uid_t rid = -1) {
    return t->hasOutgoingBufferedData();
  }
  uint32_t outgoingBufferedDataSize(registration_uid_t rid = -1) {
    return t->outgoingBufferedDataSize();
  }
  void requestFlushedNotification(registration_uid_t rid = -1) {
    t->requestFlushedNotification(rid);
  }
  void suspendDeliver(registration_uid_t rid = -1) {
    t->suspendDeliver(rid);
  }
  void suspendDeliver(MaceKey const & dest, registration_uid_t rid = -1) {
    t->suspendDeliver(dest, rid);
  }
  void resumeDeliver(registration_uid_t rid = -1) {
    t->resumeDeliver(rid);
  }
  void resumeDeliver(MaceKey const & dest, registration_uid_t rid = -1) {
    t->resumeDeliver(dest, rid);
  }
  mace::string getX509CommonName(const MaceKey& peer, registration_uid_t rid = -1) const {
    return t->getX509CommonName(peer);
  }
  EVP_PKEY* getPublicKey(const MaceKey& peer, registration_uid_t rid = -1) const {
    return t->getPublicKey(peer);
  }
  void setAuthoritativeConnectionAcceptor(registration_uid_t rid = -1) {
    t->setAuthoritativeConnectionAcceptor(rid);
  }
  void setConnectionToken(const mace::string& token, registration_uid_t rid = -1) {
    t->setConnectionToken(token, rid);
  }
  registration_uid_t registerHandler(ReceiveDataHandler& h,
				     registration_uid_t rid = -1, bool isAppHandler = true) {
    return t->registerHandler(h, rid, isAppHandler);
  }
  registration_uid_t registerHandler(NetworkErrorHandler& h,
				     registration_uid_t rid = -1, bool isAppHandler = true) {
    return t->registerHandler(h, rid, isAppHandler);
  }
  registration_uid_t registerHandler(ConnectionStatusHandler& h,
				     registration_uid_t rid = -1, bool isAppHandler = true) {
    return t->registerHandler(h, rid, isAppHandler);
  }
  registration_uid_t registerHandler(ConnectionAcceptanceHandler& h,
				     registration_uid_t rid = -1, bool isAppHandler = true) {
    return t->registerHandler(h, rid, isAppHandler);
  }
  void unregisterHandler(ReceiveDataHandler& h, registration_uid_t rid = -1) {
    t->unregisterHandler(h, rid);
  }
  void unregisterHandler(NetworkErrorHandler& h, registration_uid_t rid = -1) {
    t->unregisterHandler(h, rid);
  }
  void unregisterHandler(ConnectionStatusHandler& h,
			 registration_uid_t rid = -1) {
    t->unregisterHandler(h, rid);
  }
  void unregisterHandler(ConnectionAcceptanceHandler& h,
			 registration_uid_t rid = -1) {
    t->unregisterHandler(h, rid);
  }
  void registerUniqueHandler(ReceiveDataHandler& h) {
    t->registerUniqueHandler(h);
  }
  void registerUniqueHandler(NetworkErrorHandler& h) {
    t->registerUniqueHandler(h);
  }
  void registerUniqueHandler(ConnectionStatusHandler& h) {
    t->registerUniqueHandler(h);
  }
  void registerUniqueHandler(ConnectionAcceptanceHandler& h) {
    t->registerUniqueHandler(h);
  }
  void maceInit() { t->maceInit(); }
  void maceExit() { t->maceExit(); }

private:
  TcpTransportPtr t;

}; // TcpTransportService

} // TcpTransport_namespace
  
#endif // TCP_TRANSPORT_H
