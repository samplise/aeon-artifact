/* 
 * SimulatorTCPCommon.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, James W. Anderson
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
#include <math.h>
#include "Util.h"
#include "Log.h"
#include "SimulatorTCPCommon.h"
#include "NumberGen.h"
// #include "BaseTransport.h"
#include "Accumulator.h"
#include "mace_constants.h"
#include "MaceTypes.h"
#include "mace-macros.h"
#include "ServiceConfig.h"

using std::string;

int SimulatorTCPCommon_load_protocol() {
  return 0;
}

namespace SimulatorTCP_namespace {

  // int TCPService::PT_tcp;

  //   SimulatorTCPCommonService::SimulatorTCPCommonService(uint32_t queue_size, uint16_t port, int node,
  //                                                        bool messageErrors, int fwd)
    SimulatorTCPCommonService::SimulatorTCPCommonService(uint16_t port, int fwd, bool shared)
    : queueSize(mace::ServiceConfig<void*>::get<uint16_t>("SimulatorTCPCommon.queueSize", 20)), localPort((port == std::numeric_limits<uint16_t>::max()? Util::getPort() + NumberGen::Instance(NumberGen::PORT)->GetVal() : port)), localNode(SimCommon::getCurrentNode()),
      upcallMessageErrors(mace::ServiceConfig<void*>::get<bool>("SimulatorTCPCommon.upcallMessageErrors", false)), forwarder(fwd < 0 ? localNode : fwd),
      listening(false),
      rtsRequests(SimCommon::getNumNodes()),
      flushEventQueued(0), queuedMessages(SimCommon::getNumNodes()),
      openSockets(SimCommon::getNumNodes()),
      remoteErrors(SimCommon::getNumNodes()),
      localErrors(SimCommon::getNumNodes()),
      pendingDestNotReady(SimCommon::getNumNodes()) {
    ADD_FUNC_SELECTORS;
    ASSERT(fwd != localNode);
//    forwarder = (forwarder < 0 ? localNode : forwarder);

    maceout << "local_address " << SimCommon::getMaceKey(localNode) << " port " << localPort << " queue_size " << queueSize << Log::endl;

    if (shared) {
        mace::ServiceFactory<TransportServiceClass>::registerInstance("SimulatorTCP", this);
        mace::ServiceFactory<BufferedTransportServiceClass>::registerInstance("SimulatorTCP", this);
        mace::ServiceFactory<BandwidthTransportServiceClass>::registerInstance("SimulatorTCP", this);
    }
    ServiceClass::addToServiceList(*this);

    //     static bool defaultTest = params::get<bool>("AutoTestProperties", 0);
    static bool testThisService = params::get<bool>("AutoTestProperties.SimulatorTCP", 0);
    if (testThisService) {
      static int testId = NumberGen::Instance(NumberGen::TEST_ID)->GetVal();
      macesim::SpecificTestProperties<SimulatorTCPCommonService>::registerInstance(testId, this);
    }

  }

  void SimulatorTCPCommonService::maceInit() { 
    listening = true;
  }

  SimulatorTCPCommonService::~SimulatorTCPCommonService() { 
    mace::ServiceFactory<TransportServiceClass>::unregisterInstance("SimulatorTCP", this);
    mace::ServiceFactory<BufferedTransportServiceClass>::unregisterInstance("SimulatorTCP", this);
    mace::ServiceFactory<BandwidthTransportServiceClass>::unregisterInstance("SimulatorTCP", this);
  }

  bool SimulatorTCPCommonService::isListening() const {
    return listening;
  }

  size_t SimulatorTCPCommonService::queuedDataSize() const {
    size_t s = 0;
    for (MessageQueueMap::const_iterator i = queuedMessages.begin(); i != queuedMessages.end(); i++) {
      s += i->size();
    }
    return s;
  }

  void SimulatorTCPCommonService::startSegment(const MaceKey& peer, BandwidthDirection bd, registration_uid_t rid) {

  }


  void SimulatorTCPCommonService::endSegment(const MaceKey& peer, BandwidthDirection bd, registration_uid_t rid) {

  }


  double SimulatorTCPCommonService::getBandwidth(const MaceKey& peer, BandwidthDirection bd, registration_uid_t rid) {
    BufferStatisticsPtr bufStat = getStatistics(peer, bd);
    if (bufStat != NULL) {
      return bufStat->windowSum;
    }
    else {
      return 0;
    }
  }

  int SimulatorTCPCommonService::queued(const MaceKey& peer, registration_uid_t rid) {
    int n = SimCommon::getNode(peer);

    return queuedMessages[n].size();
  }

  void SimulatorTCPCommonService::maceReset() {
    // Handling network state when this node is rebooted
    listening = false;
    handlers.clear();
    network_handlers.clear();

    // Check and handle any pending error messages
    for (int i = 0; i < SimCommon::getNumNodes(); i++) {
      if (openSockets[i]) {
        ASSERT(!pendingDestNotReady[i]);
        if (i == localNode) {
          clearQueue(i, 0, false, true);
          remoteErrors[i] = false;
          localErrors[i] = false;
        } else if (localErrors[i]) {
          if (remoteErrors[i]) {
            // 1,1,1,1,1,1
            clearQueue(i, 0, false, false);
            netSignal(localNode, i, localPort, SimulatorTransport::SOURCE_RESET);
            remoteErrors[i] = false;
          } else {
            // 1,0,1,0,1,1
            ASSERT(0);
          }
        } else {
          if (remoteErrors[i]) {
            // 0,1,1,1,0,0
            clearQueue(i, 0, false, false);
            netSignal(localNode, i, localPort, SimulatorTransport::SOURCE_RESET);
            remoteErrors[i] = false;
          } else {
            // 0,0,1,0,0,1
            clearQueue(i, 0, false, true);
            localErrors[i] = true;
            netSignal(localNode, i, localPort, SimulatorTransport::SOURCE_RESET);
            uint32_t msgId = getNetMessageId();

            queuedMessages[i].push_front(createMessageSourceReset(i, msgId));
          }
        }

      } else { // !openSockets[i]
        if (pendingDestNotReady[i]) {
          ASSERT(!localErrors[i] && !remoteErrors[i]);
          ASSERT(removeDestNotReadyEvent(i));
          clearQueue(i, 0, false, false);
        } else if (localErrors[i]) {
          if (remoteErrors[i]) {
            // 1,1,0,1,1,0
            ASSERT(0);
          } else {
            // 1,0,0,0,1,1
            clearQueue(i, 0, false, true);
          }
        } else {
          if (remoteErrors[i]) {
            // 0,1,0,1,0,0
            ASSERT(0);
          } else {
            // 0,0,0,0,0,0
            clearQueue(i, 0, false, true);
          }
        }
      }

    }
  }

  registration_uid_t SimulatorTCPCommonService::registerHandler(ReceiveDataHandler& handler, registration_uid_t handlerUid) {
    if (handlerUid == -1) {
      handlerUid = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal();
    }
    handlers[handlerUid] = &handler;
    return handlerUid;
  }

  registration_uid_t SimulatorTCPCommonService::registerHandler(ConnectionStatusHandler& handler, registration_uid_t handlerUid) {
    if (handlerUid == -1) {
      handlerUid = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal();
    }
    connectionHandlers[handlerUid] = &handler;
    return handlerUid;
  }

  registration_uid_t SimulatorTCPCommonService::registerHandler(NetworkErrorHandler& handler, registration_uid_t handlerUid) {
    if (handlerUid == -1) {
      handlerUid = NumberGen::Instance(NumberGen::HANDLER_UID)->GetVal();
    }
    network_handlers[handlerUid] = &handler;
    return handlerUid;
  }

  void SimulatorTCPCommonService::requestToSend(const MaceKey& dest, registration_uid_t rid) {
    int n = SimCommon::getNode(dest);
    rtsRequests[n].push_back(rid);
    addCTSEvent(n);
  }

  bool SimulatorTCPCommonService::routeRTS(const MaceKey& dest, const std::string& s, registration_uid_t rid) {
    int n = SimCommon::getNode(dest);
    bool r = route(dest, s, rid);
    if (!r) {
      rtsRequests[n].push_back(rid);
      addCTSEvent(n);
    }
    return r;
  }

  void SimulatorTCPCommonService::requestFlushedNotification(registration_uid_t rid) {
    flushRequests.insert(rid);
    addFlushedEvent();
  }


  std::string SimulatorTCPCommonService::simulateFlushed() {
    for (RegSet::const_iterator i = flushRequests.begin(); i != flushRequests.end(); i++) {
      ConnectionHandlerMap::iterator k = connectionHandlers.find(*i);
      ASSERT(k != connectionHandlers.end());
      k->second->notifyFlushed(*i);
    }
    flushRequests.clear();
    flushEventQueued = 0;
    return "";
  }

  std::string SimulatorTCPCommonService::simulateCTS(int node) {
    RegList& l = rtsRequests[node];
    rtsEventsQueued.erase(node);
    registration_uid_t rid = l.front();
    l.pop_front();
    
    ConnectionHandlerMap::iterator k = connectionHandlers.find(rid);
    ASSERT(k != connectionHandlers.end());
    k->second->clearToSend(SimCommon::getMaceKey(node), rid);
    
    addCTSEvent(node);
    return "";
  }

  bool SimulatorTCPCommonService::hasFlushed() const {
    return !flushRequests.empty() && queuedDataSize() == 0;
  }

  bool SimulatorTCPCommonService::hasCTS(int node) const {
    return (!rtsRequests[node].empty() && queuedMessages[node].size() < queueSize);
  }

  uint32_t SimulatorTCPCommonService::outgoingBufferedDataSize(const MaceKey& peer, registration_uid_t registrationUid) const {
    return queuedDataSize();
  }

  bool SimulatorTCPCommonService::canSend(const MaceKey& peer, registration_uid_t registrationUid) const {
    int destNode = SimCommon::getNode(peer);

    return (queuedMessages[destNode].size() < queueSize);
  }

  bool SimulatorTCPCommonService::isAvailableMessage(int destNode) const {
    const std::deque<SimulatorMessage>& deque = queuedMessages[destNode];
    return !deque.empty() && (localErrors[destNode] || !remoteErrors[destNode]);
  }

  SimulatorMessage SimulatorTCPCommonService::getMessage(int destNode) {
    ADD_SELECTORS("SimulatorTCPCommonService::getMessage");
    ASSERT(isAvailableMessage(destNode));
    SimulatorMessage str = *queuedMessages[destNode].begin();
    if (str.messageType == SimulatorMessage::MESSAGE) {
      openSocket(destNode);
    }
    else if (str.messageType == SimulatorMessage::READ_ERROR) {
      localErrors[destNode] = false;
    }
    queuedMessages[destNode].pop_front();

    addFlushedEvent();
    addCTSEvent(destNode);
    enqueueEvent(destNode); // Update flags, events, etc.
    
    return str;
  }

  int SimulatorTCPCommonService::getPort() const { return localPort; }
  const MaceKey& SimulatorTCPCommonService::localAddress() const { 
    //     ADD_FUNC_SELECTORS;
    //     maceLog("called getLocalAddress");
    return SimCommon::getMaceKey(localNode);  //XXX is this localNode, or forwarder.
  }

  std::string SimulatorTCPCommonService::simulateEvent(const SimulatorMessage& msg) {
    ASSERT(msg.destination == localNode);
    if (msg.messageType == SimulatorMessage::MESSAGE) {
      ASSERT(!pendingDestNotReady[msg.source]);
      // Decide on socket failures, deliver the message, etc.
      recv(msg);

    } else if (msg.messageType == SimulatorMessage::DEST_NOT_READY) {
      ASSERT(pendingDestNotReady[msg.source]);
      ASSERT(!openSockets[msg.source]);
      pendingDestNotReady[msg.source] = 0;
      transport_error(msg.source, TransportError::CONNECT_ERROR);

      // Signal the other node that I am now ready. He can queue messages if he wants (MC)
      netSignal(localNode, msg.source, localPort, SimulatorTransport::DEST_READY);

    } else { //msg.messageType == READ_ERROR 
      ASSERT(msg.messageType == SimulatorMessage::READ_ERROR);
      ASSERT(openSockets[msg.source]);
      ASSERT(remoteErrors[msg.source]);
      remoteErrors[msg.source] = false;
      transport_error(msg.source, TransportError::READ_ERROR);
    }
    return "";
  }

  void SimulatorTCPCommonService::recv(const SimulatorMessage& msg) {
    ADD_SELECTORS("SimulatorTCPCommonService::recv::pkt");
    static int netErrorWeight = params::get("NET_ERROR_WEIGHT", 0);
//    static bool allowErrors = params::get("USE_NET_ERRORS",false);
    // XXX: move to modelchecker and simulator specific code?
    static int numFailures = params::get("SIM_NUM_NET_ERRORS", 0);
    
    maceout << "from=" << SimCommon::getMaceKey(msg.source).toString() << " dsize=" << msg.msg.size() << Log::endl;
    int src = (msg.forwarder == localNode ? msg.source : msg.forwarder);
    openSocket(src);

    if (!(remoteErrors[src] || localErrors[src]) /*&& allowErrors*/ && src != localNode
        && (numFailures < 0 || SimNetworkCommon::getFailureCount() < numFailures)) { 
      maceout << "Socket error following message between " << localNode
        << " and dest " << msg.source << " on port " << localPort
        << "? (0-no, 1-yes) " << Log::endl;
      // In search mode, either error or non-error may occur.  In gusto mode, no errors may occur
//      if (RandomUtil::randInt(2, 1, 0)) { 
      if (RandomUtil::randInt(2, 100, netErrorWeight)) { 
        maceerr << "Note: message followed by error between " << localNode
          << " and dest " << msg.source << " on port " << localPort << Log::endl;
        // Add the error to the local queue
        signal(src, SimulatorTransport::BIDI_READ_ERROR);
        // Add the error to the remote queue
        netSignal(localNode, src, localPort, SimulatorTransport::BIDI_READ_ERROR);

        SimNetworkCommon::incrementFailureCount();
      }
    } else {
      macedbg(1) << "Not considering read error: localError " << localErrors[src]
        << " remoteErrors " << remoteErrors[src] << " numFailures " << numFailures << Log::endl;
//      macedbg(1) << "Not considering read error: localError " << localErrors[src]
//        << " remoteErrors " << remoteErrors[src] << " allowErrors " << allowErrors << Log::endl;
    }

    // Find the deliver handler and deliver the message
    ReceiveHandlerMap::iterator i = handlers.find(msg.regId);
    if (i == handlers.end()) {
      macewarn << "Could not find handler with id " << msg.regId
        << " for message " << msg.messageId << " : dropping packet" << Log::endl;
      return;
    }
    i->second->deliver(SimCommon::getMaceKey(msg.source), SimCommon::getMaceKey(msg.destination),
        msg.msg, msg.regId);
  }

  void SimulatorTCPCommonService::signal(int other, SimulatorTransport::TransportSignal sig) {
    ADD_SELECTORS("SimulatorTCPCommonService::signal");
    
    switch(sig) {
      case SimulatorTransport::SOURCE_RESET:
        {
          if (localErrors[other]) {
            MessageQueue& mqueue = queuedMessages[other];
            ASSERT(!mqueue.empty());
            ASSERT(mqueue.front().messageType == SimulatorMessage::READ_ERROR);
            // Remove the other pending error message from the queue
            removeEventFromQueue(mqueue.front());
            
            mqueue.pop_front();
            localErrors[other] = false;
            enqueueEvent(other); // Update flags, events, etc.
          }
          else {
            ASSERT(openSockets[other] && !remoteErrors[other]);
            ASSERT(queuedMessages[other].empty()
                   || queuedMessages[other].front().messageType == SimulatorMessage::MESSAGE);
            removeEvents(other);
            remoteErrors[other] = true;
          }
          break;
        }
      case SimulatorTransport::BIDI_READ_ERROR: // BI-DIrectional
        {
          uint32_t msgId = getNetMessageId();
          maceout << "Simulator pushing back message " << msgId << " as READ_ERROR from " << localNode << " to " << other << Log::endl;

          removeEvents(other);
          enqueueEvent(other);
          
          // Create also queues the error message event
          queuedMessages[other].push_front(createMessageBidiReadError(other, msgId));
          remoteErrors[other] = true;
          localErrors[other] = true;
          ASSERT(openSockets[other]);
          break;
        }
      case SimulatorTransport::DEST_READY:
        {
          enqueueEvent(other);
          break;
        }
    }
  }

  void SimulatorTCPCommonService::clearQueue(int destNode, int transportError,
                                             bool upcallError, bool removeEv) {
    closeSocket(destNode);
    MessageQueue& mqm = queuedMessages[destNode];
    if (mqm.empty()) { return; }
    // Remove queued events in the global event list
    if (removeEv) {
      removeEvents(destNode);
    }
    
    MessageQueue::iterator it = mqm.begin(); 
    if ((*it).messageType != SimulatorMessage::MESSAGE) {
      it++;
    } 

    // Handling per-message messageErrors
    if (upcallMessageErrors && upcallError) {
      for (MessageQueue::iterator it2 = it; it2 != mqm.end(); ++it2) {
        SimulatorMessage& m = *it2;
        ASSERT(m.destination == destNode || m.nextHop == destNode);
        ASSERT(m.messageType == SimulatorMessage::MESSAGE);
        NetworkErrorHandler* handler = network_handlers[m.regId];
        handler->messageError(SimCommon::getMaceKey(destNode), transportError,
                              m.msg, m.regId);
      }
    }
    mqm.erase(it, mqm.end());

    if (mqm.empty()) { // just to set the damn flag in modelchecker. Possible workaround?
      enqueueEvent(destNode);
    }
    addFlushedEvent();
    addCTSEvent(destNode);
  }

  void SimulatorTCPCommonService::openSocket(int node) {
    if (!openSockets[node]) {
      openSockets[node] = true;
      //     stats[node] = BufferStatisticsPtr(new BufferStatistics(BaseTransport::DEFAULT_WINDOW_SIZE));
    }
  }

  void SimulatorTCPCommonService::closeSocket(int node) {
    if (openSockets[node]) {
      openSockets[node] = false;
      stats.erase(node);
    }
  }

  void SimulatorTCPCommonService::transport_error(const int& destNode, int transportError) {
    //XXX: Not capturing the possibility of interleavings!
    //   clearQueue(destNode, transportError != TransportError::CONNECT_ERROR);
    clearQueue(destNode, transportError, true, false);

    NetworkHandlerMap::iterator i;
    for (i = network_handlers.begin(); i != network_handlers.end(); i++) {
      NetworkErrorHandler* handler = i->second;
      handler->error(SimCommon::getMaceKey(destNode), transportError, "error", i->first);
    }
  }

  uint32_t SimulatorTCPCommonService::hashState() const {
    static uint32_t hashAnd = (queueSize + localPort) * localNode;
    static hash_string strHasher;
    uint32_t hashWork = hashAnd;
    for (ReceiveHandlerMap::const_iterator i = handlers.begin(); i != handlers.end(); i++) {
      hashWork = (hashWork << 1) ^ i->first;
    }
    for (NetworkHandlerMap::const_iterator i = network_handlers.begin(); i != network_handlers.end(); i++) {
      hashWork = (hashWork << 1) ^ i->first;
    }
    int node;
    MessageQueueMap::const_iterator i;
    for (i = queuedMessages.begin(), node = 0; i != queuedMessages.end(); i++, node++) {
      hashWork += node;
      for (MessageQueue::const_iterator j = i->begin(); j != i->end(); j++) {
        hashWork ^= strHasher((*j).msg);
      }
    }
    return hashWork;
  }

  void SimulatorTCPCommonService::print(std::ostream& out) const {
    //This stuff is staticish -- can be printed with node
    out << "[SimulatorTCPCommon]" << std::endl;
    out << "[queueSize = " << queueSize << "][localPort = " << localPort << "][localKey = " << SimCommon::getMaceKey(localNode) << "]" << std::endl;
    out << "[RDH] ";
    for (ReceiveHandlerMap::const_iterator i = handlers.begin(); i != handlers.end(); i++) {
      out << i->first << " ";
    }
    out << "[/RDH]" << std::endl;
    out << "[NEH] ";
    for (NetworkHandlerMap::const_iterator i = network_handlers.begin(); i != network_handlers.end(); i++) {
      out << i->first << " ";
    }
    out << "[/NEH]" << std::endl;
    out << "[socket state]" ;
    for (int i = 0; i < SimCommon::getNumNodes(); i++) {
      out << "[node " << i << "] " << localErrors[i] << "," << remoteErrors[i] << "," << openSockets[i] << "," << pendingDestNotReady[i] << " [/node " << i << "]";
    }
    out << "[/socket state]" << std::endl;
    out << "[/SimulatorTCPCommon]" << std::endl;
  }

  void SimulatorTCPCommonService::printState(std::ostream& out) const {
    //This stuff is staticish -- can be printed with node
    out << "[queueSize = " << queueSize << "][localPort = " << localPort << "][localKey = " << SimCommon::getMaceKey(localNode) << "]";
    out << "[RDH] ";
    for (ReceiveHandlerMap::const_iterator i = handlers.begin(); i != handlers.end(); i++) {
      out << i->first << " ";
    }
    out << "[/RDH]";
    out << "[NEH] ";
    for (NetworkHandlerMap::const_iterator i = network_handlers.begin(); i != network_handlers.end(); i++) {
      out << i->first << " ";
    }
    out << "[/NEH]";
    //Do not print messages during state print -- they are printed under printNetworkState
  }

  void SimulatorTCPCommonService::printNetwork(std::ostream& out, bool printState) const {
    //This stuff is the network state, not node state (can be changed by other nodes in the network
    out << "[messages]";
    int node;
    MessageQueueMap::const_iterator i;
    for (i = queuedMessages.begin(), node = 0; i != queuedMessages.end(); i++, node++) {
      if (i->size()) {
        out << "[toDest(" << node << ")] ";
        for (MessageQueue::const_iterator j = i->begin(); j != i->end(); j++) {
          if (printState)
            out << " " << (*j).regId << " " << (*j).messageType << " " << Base64::encode((*j).msg) << " ";
          else
            out << (*j).messageId << "::" << (*j).messageType << " ";
        }
        out << "[/toDest(" << node << ")]";
      }
    }
    out << "[/messages]";
  }

  bool SimulatorTCPCommonService::checkSafetyProperties(std::string& description, const _NodeMap_& nodes, const _KeyMap_& keymap) {
    ADD_SELECTORS("SimulatorTCPCommonService::testSafetyProperties");
    bool invalid = false;
    int i;
    int j;
    for (i = 0; !invalid && i < SimCommon::getNumNodes(); i++) {
      for (j = i; !invalid && j < SimCommon::getNumNodes(); j++) {
        //         SimulatorTCPCommonService* nodes_i = reinterpret_cast<SimulatorTCPCommonService*>(nodes.get(i));
        //         SimulatorTCPCommonService* nodes_j = reinterpret_cast<SimulatorTCPCommonService*>(nodes.get(j));
        SimulatorTCPCommonService const * nodes_i = nodes.get(i);
        SimulatorTCPCommonService const * nodes_j = nodes.get(j);
        if (nodes_i->localErrors[j]) {
          if (nodes_i->pendingDestNotReady[j]) {
            invalid = true;
          }
          if (nodes_j->pendingDestNotReady[i]) {
            invalid = true;
          }
          if (nodes_i->remoteErrors[j]) {
            if (nodes_i->openSockets[j]) {
              // 1,1,1 - OK.  Need 1,1,1 from j.
              if (!(nodes_j->localErrors[i] && nodes_j->remoteErrors[i] && nodes_j->openSockets[i])) {
                invalid = true;
              }
            } else {
              // 1,1,0 - INVALID!
              invalid = true;
            }
          } else {
            if (nodes_i->openSockets[j]) {
              // 1,0,1 - INVALID!
              invalid = true;
            } else {
              // 1,0,0 - Ok.  Need 0,1,1 from j
              if (!(!nodes_j->localErrors[i] && nodes_j->remoteErrors[i] && nodes_j->openSockets[i])) {
                invalid = true;
              }
            }
          }
        } else {
          if (nodes_i->remoteErrors[j]) {
            if (nodes_i->pendingDestNotReady[j]) {
              invalid = true;
            }
            if (nodes_j->pendingDestNotReady[i]) {
              invalid = true;
            }
            if (nodes_i->openSockets[j]) {
              // 0,1,1 - Ok.  Need 1,0,0 from j
              if (!(nodes_j->localErrors[i] && !nodes_j->remoteErrors[i] && !nodes_j->openSockets[i])) {
                invalid = true;
              }
            } else {
              // 0,1,0 - INVALID!
              invalid = true;
            }
          } else {
            if (nodes_i->pendingDestNotReady[j] && nodes_j->pendingDestNotReady[i]) {
              invalid = true;
            }
            if (nodes_i->openSockets[j]) {
              // 0,0,1 - Ok.  Need 0,0,1 from j
              if (nodes_i->pendingDestNotReady[j]) {
                invalid = true;
              }
              if (nodes_j->pendingDestNotReady[i]) {
                invalid = true;
              }
              if (!(!nodes_j->localErrors[i] && !nodes_j->remoteErrors[i] && nodes_j->openSockets[i])) {
                invalid = true;
              }
            } else {
              // 0,0,0 - Ok.  Need 0,0,0 from j
              if (!(!nodes_j->localErrors[i] && !nodes_j->remoteErrors[i] && !nodes_j->openSockets[i])) {
                invalid = true;
              }
            }
          }
        }
      }
    }

    if (invalid) {
      i--;
      j--;
      //       SimulatorTCPCommonService* nodes_i = reinterpret_cast<SimulatorTCPCommonService*>(nodes.get(i));
      //       SimulatorTCPCommonService* nodes_j = reinterpret_cast<SimulatorTCPCommonService*>(nodes.get(j));
      SimulatorTCPCommonService const * nodes_i = nodes.get(i);
      SimulatorTCPCommonService const * nodes_j = nodes.get(j);
      description = "SimulatorTCPCommonService";
      maceerr << "Invalid socket state! nodes("<< (i) << "," << (j) << "): ";
      maceerr << "(l,r,o,p),(l,r,o,p): ";
      maceerr << "(" << nodes_i->localErrors[j] << "," << nodes_i->remoteErrors[j] << "," << nodes_i->openSockets[j] << "," << nodes_i->pendingDestNotReady[j] << "),";
      maceerr << "(" << nodes_j->localErrors[i] << "," << nodes_j->remoteErrors[i] << "," << nodes_j->openSockets[i] << "," << nodes_j->pendingDestNotReady[i] << ")" << Log::endl;
      return false;
    }
    
    return true;
  }

}
