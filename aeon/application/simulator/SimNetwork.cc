/* 
 * SimNetwork.cc : part of the Mace toolkit for building distributed systems
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
#include "SimNetwork.h"
#include "SimTimeUtil.h"

SimNetwork* SimNetwork::_sim_inst = NULL;
int SimNetworkCommon::failureCount = 0;
    
void SimNetwork::readNetworkParameters() {
  // Sample file:
  // <num nodes> <num groups>
  // <num nodes in group> <node1> <node2> ...
  // ....
  // Square matrix of group_size x group_size. Each element = network link capacity/bitrate
  // Sq matrix of latencies
  // Look for a parameter file. Ignore special network parameters if absent

  std::string param_file = params::get<std::string>("SIM_NETWORK_PARAMS", "NetworkParams");
  std::ifstream network_params(param_file.c_str());
  has_network_parameters = network_params.good();
  if (!has_network_parameters)
    return;
  uint32_t num_nodes, num_groups;
  network_params >> num_nodes >> num_groups;
  ASSERTMSG(num_nodes > 0 && num_groups > 0, "No. of nodes or No. of groups invalid");

  // read group membership
  for (uint32_t i = 0; i < num_groups; ++i) {
    uint32_t num_members;
    network_params >> num_members;
    for (uint32_t j = 0; j < num_members; ++j) {
      uint32_t node;
      network_params >> node;
      ASSERT(node < num_nodes);
      // node belongs to group i
      net_groups[node] = i;
    }
  }

  for (uint32_t i = 0; i < num_nodes; i++)
    ASSERTMSG(net_groups.find(i) != net_groups.end(), "Incomplete group membership for nodes in Network params");

  // read (num_groups x num_groups) Bandwidth matrix
  net_bw.resize(num_groups);
  for (uint32_t i = 0; i < num_groups; ++i) {
    mace::vector<uint32_t>& bitrate_group = net_bw[i];
    for (uint32_t j = 0; j < num_groups; ++j) {
      uint32_t bitrate;
      network_params >> bitrate;
      bitrate_group.push_back(std::max(bitrate, (uint32_t)1));
    }
  }

  // read (num_groups x num_groups) Latency matrix
  net_latency.resize(num_groups);
  for (uint32_t i = 0; i < num_groups; ++i) {
    mace::vector<uint32_t>& latency_group = net_latency[i];
    for (uint32_t j = 0; j < num_groups; ++j) {
      uint32_t latency;
      network_params >> latency;
      latency_group.push_back(std::max(latency, (uint32_t)1));
    }
  }
}

void SimNetwork::queueDestNotReadyEvent(int destNode, int srcNode, int srcPort) {
  Event ev;
  ev.node = srcNode;
  ev.type = Event::NETWORK;
  ev.simulatorVector.push_back(SimNetworkCommon::DEST_NOT_READY);
  ev.simulatorVector.push_back(destNode);
  ev.simulatorVector.push_back(srcPort);
  ev.desc = "(dest not ready,dest,port)";

  uint64_t t = SimTimeUtil::getNextTime(SimTimeUtil::nodeTimeu(srcNode) + 1);
  SimTimeUtil::addEvent(t, ev);

  SimulatorTransport* src = getTransport(srcNode, srcPort);
  src->prepareDestNotReady(destNode, t);
}
