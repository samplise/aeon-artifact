/* 
 * SimulatorUDP.cc : part of the Mace toolkit for building distributed systems
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
#include "SimEventWeighted.h"
#include "SimulatorUDP.h"
#include "ServiceConfig.h"

namespace SimulatorUDP_namespace {

void SimulatorUDPService::addNetEvent(int destNode) {
  Event ev;
  ev.node = destNode;
  ev.type = Event::NETWORK;
  ev.simulatorVector.push_back(SimNetworkCommon::MESSAGE);
  ev.simulatorVector.push_back(localNode);
  ev.simulatorVector.push_back(localPort);
  ev.desc = "(MESSAGE EVENT,src,port)";

  SimEventWeighted::addEvent(SimNetwork::MESSAGE_WEIGHT, ev);
}

void SimulatorUDPService::enqueueEvent(int destNode) {
  if (isAvailableMessage(destNode)) {
    addNetEvent(destNode);
  }
}
  
void SimulatorUDPService::queueMessage(int destNode, uint32_t msgId, registration_uid_t handlerUid, const std::string& msg) {
  MessageQueue& mqueue = queuedMessages[destNode];

  mqueue.push_back(SimulatorMessage(localNode, destNode, msgId, -1, -1, 0, 0, SimulatorMessage::MESSAGE, handlerUid, msg));

  if (mqueue.size() == 1) {
    addNetEvent(destNode);
  }
}

TransportServiceClass& configure_new_SimulatorUDP_Transport(bool shared) {
    return *(new SimulatorUDPService(std::numeric_limits<uint16_t>::max(), shared));
}

void load_protocol() {
    mace::ServiceFactory<TransportServiceClass>::registerService(&configure_new_SimulatorUDP_Transport, "SimulatorUDP");
    mace::ServiceConfig<TransportServiceClass>::registerService("SimulatorUDP", mace::makeStringSet("lowlatency,ipv4,SimulatorUDP,UdpTransport",","));
}

}