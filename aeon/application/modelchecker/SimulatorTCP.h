/* 
 * SimulatorTCP.h : part of the Mace toolkit for building distributed systems
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
#ifndef SIMULATOR_TCP_SERVICE_H
#define SIMULATOR_TCP_SERVICE_H

#include "Sim.h"
#include "SimNetwork.h"
#include "SimulatorTCPCommon.h"

namespace SimulatorTCP_namespace {

/**
 * Modelchecker-specific transport. Differs from the common class in the way
 * message events are queued (in SimEventWeighted). At most one message event
 * per node is queued so that events do not happen out of order. Modelchecker
 * has absolutely no sense of time or how slow things happen. It only cares
 * about interleavings.
 */
class SimulatorTCPService : public SimulatorTCPCommonService {
  public:
      //     SimulatorTCPService(uint32_t queue_size, uint16_t port, int node, bool messageErrors = false, int forwarder = -1)
      //       : SimulatorTCPCommonService(queue_size, port, node, messageErrors, forwarder),
    SimulatorTCPService(uint16_t port = std::numeric_limits<uint16_t>::max(), int forwarder = -1)
      : SimulatorTCPCommonService(port, forwarder, true),
        net(SimNetwork::Instance()), eventQueued(Sim::getNumNodes()) {
      net.registerHandler(getPort(), *this);
    }

    SimulatorTCPService(uint16_t port /*= std::numeric_limits<uint16_t>::max()*/, int forwarder /* = -1*/, bool shared)
      : SimulatorTCPCommonService(port, forwarder, shared),
        net(SimNetwork::Instance()), eventQueued(Sim::getNumNodes()) {
      net.registerHandler(getPort(), *this);
    }

    ~SimulatorTCPService() { }

    bool route(const MaceKey& dest, const std::string& s, registration_uid_t registrationUid);

    /// Remove event queued by local for destNode
    bool removeEvent(int destNode, int local,
                     SimNetworkCommon::NetEventType type,
                     int weight) const;
  
    void addCTSEvent(int dest);
    /// Add and remove FLUSHED event from the queue
    void addFlushedEvent();
    bool removeFlushedEvent();
    virtual void prepareDestNotReady(int node, uint64_t t) {
      pendingDestNotReady[node] = 1;
    }
    
    inline uint32_t getNetMessageId() {
      return net.getMessageId();
    }

    inline SimulatorTransport* getNetTransport(int node, int port) {
      return net.getTransport(node, port);
    }

    void netSignal(int source, int dest, int port, SimulatorTransport::TransportSignal sig) {
      net.signal(source, dest, port, sig);
    }
 
    void addNetEvent(int destNode, SimNetworkCommon::NetEventType mType,
                     const std::string& desc, int weight) const;
    
    void enqueueEvent(int destNode);
    /// Checks to see if any DEST_NOT_READY that I sent is still pending
    bool isDestNotReadyPending(int destNode) const;
    /// Remove DEST_NOT_READY from event queue
    bool removeDestNotReadyEvent(int other);
    /// Remove message given SimulatorMessage equivalent
    bool removeEventFromQueue(const SimulatorMessage& msg) const;
    /// Remove any event that may be queued for the other node
    void removeEvents(int other) const;
    SimulatorMessage createMessageSourceReset(int other, uint32_t msgId);
    SimulatorMessage createMessageBidiReadError(int other, uint32_t msgId);
  
    const BufferStatisticsPtr getStatistics(const MaceKey& peer, Connection::type d, registration_uid_t registrationUid);


  private:
    SimNetwork& net;
    typedef std::vector<bool> EventQueuedList;
    EventQueuedList eventQueued; /// Is an event to a given destination queued?

};

}

#endif // SIMULATOR_TCP_SERVICE_H
