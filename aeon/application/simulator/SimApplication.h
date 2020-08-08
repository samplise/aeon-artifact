/* 
 * SimApplication.h : part of the Mace toolkit for building distributed systems
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
#ifndef SIM_APP_H
#define SIM_APP_H

#include "Sim.h"
#include "SimApplicationCommon.h"
#include "SimTimeUtil.h"

class SimApplication : public SimApplicationCommon {
 protected:
  class SimNodeHandler : public SimNodeHandlerCommon {
   public:
    SimNodeHandler(int nodenum, SimApplicationServiceClass* nodeapp)
        : SimNodeHandlerCommon(nodenum, nodeapp) { }

    void addInitEvent() {
      ASSERT(!eventSimulated);
      // Schedule maceInit
      Event e;
      e.node = nodeNum;
      e.type = Event::APPLICATION;
      e.simulatorVector.push_back(SimApplication::APP); 
      e.desc = "(maceInit)";

      SimTimeUtil::addEvent(SimTimeUtil::getNextTime(SimTimeUtil::nodeTimeu(nodeNum)), e);
    }

    void addFailureEvent() {
//     static bool simulateFailure = params::get("SIM_NODE_FAILURE", false);
      static int numFailures = params::get("SIM_NUM_FAILURES", 0);
      if (numFailures == 0)
        return;
//     typedef mace::deque<int> IntList;
//     static IntList failNodes = params::getList<int>("SIM_FAIL_NODES");
     ASSERTMSG(false, "Untested Failures on Simulator");

//     if(simulateFailure && eventSimulated && RandomUtil::randInt(2, 1, 0)) {
//       pending = true;
//       Event e;
//       e.node = nodeNum;
//       e.type = Event::APPLICATION;
//       e.simulatorVector.push_back(SimApplication::RESET); 
//       e.desc = "(Node Reset)";
//
//       SimTimeUtil::addEvent(SimTimeUtil::getNextTime(SimTimeUtil::nodeTimeu(nodeNum)+2), e);
//     }
   }

   void checkAndClearFailureEvents() {
//     static int numFailures = params::get("SIM_NUM_FAILURES", 0);
//     ASSERTMSG(false, "Untested Failures on Simulator");
//
//     if (!(numFailures < 0 || failureCount < numFailures)) {
//     // Possibly maintain a list of times of failures and delete it?
//     }
   }
 };

protected:
  typedef mace::vector<ServiceClass*, mace::SoftState> ServiceClassVector;
  typedef mace::vector<ServiceClassVector, mace::SoftState> ServicePrintVector;

  static SimApplication* _sim_inst;
  
  SimApplication(SimApplicationServiceClass** tnodes) : SimApplicationCommon(tnodes) {
    // Instantitate the per-node handlers
    for (int i = 0; i < Sim::getNumNodes(); i++) {
      nodeHandlers[i] = new SimNodeHandler(i, nodes[i]);
      // Init the nodes
      Sim::setCurrentNode(i);
      nodeHandlers[i]->initNode();
    }
  }

  static bool CheckInstance() { 
    ASSERT(_sim_inst != NULL);
    return true;
  }
  
public:
  static SimApplication& Instance() { static bool dud __attribute((unused)) = CheckInstance(); return *_sim_inst; }
  static void SetInstance(SimApplicationServiceClass** nodes) {
    ASSERT(_sim_inst == NULL);
    ASSERT(nodes != NULL);
    _sim_inst = new SimApplication(nodes);
  }

  static void reset() {
    delete _sim_inst;
    _sim_inst = NULL;
  }

  virtual ~SimApplication() {
    for(int i = 0; i < Sim::getNumNodes(); i++) {
      delete nodeHandlers[i];
    }
    delete[] nodeHandlers;
    nodeHandlers = NULL;
  }
};

#endif // SIM_APP_H
