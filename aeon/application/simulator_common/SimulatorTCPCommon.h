/* 
 * SimulatorTCPCommon.h : part of the Mace toolkit for building distributed systems
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
#ifndef SIMULATOR_TCP_COMMON_SERVICE_H
#define SIMULATOR_TCP_COMMON_SERVICE_H

#include <deque>

#include "BandwidthTransportServiceClass.h"
#include "BufferedTransportServiceClass.h"
#include "RouteServiceClass.h"
#include "Serializable.h"
#include "SimNetworkCommon.h"
#include "m_map.h"
#include "mace.h"

namespace SimulatorTCP_namespace {

class SimulatorTCPService;
class SimulatorTCPCommonService;
typedef mace::map<int, SimulatorTCPCommonService const *, mace::SoftState> _NodeMap_;
typedef mace::map<MaceKey, int, mace::SoftState> _KeyMap_;

/**
 * Simulated Transport for modelchecking. Simulates a reliable transport which
 * also provides queueing. Simulates socket errors, and manages error handling
 */
class SimulatorTCPCommonService : public virtual BandwidthTransportServiceClass, public virtual BufferedTransportServiceClass, public virtual SimulatorTransport {
public:
  typedef std::deque<SimulatorMessage> MessageQueue;
  typedef mace::vector<MessageQueue, mace::SoftState> MessageQueueMap;
  typedef mace::map<int, BufferStatisticsPtr, mace::SoftState> StatsMap;
  typedef mace::vector<int> SocketState;
  typedef mace::vector<uint64_t> SocketTimes;
  static const int MAX_QUEUE_SIZE = 100000;
  //   static int PT_tcp;

public:
  //   SimulatorTCPCommonService(uint32_t queue_size, uint16_t port, int node,
  //                             bool messageErrors, int forwarder);
  SimulatorTCPCommonService(uint16_t port, int forwarder, bool shared);
  virtual ~SimulatorTCPCommonService();

  void maceInit();
  void maceExit() {}
  void maceReset();
  uint32_t hashState() const;
  void print(std::ostream& out) const;
  void printState(std::ostream& out) const;
  void printNetwork(std::ostream& out, bool printState = false) const;
  registration_uid_t registerHandler(ReceiveDataHandler& handler, registration_uid_t registrationUid = -1);
  registration_uid_t registerHandler(NetworkErrorHandler& handler, registration_uid_t registrationUid = -1);
  registration_uid_t registerHandler(ConnectionStatusHandler& h, registration_uid_t rid = -1);
  void unregisterHandler(ReceiveDataHandler& handler, registration_uid_t rid = -1) { }
  void unregisterUniqueHandler(ReceiveDataHandler& handler) { unregisterHandler(handler); }
  void registerUniqueHandler(ReceiveDataHandler& handler) { }
  void unregisterHandler(NetworkErrorHandler& handler, registration_uid_t rid = -1) { }
  void unregisterUniqueHandler(NetworkErrorHandler& handler) { unregisterHandler(handler); }
  void registerUniqueHandler(NetworkErrorHandler& handler) { }  

  /// Intercepted by the simulated transport to queue message for delivery at remote node
  virtual bool route(const MaceKey& dest, const std::string& s, registration_uid_t registrationUid) = 0;

  uint32_t outgoingBufferedDataSize(const MaceKey& peer, registration_uid_t registrationUid = -1) const;
  virtual const BufferStatisticsPtr getStatistics(const MaceKey& peer, Connection::type d, registration_uid_t registrationUid = -1) = 0;
  
  bool canSend(const MaceKey& peer, registration_uid_t registrationUid = -1) const;
  void requestToSend(const MaceKey& peer, registration_uid_t rid);
  bool routeRTS(const MaceKey& dest, const std::string& s, registration_uid_t rid);
  void requestFlushedNotification(registration_uid_t rid);

  void signal(int source, TransportSignal sig);

  /// Simulate the message at this node. Packet, Dest not ready, socket error, etc.
  std::string simulateEvent(const SimulatorMessage& msg);
  /// Perform the delivery of an actual packet
  void recv(const SimulatorMessage& m);
  void transport_error(const int& destAddr, int transportError);

  std::string simulateFlushed();
  std::string simulateCTS(int node);
  virtual void addCTSEvent(int dest) = 0;
  virtual void addFlushedEvent() = 0;
  virtual bool removeFlushedEvent() = 0;
  bool hasFlushed() const;
  bool hasCTS(int node) const;
  virtual void prepareDestNotReady(int node, uint64_t t) = 0;

