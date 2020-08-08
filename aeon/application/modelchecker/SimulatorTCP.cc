/* 
 * SimulatorTCP.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, James W. Anderson, Karthik Nagaraj
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
#include "BaseTransport.h"
#include "MaceTime.h"
#include "Sim.h"
#include "SimEventWeighted.h"
#include "SimulatorTCP.h"
#include "ServiceConfig.h"

namespace SimulatorTCP_namespace {

void SimulatorTCPService::addNetEvent(int destNode, SimNetworkCommon::NetEventType mType,
                                      const std::string& desc, int weight) const {
  Event ev;
  ev.node = destNode;
  ev.type = Event::NETWORK;
  ev.simulatorVector.push_back(mType);
  ev.simulatorVector.push_back(localNode);
  ev.simulatorVector.push_back(localPort);
  ev.desc = desc;

  SimEventWeighted::addEvent(weight, ev);
}

void SimulatorTCPService::enqueueEvent(int destNode) {
  ADD_SELECTORS("SimulatorTCPService::enqueuEvent");
  eventQueued[destNode] = isAvailableMessage(destNode);
  maceout << "me: " << localNode << " destNode: " << destNode << " eventQueued: " << eventQueued[destNode] << Log::endl;
  if (isAvailableMessage(destNode)) {
    addNetEvent(destNode, SimNetworkCommon::MESSAGE, "(MESSAGE EVENT,src,port)", SimNetwork::MESSAGE_WEIGHT);
  } 
}

bool SimulatorTCPService::route(const MaceKey& dest, const std::string& s, registration_uid_t handlerUid) {
  ADD_SELECTORS("SIMULATOR::TCP::route::pkt::(downcall)");
  maceout << " dest=" << dest.toString() << " size=" << s.size() << " priority=" << 0 << " handlerUid=" << handlerUid << " port=" << localPort << Log::endl;
  uint32_t msgId = net.getMessageId();
  int destNode = Sim::getNode(dest);
  int destQueue = forwarder;
  MaceKey nextHop = dest;
  if (destQueue == localNode) {
    // no forwarder defined
    destQueue = destNode;
  }
  else {
    // send to forwarder
    nextHop = Sim::getMaceKey(destQueue);
  }

  MessageQueue& mqueue = queuedMessages[destQueue];
  if (mqueue.size() < queueSize) {
    maceout << "route([dest=" << nextHop << "][s_deserialized=pkt(id=" << msgId << ")])" << Log::endl;
    if (!eventQueued[destNode] && !remoteErrors[destNode] && !pendingDestNotReady[destNode]) {
      if (!isDestNotReadyPending(destNode)) {
        // Add event only when queue is empty => One event per queue
        // Queue size > 0 => eventQueued
        ASSERT(mqueue.size() == 0 || eventQueued[destNode]);
        // XXX: Should event be queued for destQueue instead of destNode?
        addNetEvent(destNode, SimNetworkCommon::MESSAGE, "(MESSAGE EVENT,src,port)", SimNetwork::MESSAGE_WEIGHT);
        eventQueued[destNode] = true;
      }
    }

    Log::binaryLog(selectorId->log,
        LogicalClockPipeline::MessageId(msgId, 0, 0), 0);

    mqueue.push_back(SimulatorMessage(localNode, destNode, msgId, forwarder, destQueue, 0, 0, SimulatorMessage::MESSAGE, handlerUid, s));

    removeFlushedEvent();

    return true;
  }
  maceout << "route([dest=" << nextHop << "][s_deserialized=pkt(id=" << msgId << ", SIM_DROP=1)])" << Log::endl;
  return false;
}

bool SimulatorTCPService::isDestNotReadyPending(int destNode) const {
  static const int DEST_NOT_READY_WEIGHT = 2; // optimization for reducing walk
  
  const EventList& eventList = SimEvent::getEvents();
  if (eventList.find(DEST_NOT_READY_WEIGHT) == eventList.end()) { return false; }
  for (EventList::const_iterator ev = eventList.find(DEST_NOT_READY_WEIGHT);
       ev != eventList.upper_bound(DEST_NOT_READY_WEIGHT); ++ev) {
    if (ev->second.equals(destNode, Event::NETWORK,
                          SimNetworkCommon::DEST_NOT_READY,
                          localNode, localPort)) {
      return true;
    }
  }
  return false;
}

void SimulatorTCPService::addFlushedEvent() {
  static const int FLUSH_WEIGHT = 40;
  if (hasFlushed() && flushEventQueued == 0) {
    addNetEvent(localNode, SimNetworkCommon::FLUSHED, "Flushed", FLUSH_WEIGHT);

    flushEventQueued = 1; // simply non-zero
  }
}

bool SimulatorTCPService::removeEvent(int destNode, int local,
                                      SimNetworkCommon::NetEventType type,
                                      int weight) const {
  EventList& eventList = SimEvent::getEvents();
  if (eventList.find(weight) == eventList.end()) {
    return false;
  }

  // Use known weight to reduce search space
  for (EventList::iterator ev = eventList.find(weight);
       ev != eventList.upper_bound(weight); ++ev) {
    if (ev->second.equals(destNode, Event::NETWORK,
                          type, local, localPort)) {
      SimEventWeighted::removeEvent(ev);
      return true;
    }
  }
  return false;

}

bool SimulatorTCPService::removeFlushedEvent() {
  if (flushEventQueued == 0)
    return false;
  flushEventQueued = 0;
  static const int FLUSH_WEIGHT = 40;
  ASSERT(removeEvent(localNode, localNode,
                     SimNetworkCommon::FLUSHED, FLUSH_WEIGHT));
  return true;
}

bool SimulatorTCPService::removeDestNotReadyEvent(int other) {
  pendingDestNotReady[other] = 0;
  static const int DEST_NOT_READY_WEIGHT = 2;
  return removeEvent(localNode, other,
                     SimNetworkCommon::DEST_NOT_READY, DEST_NOT_READY_WEIGHT);
}

void SimulatorTCPService::addCTSEvent(int dest) {
  if (hasCTS(dest) && !rtsEventsQueued.containsKey(dest)) {
    addNetEvent(localNode, SimNetworkCommon::FLUSHED, "(CTS,dest node,port)", SimNetwork::MESSAGE_WEIGHT);

    rtsEventsQueued[dest] = 1;
  }
}

bool SimulatorTCPService::removeEventFromQueue(const SimulatorMessage& msg) const {
  return removeEvent(msg.destination, localNode,
                     SimNetworkCommon::MESSAGE, SimNetwork::MESSAGE_WEIGHT);
}

void SimulatorTCPService::removeEvents(int other) const {
  ADD_FUNC_SELECTORS;
  const MessageQueue& mqueue = queuedMessages[other];
  if (mqueue.empty()) { return; }
  MessageQueue::const_iterator i = mqueue.begin();
  
  if (i->messageType == SimulatorMessage::MESSAGE) {
    ASSERT(removeEventFromQueue(*i));
  } // else leave error messages in the queue
}

SimulatorMessage SimulatorTCPService::createMessageSourceReset(int other, uint32_t msgId) {
  // Schedule an event if required
  if (!eventQueued[other]) {
    addNetEvent(other, SimNetworkCommon::MESSAGE,
                "(MESSAGE EVENT,src,port)", SimNetwork::MESSAGE_WEIGHT);
    eventQueued[other] = true;
  }
  return SimulatorMessage(localNode, other, msgId, forwarder, forwarder,
                          0, 0, SimulatorMessage::READ_ERROR, -1, "");
}

SimulatorMessage SimulatorTCPService::createMessageBidiReadError(int other, uint32_t msgId) {
  // Schedule an event if required
  if (!eventQueued[other]) {
    addNetEvent(other, SimNetworkCommon::MESSAGE,
                "(MESSAGE EVENT,src,port)", SimNetwork::MESSAGE_WEIGHT);
    eventQueued[other] = true;
  }
  return SimulatorMessage(localNode, other, msgId, forwarder, forwarder,
                          0, 0, SimulatorMessage::READ_ERROR, -1, "");
}

const BufferStatisticsPtr SimulatorTCPService::getStatistics(const MaceKey& peer, Connection::type d, registration_uid_t registrationUid) {
  BufferStatisticsPtr stats(new BufferStatistics(BaseTransport::DEFAULT_WINDOW_SIZE));
  return stats;
}

TransportServiceClass& configure_new_SimulatorTCP_Transport(bool shared) {
    return *(new SimulatorTCPService(std::numeric_limits<uint16_t>::max(), -1, shared));
}

BufferedTransportServiceClass& configure_new_SimulatorTCP_BufferedTransport(bool shared) {
    return *(new SimulatorTCPService(std::numeric_limits<uint16_t>::max(), -1, shared));
}

BandwidthTransportServiceClass& configure_new_SimulatorTCP_BandwidthTransport(bool shared) {
    return *(new SimulatorTCPService(std::numeric_limits<uint16_t>::max(), -1, shared));
}

void load_protocol() {
    mace::ServiceFactory<TransportServiceClass>::registerService(&configure_new_SimulatorTCP_Transport, "SimulatorTCP");
    mace::ServiceFactory<BandwidthTransportServiceClass>::registerService(&configure_new_SimulatorTCP_BandwidthTransport, "SimulatorTCP");
    mace::ServiceFactory<BufferedTransportServiceClass>::registerService(&configure_new_SimulatorTCP_BufferedTransport, "SimulatorTCP");

    mace::ServiceConfig<TransportServiceClass>::registerService("SimulatorTCP", mace::makeStringSet("reliable,inorder,ipv4,SimulatorTCP,TcpTransport",","));
    mace::ServiceConfig<BandwidthTransportServiceClass>::registerService("SimulatorTCP", mace::makeStringSet("reliable,inorder,ipv4,SimulatorTCP,TcpTransport",","));
    mace::ServiceConfig<BufferedTransportServiceClass>::registerService("SimulatorTCP", mace::makeStringSet("reliable,inorder,ipv4,SimulatorTCP,TcpTransport",","));
}

}
