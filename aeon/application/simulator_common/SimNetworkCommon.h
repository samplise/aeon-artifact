/* 
 * SimNetworkCommon.h : part of the Mace toolkit for building distributed systems
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
#ifndef SIM_NET_COMMON_H
#define SIM_NET_COMMON_H

#include "Iterator.h"
#include "MaceTypes.h"
#include "ReceiveDataHandler.h"
#include "SimCommon.h"
#include "SimulatorTransport.h"
#include "Util.h"
#include "m_map.h"
#include "mace-macros.h"
#include "mset.h"
#include "mvector.h"

typedef mace::vector<SimulatorTransport*, mace::SoftState> NodeRecipientMap;
typedef mace::vector<NodeRecipientMap, mace::SoftState> PortNodeRecipientMap;
typedef mace::set<int> ErrorSet;
typedef mace::map<int, ErrorSet> ErrorMap;

class SimNetworkCommon : public SimCommon {
  protected:
    PortNodeRecipientMap simulatorTransportMap;
    uint32_t currentMessageId;
    int base_port;
    static int failureCount;

    const int MESSAGE_WEIGHT;
    SimNetworkCommon() 
      : SimCommon(), 
        simulatorTransportMap(1, NodeRecipientMap(SimCommon::getNumNodes())), 
        currentMessageId(0), // XXX 1 in MC, 0 in PC
        base_port(params::get("MACE_PORT", 5377)),
	MESSAGE_WEIGHT(params::get("MESSAGE_WEIGHT", 8))
    { }
  
  public:
    uint32_t getMessageId() {
      return currentMessageId++;
    }

    uint32_t hashState() const {
      uint32_t hash = 0;
      int port;
      int node;
      PortNodeRecipientMap::const_iterator i;
      NodeRecipientMap::const_iterator j;
      for(i = simulatorTransportMap.begin(), port = base_port; i != simulatorTransportMap.end(); i++, port++) {
        for(j = i->begin(), node = 0; j != i->end(); j++, node++) {
          hash += port ^ node ^  (*j)->hashState();
        }
      }
      return hash;
    }

    void printInternal(std::ostream& out, bool printState = false) const {
      int port;
      int node;
      PortNodeRecipientMap::const_iterator i;
      NodeRecipientMap::const_iterator j;
      for(i = simulatorTransportMap.begin(), port = base_port; i != simulatorTransportMap.end(); i++, port++) {
        out << "[Port(" << port << ")]";
        for(j = i->begin(), node = 0; j != i->end(); j++, node++) {
          out << "[Node(" << node << ")]";
          
          (*j)->printNetwork(out, printState);
          
          out << "[/Node(" << node << ")]";
        }
        out << "[/Port(" << port << ")]";
      }
    }

    void print(std::ostream& out) const {
      printInternal(out);
    }

    void printState(std::ostream& out) const {
      printInternal(out, true);
    }

    void registerHandler(int port, SimulatorTransport& trans) {
      ADD_FUNC_SELECTORS;
      maceout << "registering handler for node " << SimCommon::getCurrentMaceKey()
              << " on port " << port << " for node " << getCurrentNode()
              << " with transport " << &trans << Log::endl;
      unsigned p2 = port;
      p2 -= (base_port-1);
      while(p2 > simulatorTransportMap.size()) { simulatorTransportMap.push_back(NodeRecipientMap(SimCommon::getNumNodes())); }
      SimulatorTransport*& t = simulatorTransportMap[port-base_port][SimCommon::getCurrentNode()];
      ASSERTMSG(t == NULL, "Two transports for a single node must use a different port in the model checker.");
      t = &trans;
      //       simulatorTransportMap[port-base_port][SimCommon::getCurrentNode()] = &trans;
    }
    SimulatorTransport* getTransport(int node, int port) {
      return simulatorTransportMap[port-base_port][node];
    }
    bool isListening(int node, int port) const {
      return simulatorTransportMap[port-base_port][node] != NULL && simulatorTransportMap[port-base_port][node]->isListening();
    }
    void reset() {
      for(PortNodeRecipientMap::iterator i = simulatorTransportMap.begin(); i != simulatorTransportMap.end(); i++) {
        for(NodeRecipientMap::iterator j = i->begin(); j != i->end(); j++) {
            //           delete *j;
          *j = NULL;
        }
      }
      currentMessageId = 0;
      failureCount = 0;
    }
    enum NetEventType { MESSAGE, DEST_NOT_READY, CTS, FLUSHED };
    void signal(int source, int dest, int port, SimulatorTransport::TransportSignal sig) {
      simulatorTransportMap[port-base_port][dest]->signal(source, sig);
    }

    /// Queue a DEST_NOT_READY when the receiver is not yet listening
    virtual void queueDestNotReadyEvent(int destNode, int srcNode, int srcPort) = 0;

    bool hasEventsWaiting() const {
      // Check the event queue for any NETWORK event
      const EventList& eventList = SimEvent::getEvents();
      for (EventList::const_iterator ev = eventList.begin();
           ev != eventList.end(); ++ev) {
        if (ev->second.type == Event::NETWORK) {
          return true;
        }
      }
      return false;
    }

    std::string simulateEvent(const Event& e) {
      ADD_SELECTORS("SimNetwork::simulateEvent");

      const int net_event = e.simulatorVector[0];
      const int src_node = e.simulatorVector[1];
      const int src_port = e.simulatorVector[2];

      maceout << "Simulating network event for " << e.node << " from " << src_node << " type " << net_event << " on port " << src_port << Log::endl;

      SimulatorTransport* dest = getTransport(e.node, src_port);
      std::ostringstream r;

      switch(net_event) {
        case SimNetworkCommon::MESSAGE: 
          {
            if (!dest->isListening()) {
              queueDestNotReadyEvent(e.node, src_node, src_port);
              r << "Connection refused from " << src_node;
            }
            else {
              SimulatorMessage msg = getTransport(src_node, src_port)->getMessage(e.node);
              if(msg.messageType == SimulatorMessage::READ_ERROR) {
                maceout << "Delivering socket error from " << msg.source << " to " << msg.destination
                  << " messageId " << msg.messageId << Log::endl;
                r << "error from " << msg.source << " on " << msg.destination << " id " << msg.messageId << " size " << msg.msg.size();
              } else {
                maceout << "Delivering messageId " << msg.messageId << " from "
                  << msg.source << " to " << msg.destination << Log::endl;
                r << "id " << msg.messageId << " from " << msg.source << " to " << msg.destination << " size " << msg.msg.size();
              }
              r << " " << dest->simulateEvent(msg);
            }
            break;
          }

        case SimNetworkCommon::DEST_NOT_READY:  //XXX queue DEST_NOT_READY messages!!!
          {
            SimulatorMessage msg(src_node, e.node, getMessageId(), -1, -1, 0, 0, 
                                 SimulatorMessage::DEST_NOT_READY, -1, "");

            maceout << "Event of " << msg.destination << " is a dest_not_ready error from node "
                << msg.source << Log::endl;

            std::string s = dest->simulateEvent(msg);

            r << s << " dest_not_ready error from " << msg.source << " on " << msg.destination;
            break;
          }
        
        case SimNetworkCommon::CTS: {
          maceout << "simulating CTS on " << e.node << " for " << src_node << Log::endl;
          r << "CTS for " << src_node << " on " << e.node << " ";
          r << dest->simulateCTS(src_node);
          break;
        }
        
        case SimNetworkCommon::FLUSHED: {
          maceout << "simulating flushed on " << src_node << Log::endl;
          r << "notifying flushed on " << e.node << " ";
          r << dest->simulateFlushed();
          break;
        }
        
        default: ASSERT(net_event == SimNetworkCommon::MESSAGE || net_event == SimNetworkCommon::DEST_NOT_READY);
      }
      
      return r.str();
    }
    
    static int getFailureCount() {
      return failureCount;
    }
    static void incrementFailureCount() {
      failureCount++;
    }

    virtual ~SimNetworkCommon() { }
};

#endif // SIM_NET_COMMON_H