  bool isAvailableMessage(int dest) const;
  SimulatorMessage getMessage(int dest);
  int getPort() const;
  const MaceKey& localAddress() const;

  bool isListening() const;
  size_t queuedDataSize() const;

  // BandwidthTransportServiceClass members
  void startSegment(const MaceKey& peer, BandwidthDirection bd, registration_uid_t rid);
  void endSegment(const MaceKey& peer, BandwidthDirection bd, registration_uid_t rid);
  double getBandwidth(const MaceKey& peer, BandwidthDirection bd, registration_uid_t rid);
  // not used in BP
//   bool hasRoom(const MaceKey& peer);
  int queued(const MaceKey& peer, registration_uid_t rid);
  // not used in BP
//   void setWindowSize(const MaceKey& peer);

  static bool checkSafetyProperties(mace::string& description, const _NodeMap_& nodes, const _KeyMap_& keymap);
  static bool checkLivenessProperties(mace::string& description, const _NodeMap_& nodes, const _KeyMap_& keymap) {
    return true; //could do other perhaps, but not so general.
  }
  
protected:
  void clearQueue(int destNode, int transportError,
                  bool upcallError, bool removeEv);
  void openSocket(int node);
  void closeSocket(int node);
  virtual void removeEvents(int other) const = 0;
  
protected:
  const uint32_t queueSize;
  const int localPort;
  const uint16_t localNode;
  const bool upcallMessageErrors; /// deliver per-message errors?
  const int forwarder; /// Forwarder node. =localNode when no forwarder
  bool listening; /// true after maceInit
  
  typedef mace::deque<registration_uid_t> RegList;
  typedef mace::vector<RegList> RtsList;
  typedef mace::map<int, uint64_t> RtsEventsMap;
  RtsList rtsRequests;
  RtsEventsMap rtsEventsQueued;
  
  typedef mace::set<registration_uid_t> RegSet;
  RegSet flushRequests;
  uint64_t flushEventQueued; // a FLUSH queued in event list
  
  typedef mace::map < registration_uid_t, ConnectionStatusHandler*, mace::SoftState> ConnectionHandlerMap;
  ConnectionHandlerMap connectionHandlers;
  typedef mace::map < registration_uid_t, ReceiveDataHandler*, mace::SoftState> ReceiveHandlerMap;
  ReceiveHandlerMap handlers;
  typedef mace::map < registration_uid_t, NetworkErrorHandler*, mace::SoftState> NetworkHandlerMap;
  NetworkHandlerMap network_handlers;

  MessageQueueMap queuedMessages;
  SocketState openSockets; // true if we have delivered a message to the dest, or if we have received a message
  SocketState remoteErrors; // true if there is an error message coming locally (controlled by signals)
  SocketState localErrors; // true if we have a queued error message
  SocketTimes pendingDestNotReady; // true if we have a queued error message
  StatsMap stats; // XXX does the modelchecker need this?

protected:

  virtual uint32_t getNetMessageId() = 0;
  /// Fetch the transport for the given node (usually the remote one)
  virtual SimulatorTransport* getNetTransport(int node, int port) = 0;
  /// Simply calls net.signal on the actual network instance
  virtual void netSignal(int source, int dest, int port, SimulatorTransport::TransportSignal sig) = 0;
    
  /// Primarily used by the modelchecker to maintain a single net-event per node in the pending list
  virtual void enqueueEvent(int destNode) = 0;
  /// Remove the DEST_NOT_READY event from the queue of the remote node
  virtual bool removeDestNotReadyEvent(int other) = 0;
  /// Used to clear messages from this node-specific queue
  virtual bool removeEventFromQueue(const SimulatorMessage& msg) const = 0;
  /// Enqueue a source reset message for this node. Enqueue an event if necessary
  virtual SimulatorMessage createMessageSourceReset(int other, uint32_t msgId) = 0;
  /// Enqueue a bi-directional socket failure message for this node. Enqueue an event if necessary
  virtual SimulatorMessage createMessageBidiReadError(int other, uint32_t msgId) = 0;
};

}

int SimulatorTCPCommon_load_protocol();
SimulatorTCP_namespace::SimulatorTCPCommonService *SimulatorTCPCommonService_init_function();

#endif // SIMULATOR_TCP_COMMON_SERVICE_H
