/* 
 * UdpTransport.h : part of the Mace toolkit for building distributed systems
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
#ifndef UDP_TRANSPORT_H
#define UDP_TRANSPORT_H

#include "CircularQueueList.h"

#include "BaseTransport.h"
#include "SimulatorBasics.h"

class UdpTransport;

typedef boost::shared_ptr<UdpTransport> UdpTransportPtr;

class UdpTransport : public virtual BaseTransport {
public:
  typedef boost::shared_ptr<std::string> StringPtr;
  typedef CircularQueueList<StringPtr> StringPtrQueue;

public:
  static UdpTransportPtr create(int portOffset);
  virtual ~UdpTransport();

  using BaseTransport::route;
  virtual bool route(const MaceKey& dest, const std::string& s, registration_uid_t rid) {
    return BaseTransport::route(dest, s, false, rid);
  } // route

  virtual void doIO(CONST_ISSET fd_set& rset, CONST_ISSET fd_set& wset, uint64_t st);
  virtual void suspendDeliver(MaceKey const & dest, registration_uid_t rid) {
    ASSERT(dataHandlers.size() <= 1);
    suspended.insert(dest);
  }
  virtual void resumeDeliver(MaceKey const & dest, registration_uid_t rid) {
    ASSERT(dataHandlers.size() <= 1);
    suspended.erase(dest);
  }

  virtual void closeConnections() { }
  virtual void freeSockets() { }

  bool runDeliverCondition(ThreadPoolType* tp, uint threadId);
  void runDeliverSetup(ThreadPoolType* tp, uint threadId);
  void runDeliverProcessUnlocked(ThreadPoolType* tp, uint threadId);
  void runDeliverFinish(ThreadPoolType* tp, uint threadId);

protected:
  int getSockType() { return SOCK_DGRAM; }
  //   virtual void runDeliverThread();
  virtual bool sendData(const MaceAddr& src, const MaceKey& dest,
			const MaceAddr& nextHop, registration_uid_t rid,
			const std::string& ph, const std::string& s, bool checkQueueSize, bool rts);

private:
  UdpTransport(int portOffset);

public:
  static const size_t MAX_MESSAGE_SIZE;
  static const size_t MAX_PAYLOAD_SIZE;

private:
  uint64_t rc;
  uint64_t wc;

  char* rbuf;

  NodeSet suspended;
  StringPtrQueue rhq;
  StringPtrQueue rq;
  StringPtrQueue pipq;
}; // UdpTransport

namespace UdpTransport_namespace {

class UdpTransportService : public virtual TransportServiceClass {
public:
  UdpTransportService(int portOffset = INT_MAX) {
    ASSERTMSG( ( (!macesim::SimulatorFlags::simulated()) || params::containsKey("ALLOW_LIVE_TRANSPORT_IN_SIMULATOR") ), "Not safe to create UDP Transport when using Simulator.  Set ALLOW_LIVE_TRANSPORT_IN_SIMULATOR to override.");
    t = UdpTransport::create(portOffset);
  }
  virtual ~UdpTransportService();
  bool route(const MaceKey& dest, const std::string& s, registration_uid_t rid = -1) {
    return t->route(dest, s, rid);
  }
  const MaceKey&  localAddress() const {
    return t->localAddress();
  }
  void suspendDeliver(MaceKey const & dest, registration_uid_t rid = -1) {
    t->suspendDeliver(dest, rid);
  }
  void resumeDeliver(MaceKey const & dest, registration_uid_t rid = -1) {
    t->resumeDeliver(dest, rid);
  }
  registration_uid_t registerHandler(ReceiveDataHandler& h, registration_uid_t rid = -1, bool isAppHandler = true) {
    return t->registerHandler(h, rid, isAppHandler);
  }
  registration_uid_t registerHandler(NetworkErrorHandler& h,
				     registration_uid_t rid = -1, bool isAppHandler = true) {
    return t->registerHandler(h, rid, isAppHandler);
  }
  void unregisterHandler(ReceiveDataHandler& h, registration_uid_t rid = -1) {
    t->unregisterHandler(h, rid);
  }
  void unregisterHandler(NetworkErrorHandler& h, registration_uid_t rid = -1) {
    t->unregisterHandler(h, rid);
  }
  void registerUniqueHandler(ReceiveDataHandler& h) { t->registerUniqueHandler(h); }
  void registerUniqueHandler(NetworkErrorHandler& h) { t->registerUniqueHandler(h); }
  void maceInit() { t->maceInit(); }
  void maceExit() { t->maceExit(); }

private:
  UdpTransportPtr t;

}; // UdpTransportService

} // UdpTransport_namespace


#endif // UDP_TRANSPORT_H
