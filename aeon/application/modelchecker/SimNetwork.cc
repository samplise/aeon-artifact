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
#include "SimEventWeighted.h"
#include "SimNetwork.h"

SimNetwork* SimNetwork::_sim_inst = NULL;
int SimNetworkCommon::failureCount = 0;
const int SimNetwork::MESSAGE_WEIGHT = params::get("MESSAGE_WEIGHT", 8);
    
void SimNetwork::queueDestNotReadyEvent(int destNode, int srcNode, int srcPort) {
  Event ev;
  ev.node = srcNode;
  ev.type = Event::NETWORK;
  ev.simulatorVector.push_back(SimNetworkCommon::DEST_NOT_READY);
  ev.simulatorVector.push_back(destNode);
  ev.simulatorVector.push_back(srcPort);
  ev.desc = "(dest not ready,dest,port)";

  static const int DEST_NOT_READY_WEIGHT = 2;
  SimEventWeighted::addEvent(DEST_NOT_READY_WEIGHT, ev);

  SimulatorTransport* src = getTransport(srcNode, srcPort);
  src->prepareDestNotReady(destNode, 1);
}

