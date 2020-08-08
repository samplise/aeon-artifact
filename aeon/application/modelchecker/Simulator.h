/* 
 * Simulator.h : part of the Mace toolkit for building distributed systems
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

#include "Sim.h"
#include "SimApplication.h"
#include "SimNetwork.h"
#include "SimScheduler.h"
#include "SimulatorCommon.h"

#ifndef SIMULATOR_H
#define SIMULATOR_H

namespace macesim {


class __Simulator__ : public __SimulatorCommon__ {
  public:
    static void initState() {
      ADD_FUNC_SELECTORS;
      __SimulatorCommon__::initState(SimApplication::Instance(),
          SimNetwork::Instance(),
          SimScheduler::Instance());
    }

    static void dumpState();
    
    static bool isDuplicateState() {
      ADD_FUNC_SELECTORS;
      return __SimulatorCommon__::isDuplicateState(SimApplication::Instance(),
                                                   SimNetwork::Instance(),
                                                   SimScheduler::Instance());
    }

    static std::string simulateEvent(const Event& e) {
      switch(e.type) {  
        case Event::APPLICATION:
          return SimApplication::Instance().simulateEvent(e);
        case Event::NETWORK:
          return SimNetwork::Instance().simulateEvent(e);
        case Event::SCHEDULER:
          return SimScheduler::Instance().simulateEvent(e);
      }
      return "";
    }

    static void simulateNode(const Event& e) {
      ADD_FUNC_SELECTORS;
      static log_id_t bid = Log::getId("SimulateEventBegin");
      static log_id_t eid = Log::getId("SimulateEventEnd");

      //Step 1: Load the node's context (parameters & environment)
      Sim::setCurrentNode(e.node);
      mace::LogicalClock& inst = mace::LogicalClock::instance();
      inst.setMessageId(0);
      
      Log::binaryLog(selectorId->log, 
		     StartEventReader_namespace::StartEventReader(e.node, Sim::step * 1000000, true),
		     0);
      
      maceLog("simulating node %d", e.node);

      const char* eventType = eventTypes()[e.type];

      maceout << eventType << Log::endl;
      Log::log(bid) << Sim::step << " " << e.node << " " << eventType << Log::endl;

      std::string r = simulateEvent(e);

      Log::binaryLog(selectorId->log, 
		     StartEventReader_namespace::StartEventReader(e.node, Sim::step * 1000000, false),
		     0);

      Log::log(eid) << Sim::step << " " << e.node << " " << eventType << " " << r << Log::endl;
    }

    static bool stoppingCondition(bool& isLive, bool& isSafe, mace::string& description) {
      int searchDepthTest;
      bool stoppingCond = __SimulatorCommon__::stoppingCondition(isLive, isSafe, searchDepthTest, description);
      if (stoppingCond)
        return stoppingCond;

      return extraStoppingCondition(isLive, searchDepthTest);
    }

    static bool extraStoppingCondition(bool isLive, int searchDepthTest) {
      static const bool printPrefix = params::get("PRINT_SEARCH_PREFIX", false);
      static const bool useRandomWalks = params::get("USE_RANDOM_WALKS", true); // now default to true. 
      static int stopping = 0;

      if (isLive && randomUtil->implementsPrintPath()) {
        if (printPrefix) {
          std::ofstream out(("prefix"+boost::lexical_cast<std::string>(stopping++)+".path").c_str());
          randomUtil->printPath(out);
          out.close();
        }
      }

      return (searchDepthTest >= 0 && randomUtil->pathIsDone() && (!useRandomWalks || isLive));
    }

    static void initializeVars();

};

}

#endif // SIMULATOR_H
