/* 
 * SimNetwork.h : part of the Mace toolkit for building distributed systems
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
#ifndef SIM_NET_H
#define SIM_NET_H

#include "RandomVariable.h"
#include "SimNetworkCommon.h"

class SimNetwork : public SimNetworkCommon {
  protected:
    static SimNetwork* _sim_inst;

    bool has_network_parameters;  // Using Special network parameters?
    mace::vector<mace::vector<uint32_t> > net_bw;  // matrix of bandwidth of links (in kbps)
    mace::vector<mace::vector<uint32_t> > net_latency;  // matrix of latencies of links (in ms)
    mace::map<uint32_t, uint32_t> net_groups;  // maps node to groupID, for network delay purposes
    
    SimNetwork() : SimNetworkCommon() {
      readNetworkParameters();
    }

  private:
    void readNetworkParameters();

  public:
    uint64_t getLatencyImpl(int src, int dest, size_t size) {
      // Computes the Transmission delay
      // 8000 Kbps
      // 1 byte = 8 bits
      int numOutgoingFlows = 0;
      
      for (PortNodeRecipientMap::iterator i = simulatorTransportMap.begin();
	   i != simulatorTransportMap.end(); i++) {
	numOutgoingFlows += (*i)[src]->getNumFlows();
      }
      numOutgoingFlows = std::max(numOutgoingFlows, 1);
      
      uint64_t linkLatency = getFixedLatencyImpl(src, dest);
      uint64_t bitrate;
      if (!has_network_parameters) {
        bitrate = 8000000; //8000 kbps
      } else {
        ASSERT ((uint32_t)src < net_groups.size() && (uint32_t)dest < net_groups.size());
        bitrate = (uint64_t)net_bw[net_groups[src]][net_groups[dest]] * 1000;
      }
      static const uint64_t usps = 1000000;
      
      uint64_t baseLatency = numOutgoingFlows * size * 8 * usps / bitrate;
      uint64_t randomLatency = (uint64_t)RandomVariable::pareto(linkLatency, 3);
      if (randomLatency > (uint64_t)60 * 1000 * 1000 * (size / 1500 + 1)) {
        randomLatency = (uint64_t)60 * 1000 * 1000 * (size / 1500 + 1);
      }
      
      return baseLatency + randomLatency;
    }

    static uint64_t getLatency(int src, int dest, size_t size) {
      return Instance().getLatencyImpl(src, dest, size);
    }

    uint64_t getFixedLatencyImpl(int src, int dest) {
      // Computes the propagation delay
      uint64_t linkLatency;
      if (!has_network_parameters) {
        linkLatency = 1000; //1000 microsec = 1ms
      } else {
        ASSERT ((uint32_t)src < net_groups.size() && (uint32_t)dest < net_groups.size());
        linkLatency = (uint64_t)net_latency[net_groups[src]][net_groups[dest]] * 1000;
      }
      return linkLatency;
    }

    static uint64_t getFixedLatency(int src, int dest) {
      return Instance().getFixedLatencyImpl(src, dest);
    }

    void queueDestNotReadyEvent(int destNode, int srcNode, int srcPort);

    static SimNetwork& SetInstance() { 
      ASSERT(_sim_inst == NULL);
      _sim_inst = new SimNetwork();
      return *_sim_inst; 
    } 

    static SimNetwork& Instance() { 
      ASSERT(_sim_inst != NULL);
      return *_sim_inst; 
    } 

    virtual ~SimNetwork() { }
};

#endif // SIM_NET_H
