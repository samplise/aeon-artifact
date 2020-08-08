/* 
 * BaseTransport.cc : part of the Mace toolkit for building distributed systems
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
#include "Log.h"
#include "Accumulator.h"
#include "NumberGen.h"
#include "BaseTransport.h"
#include "params.h"
#include "ContextMapping.h"

const int BaseTransport::TRANSPORT_TRACE_DELIVER;
const int BaseTransport::TRANSPORT_TRACE_ERROR;
const int BaseTransport::TRANSPORT_TRACE_CTS;
const int BaseTransport::TRANSPORT_TRACE_FLUSH;

BaseTransport::BaseTransport(int portoff, MaceAddr maddr, int bl, SockAddr fw,
			     SockAddr local, int num_delivery_threads) :
  deliverState(WAITING),
  initCount(0),
  portOffset( (portoff == INT_MAX) ? NumberGen::Instance(NumberGen::PORT)->GetVal() : portoff),
  port( Util::getPort() + portOffset ),
  localAddr(maddr == SockUtil:: NULL_MACEADDR ? Util::getMaceAddr() : maddr),
  saddr(localAddr.local.addr),
  localKey(MaceKey(ipv4, localAddr)),
  forwardingHost(fw),
  localHost(local),
  srcAddr(
        ( !localHost.isNull() ?
          MaceAddr(localHost, SockUtil::NULL_MSOCKADDR) :
          ( !forwardingHost.isNull() ?
            MaceAddr(forwardingHost, SockUtil::NULL_MSOCKADDR) :
            localAddr
          )
        )
  ),
  srcKey(
    ( mace::ContextMapping::getVirtualNodeMaceKey() == MaceKey::null )?
        ( !localHost.isNull() ?
          MaceKey(ipv4, localHost.addr, localHost.port) :
          ( !forwardingHost.isNull() ?
            MaceKey(ipv4, forwardingHost.addr, forwardingHost.port) :
            localKey
          )
        ): mace::ContextMapping::getVirtualNodeMaceKey()
  ),
  numDeliveryThreads( (num_delivery_threads == INT_MAX)? 1 : num_delivery_threads),
  backlog(bl),
  transportSocket(0),
  starting(true),
  running(false),
  shuttingDown(false),
  doClose(false),
  proxying(false),
  disableTranslations(false),
  authoritativeConnectionAcceptor(0),
  routeTime(0),
  sendDataTime(0),
  doIOTime(0)
{

  ADD_SELECTORS("BaseTransport::BaseTransport");

  SockUtil::init();

  //if (maddr == SockUtil::NULL_MACEADDR) {
  //  maddr = Util::getMaceAddr();
  //}

  if ((saddr & 0x000000ff) == 0x7f) {
    if (params::get(params::MACE_WARN_LOOPBACK, true)) {
      macewarn << "The local address " << saddr << " uses the loopback." << Log::endl;
    }
    if (! params::get<bool>(params::MACE_ADDRESS_ALLOW_LOOPBACK, false)) {
      ABORT("Loopback address detected, but parameter MACE_ADDRESS_ALLOW_LOOPBACK not set to true");
    }
  }

  //if (portoff == INT_MAX) {
  //  portOffset = NumberGen::Instance(NumberGen::PORT)->GetVal();
  //}
  //else {
  //  portOffset = portoff;
  //}
  maceout << "port offset=" << (int)portOffset << Log::endl;

  //if (num_delivery_threads == INT_MAX) {
  //  numDeliveryThreads = 1;
  //}
  //else {
  //  numDeliveryThreads = num_delivery_threads;
  //}
  maceout << "number of delivery thread=" << (int)num_delivery_threads << Log::endl;


  //   saddr = maddr.local.addr;
  //   port = Util::getPort() + portOffset;

  //   localAddr = maddr;
  //   localKey = MaceKey(ipv4, localAddr);

  // if (!localHost.isNull()) {
  //   srcAddr = MaceAddr(localHost, SockUtil::NULL_MSOCKADDR);
  //   srcKey = MaceKey(ipv4, localHost.addr, localHost.port);
  // }
  // else if (!forwardingHost.isNull()) {
  //   srcAddr = MaceAddr(forwardingHost, SockUtil::NULL_MSOCKADDR);
  //   srcKey = MaceKey(ipv4, forwardingHost.addr, forwardingHost.port);
  // }
  // else {
  //   srcAddr = localAddr;
  //   srcKey = localKey;
  // }

  maceout << "listening on " << Util::getAddrString(localAddr)
	  << " (port)" << port
	  << " srcAddr=" << srcAddr << " srcKey=" << srcKey
	  << Log::endl;

  if (params::get<bool>(params::MACE_TRANSPORT_DISABLE_TRANSLATION, false)) {
    disableTranslations = true;
  }

  pthread_mutex_init(&tlock, 0);
  pthread_mutex_init(&dlock, 0);
  pthread_mutex_init(&conlock, 0);

} // BaseTransport

BaseTransport::~BaseTransport() {
  pthread_mutex_destroy(&tlock);
  pthread_mutex_destroy(&dlock);
  pthread_mutex_destroy(&conlock);
  delete tpptr; 
  //   delete dt;
} // ~BaseTransport

void* BaseTransport::startDeliverThread(void* arg) {
  //In the end, this thread's purpose will be to wait until the thread pool
  //completes.  There may be a better design, but since running needs to be set
  //to false and doClose to true, I'm not immediately sure what it is.
  
  //Selectors only needed if logging statements exist.
  //ADD_SELECTORS("BaseTransport::startDeliverThread");

  BaseTransport* transport = (BaseTransport*)arg;

  // Starting up thread pool and delivery threads.
  transport->setupThreadPool();

  //transport->runDeliverThread();

  transport->killThreadPool();

  transport->running = false;
  transport->doClose = true;

  return 0;
} // startDeliverThread

void BaseTransport::run(uint32_t i) {
  
}

void BaseTransport::run() throw(SocketException) {
  ScopedLock sl(tlock);
  if (running) {
    return;
  }

  // if the transport is not starting, then it needs to be re-added to
  // the scheduler...for now, do not allow transport to be re-started
  ASSERT(starting);

  setupSocket();

  running = true;
  starting = false;
  shuttingDown = false;

  ASSERT(transportSocket);

  if (getSockType() == SOCK_STREAM) {
    if (listen(transportSocket, backlog) < 0) {
      Log::perror("listen");
      ABORT("listen");
    }
  }

  runNewThread(&deliverThread, BaseTransport::startDeliverThread, this, 0);

} // run

void BaseTransport::shutdown() {
  ADD_SELECTORS("BaseTransport::shutdown");
  ScopedLock sl(tlock);
  if (shuttingDown || !running) {
    return;
  }

  maceout << "transport shutting down" << Log::endl;
  shuttingDown = true;
  signalDeliver();

  maceout << "routeTime=" << routeTime
	  << " sendDataTime=" << sendDataTime
	  << " doIOTime=" << doIOTime
	  << Log::endl;
} // shutdown

void BaseTransport::maceInit() {
  initCount++;
  run();
} // maceInit

void BaseTransport::maceExit() {
  initCount--;
  if (initCount == 0) {
    shutdown();
  }
} // maceExit

void BaseTransport::setupSocket() {
  ADD_SELECTORS("BaseTransport::setupSocket");

  transportSocket = socket(AF_INET, getSockType(), 0);
  if (transportSocket < 0) {
    Log::perror("socket");
    ABORT("socket");
  }

  //   macedbg(1) << "transportSocket: " << transportSocket << Log::endl;

  int n = 1;
  if (setsockopt(transportSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0) {
    Log::perror("setsockopt");
    ABORT("setsockopt");
  }

  struct sockaddr_in sa;
  uint32_t bindaddr = INADDR_ANY;
  if (params::containsKey(params::MACE_BIND_LOCAL_ADDRESS)) {
    if (params::get<int>(params::MACE_BIND_LOCAL_ADDRESS)) {
      bindaddr = saddr;
    }
  }

  struct in_addr ia;
  ia.s_addr = bindaddr;
  maceout << "binding to " << inet_ntoa(ia) << ":" << port << Log::endl;

  SockUtil::fillSockAddr(bindaddr, port, sa);
  if (bind(transportSocket, (struct sockaddr*)&sa, sizeof(sa)) < 0) {
    Log::perror("bind");
    ::close(transportSocket);
    std::ostringstream m;
    m << "BaseTransport::setupSocket: error binding to " << bindaddr << ":" << port
      << ": " << Util::getErrorString(SockUtil::getErrno());
    throw BindException(m.str());
  }

  SockUtil::setNonblock(transportSocket);
} // setupSocket

void BaseTransport::closeSockets() {
  ADD_SELECTORS("BaseTransport::closeSockets");
  ScopedLock sl(tlock);

  if (!doClose) {
    return;
  }

  closeConnections();

  doClose = false;

  if (transportSocket) {
    ::close(transportSocket);
    transportSocket = 0;
  }
  maceout << "halted transport" << Log::endl;
} // closeConnections

void BaseTransport::deliverDataSetup(ThreadPoolType* tp, DeliveryData& data) {
  ADD_SELECTORS("BaseTransport::deliverDataSetup");

  // chuangw: Why should it unlock during deserialization? improve parallelism.
  size_t hdrsz = data.shdr.size();
  size_t flag_pos = hdrsz - sizeof(uint32_t) - sizeof(uint8_t);
  if( !( data.shdr[ flag_pos  ] & TransportHeader::INTERNALMSG ) ){
    ThreadStructure::newMessageTicket();
  }
  tp->unlock();
  try {
    istringstream in(data.shdr);
    data.hdr.deserialize(in);
  }
  catch (const Exception& e) {
    Log::err() << "Transport deliver deserialization exception: " << e << Log::endl;
    return;
  }
  tp->lock();

  DataHandlerMap::iterator i = dataHandlers.find(data.hdr.rid);
  if (i == dataHandlers.end()) { data.dataHandler = NULL; }
  else { data.dataHandler = i->second; }
}

void BaseTransport::deliverData(DeliveryData& data) {
  ADD_SELECTORS("BaseTransport::deliverData");

  static Accumulator* recvaccum = Accumulator::Instance(Accumulator::TRANSPORT_RECV);

  data.remoteKey = MaceKey(ipv4, data.hdr.src);

  //Note: suspended can get awkward with multiple deliver threads.  However,
  //since there could be races between app threads and the deliver thread, this
  //is not really much of a change.
  if (data.suspended.contains(data.remoteKey)) {
    mace::AgentLock::nullTicket();
    return;
  }

  if (data.hdr.dest.proxy == localAddr.local) {
    static const std::string ph; // empty string to pass in as the pipeline header -- s already has been pipeline processed
    // this is the proxy node, forward the message
    // Q: XXX do we ping the pipeline here?
    // Does this need the deliver lock?
      
    sendData(data.hdr.src, MaceKey(ipv4, data.hdr.dest), data.hdr.dest, data.hdr.rid, ph, data.s, false, false);
    mace::AgentLock::nullTicket();
    return;
  }

  // Handler lookup needs the lock.
  if (data.dataHandler == NULL) {
    Log::err() << "BaseTransport::deliverData: no handler registered with "
	       << data.hdr.rid << Log::endl;
    mace::AgentLock::nullTicket();
    return;
  }

  if (!disableTranslations && (data.hdr.dest != localAddr)) {
    lock();
    translations[getNextHop(data.hdr.src)] = data.hdr.dest;
    unlock();
  }

  MaceKey dest(ipv4, data.hdr.dest);

  // accumulate has its own lock.
  recvaccum->accumulate(data.s.size());
  // XXX !!!
  //       sha1 hash;
  //       HashUtil::computeSha1(s, hash);
  //   maceout << "delivering " << Log::toHex(s) << " from " << src << Log::endl;
  if (!macedbg(1).isNoop()) {
    macedbg(1) << "delivering message from " << data.remoteKey << " size " << data.s.size() << Log::endl;
  }
//   maceout << "delivering " << s.size() << Log::endl;
//   traceout << TRANSPORT_TRACE_ERROR << src << dest << s << hdr.rid << Log::end;

  //Probably no lock needed.
  if (pipeline) {
    pipeline->deliverData(data.remoteKey, data.s, data.hdr.rid);
  }

  try {
    data.dataHandler->deliver(data.remoteKey, dest, data.s, data.hdr.rid);
  } 
  catch(const ExitedException& e) {
    Log::err() << "BaseTransport delivery caught exception for exited service: " << e << Log::endl;
    //This try/catch may no longer be necessary... ?
  }

  //   mace::AgentLock::possiblyNullTicket();

  return;
} // deliverData

bool BaseTransport::acceptConnection(const MaceAddr& maddr,
				     const mace::string& t) {
  if (authoritativeConnectionAcceptor == 0) {
    return true;
  }
  return authoritativeConnectionAcceptor->acceptConnection(
    MaceKey(ipv4, maddr), t, authoritativeConnectionAcceptorRid);
} // acceptConnection

void BaseTransport::setAuthoritativeConnectionAcceptor(registration_uid_t rid) {
  ASSERT(authoritativeConnectionAcceptor == 0);
  authoritativeConnectionAcceptorRid = rid;
  AcceptanceHandlerMap::iterator i = acceptanceHandlers.find(rid);
  ASSERT(i != acceptanceHandlers.end());
  authoritativeConnectionAcceptor = i->second;
} // setAuthoritativeConnectionAcceptor

void BaseTransport::setConnectionToken(const mace::string& token,
				       registration_uid_t rid) {
  ASSERT(authoritativeConnectionAcceptor != 0);
  ASSERT(authoritativeConnectionAcceptorRid == rid);
  connectionToken = token;
} // setConnectionToken

registration_uid_t BaseTransport::registerHandler(ReceiveDataHandler& h,
						  registration_uid_t rid, bool isAppHandler) {
  if (rid == -1) {
    rid = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal();
  }
  dataHandlers[rid] = &h;
  return rid;
} // registerHandler

void BaseTransport::registerUniqueHandler(ReceiveDataHandler& h) {
  dataHandlers[-1] = &h;
} // registerUniqueHandler

void BaseTransport::unregisterHandler(ReceiveDataHandler& h, registration_uid_t rid) {
  dataUnreg.push(rid);
}


registration_uid_t BaseTransport::registerHandler(NetworkErrorHandler& h,
						  registration_uid_t rid, bool isAppHandler) {
  if (rid == -1) {
    rid = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal();
  }
  errorHandlers[rid] = &h;
  return rid;
} // registerHandler

void BaseTransport::registerUniqueHandler(NetworkErrorHandler& h) {
  errorHandlers[-1] = &h;
} // registerUniqueHandler

void BaseTransport::unregisterHandler(NetworkErrorHandler& h, registration_uid_t rid) {
  errorUnreg.push(rid);
}

registration_uid_t BaseTransport::registerHandler(ConnectionStatusHandler& h,
						  registration_uid_t rid, bool isAppHandler) {
  if (rid == -1) {
    rid = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal();
  }
  connectionHandlers[rid] = &h;
  return rid;
}

void BaseTransport::registerUniqueHandler(ConnectionStatusHandler& h) {
  connectionHandlers[-1] = &h;
}

void BaseTransport::unregisterHandler(ConnectionStatusHandler& h, registration_uid_t rid) {
  connectionUnreg.push(rid);
}

registration_uid_t BaseTransport::registerHandler(ConnectionAcceptanceHandler& h,
						  registration_uid_t rid, bool isAppHandler) {
  if (rid == -1) {
    rid = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal();
  }
  acceptanceHandlers[rid] = &h;
  return rid;
}

void BaseTransport::registerUniqueHandler(ConnectionAcceptanceHandler& h) {
  acceptanceHandlers[-1] = &h;
}

void BaseTransport::unregisterHandler(ConnectionAcceptanceHandler& h, registration_uid_t rid) {
  acceptanceUnreg.push(rid);
}

void BaseTransport::unregisterHandlers() {
  while (!dataUnreg.empty()) {
    dataHandlers.erase(dataUnreg.front());
    dataUnreg.pop();
  }
  while (!errorUnreg.empty()) {
    errorHandlers.erase(errorUnreg.front());
    errorUnreg.pop();
  }
  while (!connectionUnreg.empty()) {
    connectionHandlers.erase(connectionUnreg.front());
    connectionUnreg.pop();
  }
  while (!acceptanceUnreg.empty()) {
    acceptanceHandlers.erase(acceptanceUnreg.front());
    acceptanceUnreg.pop();
  }
  
} // unregisterHandlers

