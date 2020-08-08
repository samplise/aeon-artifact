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
#include <math.h>

#include "Distribution.h"
#include "ScopedFingerprint.h"
#include "SimApplication.h"
#include "SimNetwork.h"
#include "SimScheduler.h"
#include "SimTimeUtil.h"
#include "SimulatorCommon.h"

#ifndef SIMULATOR_H
#define SIMULATOR_H

namespace macesim {

typedef std::map<std::string, Distribution*> DistributionMap;

class __Simulator__ : public __SimulatorCommon__ {
  protected:
    static DistributionMap distributions;

  public:
    static bool max_time_error;

    static void initState() {
      ADD_FUNC_SELECTORS;
      __SimulatorCommon__::initState(SimApplication::Instance(),
          SimNetwork::Instance(),
          SimScheduler::Instance());
    }
  
    static bool distributionsLoaded() {
      return distributions.size() != 0;
    }
    
    static void loadDistributions() {
      ADD_FUNC_SELECTORS;
      std::string distFile = params::get<std::string>("LATENCY_DISTRIBUTION_FILE",
						      "distributions.dat");
      std::ifstream in(distFile.c_str());
      if (!in.good()) {
	Log::perror("Could not open distribution file");
	exit(1);
      }
      Distribution* d = new Distribution();
      int cnt = 0;
      
      while (d->load(in) == 0) {
	cnt++;
	distributions[d->getId()] = d;
	d = new Distribution();
      }
      maceout << "Loaded " << cnt << " distributions" << Log::endl;
      delete d;
    }
  
    static uint64_t evaluateDistribution(const std::string& id, double frac) {
      // NOTE: this function assumes the distribution is evenly spaced
      ADD_SELECTORS("Simulator::evaluateDistribution");
      
      ASSERT(frac <= 1);
      DistributionMap::iterator i = distributions.find(id);
      
      if (i != distributions.end()) {
	Distribution* dist = i->second;
	int numVals = dist->getNumValues();
	int lowerIndex = (int)floor(frac * (numVals - 1));
	
	if (lowerIndex == numVals - 1) {
	  return dist->getValue(lowerIndex);
	}
	else {
	  int upperIndex = lowerIndex + 1;
	  double increment = 1.0 / (numVals - 1);
	  double lowerFrac = increment * lowerIndex;
	  uint64_t lowVal = dist->getValue(lowerIndex);
	  uint64_t highVal = dist->getValue(upperIndex);
	  
	  ASSERT(frac >= lowerFrac);
	  ASSERT(highVal >= lowVal);
	  
	  return lowVal + (uint64_t)((frac - lowerFrac) / increment * (highVal - lowVal));
	}
      }
      else {
	macewarn << "Distribution for event \"" << id << "\" not known,"
		 << " assuming 10ms" << Log::endl;
	return 10000;
      }
    }

    static void dumpState();
    static bool isDuplicateState() {
      ADD_FUNC_SELECTORS;
      return __SimulatorCommon__::isDuplicateState(SimApplication::Instance(),
                                                   SimNetwork::Instance(),
                                                   SimScheduler::Instance());
    }

//    static void addSimulatorEvent(const Event& e) {
//      SimApplication::Instance().addSimulatorEvent(e);
//    }

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
      static log_id_t lid = Log::getId("TransitionTimes");
      
      uint64_t startTime = SimTimeUtil::realTimeu();
      //Step 1: Load the node's context (parameters & environment)
      Sim::setCurrentNode(e.node);
      uint64_t nodeTime = SimTimeUtil::nodeTimeu(e.node);
      
      Log::binaryLog(selectorId->log, 
		     StartEventReader_namespace::StartEventReader(e.node, nodeTime, true),
		     0);
      
      macedbg(0) << "simulating event " << e << Log::endl;

      const char* eventType = eventTypes()[e.type];

      maceout << eventType << Log::endl;
      Log::log(bid) << Sim::step << " " << e.node << " " << eventType << Log::endl;

      std::string r = simulateEvent(e);

      Log::binaryLog(selectorId->log, 
		     StartEventReader_namespace::StartEventReader(e.node, SimTimeUtil::nodeTimeu(e.node), false),
		     0);

      uint64_t realTransTime = SimTimeUtil::realTimeu() - startTime;
      Log::log(lid) << realTransTime << " " << Sim::step << " " << e.node << " " << eventType << " " << r << " e = " << e << " event = " << mace::ScopedFingerprint::getFingerprint() << Log::endl;
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
      ADD_FUNC_SELECTORS;
      static const uint64_t minOutlier = params::get("MIN_PATH_OUTLIER_DURATION", 0);
      static const uint64_t maxOutlier = params::get("MAX_PATH_OUTLIER_DURATION", 0);
      static const bool printPrefix = params::get("PRINT_SEARCH_PREFIX", false);
      static const bool useRandomWalks = params::get("USE_RANDOM_WALKS", true); // now default to true. 
      static int stopping = 0;
      static int mincount = 0;
      static int maxcount = 0;
      bool pathTooLong = false;
      
      if (isLive && randomUtil->implementsPrintPath()) {
        if (printPrefix) {
          std::ofstream out(("prefix"+boost::lexical_cast<std::string>(stopping++)+".path").c_str());
          randomUtil->printPath(out);
          out.close();
        }
      
        uint64_t min, max, avg;
        SimTimeUtil::getTimeStats(min, max, avg);
        if (minOutlier && avg < minOutlier) {
          maceout << "Found MIN outlier " << mincount << " ( " << avg << " < " << minOutlier << " ) " << Log::endl;
          std::ofstream out(("minoutlier"+boost::lexical_cast<std::string>(mincount++)+"-"+boost::lexical_cast<std::string>(avg)+".path").c_str());
          randomUtil->printPath(out);
          out.close();
        }
        if (maxOutlier && avg > maxOutlier) {
          pathTooLong = true;
          isLive = false;
          if (!max_time_error) {
            maceout << "Found MAX outlier " << maxcount << " ( " << avg << " > " << maxOutlier << " ) " << Log::endl;
            std::ofstream out(("maxoutlier"+boost::lexical_cast<std::string>(maxcount++)+"-"+boost::lexical_cast<std::string>(avg)+".path").c_str());
            randomUtil->printPath(out);
            out.close();
          }
        }
      }
      
      return (searchDepthTest >= 0 && randomUtil->pathIsDone() && (!useRandomWalks || isLive || pathTooLong));
    }

    static void initializeVars();

};

}

#endif // SIMULATOR_H
