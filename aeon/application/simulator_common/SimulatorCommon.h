/* 
 * SimulatorCommon.h : part of the Mace toolkit for building distributed systems
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
#ifndef SIMULATOR_COMMON_H
#define SIMULATOR_COMMON_H

#include <fstream>
#include <map>
#include <signal.h>
#include <string>
#include <vector>

#include "SimulatorBasics.h"

#include "Log.h"
#include "MaceTime.h"
#include "Properties.h"
#include "SimApplicationCommon.h"
#include "SimCommon.h"
#include "SimNetworkCommon.h"
#include "SimRandomUtil.h"
#include "SimSchedulerCommon.h"
#include "StartEventReader.h"
#include "mace-macros.h"
#include "mhash_map.h"
#include "mstring.h"
#include "params.h"

namespace macesim {


class __SimulatorCommon__ {
  protected:
    static std::vector<uint32_t> nodeState;
    static std::vector<std::string> nodeStateStr;
    static std::vector<uint32_t> initialNodeState;
    static std::vector<std::string> initialNodeStateStr;
    static mace::hash_map<uint32_t, UIntList> systemStates;
    static uint32_t firstState;
    static hash_string hasher;
    static int error_path;
    static bool monitorOverride;
    static bool monitorWaiting;

  public:
    static bool divergenceMonitor;
    static bool use_broken_hash;
    static bool use_hash;
    static bool assert_safety;
    static bool print_errors;
    static bool max_steps_error;
    static SimRandomUtil* randomUtil;
    typedef std::map<UIntList, char*> MemoryMap;
    static unsigned loggingStartStep;
    static bool doLogging;
    static int max_dead_paths;
    static int num_nodes;
    
  public:
    static void initState(SimApplicationCommon& app,
                          SimNetworkCommon& net,
                          SimSchedulerCommon& sched) {
      ADD_FUNC_SELECTORS;
      if (!use_hash) { return; }
      static bool inited = false;
      if (!inited) {
        //SimApplication& app = SimApplication::Instance();
        std::ostringstream out;
        inited = true;
        nodeState.resize(num_nodes);
        nodeStateStr.resize(num_nodes);
        firstState = 0;
        //XXX: update nodeState
        for(int i = 0; i < num_nodes; i++) {
          if (use_broken_hash) {
            nodeState[i] = app.hashNode(i);
          } else {
            nodeStateStr[i] = app.toStateString(i);
            out.write(nodeStateStr[i].data(), nodeStateStr[i].size());
            nodeState[i] = hasher(nodeStateStr[i]);
            firstState += nodeState[i];
          }
          maceout << "node " << i << " state " << nodeState[i] << Log::endl;
        }
        initialNodeState = nodeState;
        initialNodeStateStr = nodeStateStr;

        //Sim& net = SimNetwork::Instance();
        if (use_broken_hash) {
          uint32_t tempState = net.hashState();
          maceout << "net state " << tempState << Log::endl;
          firstState += tempState;
        } else {
          net.print(out);
        }

        //Sim& sched = SimScheduler::Instance();
        if (use_broken_hash) {
          uint32_t tempState = sched.hashState();
          maceout << "sched state " << tempState << Log::endl;
          firstState += tempState;
        } else {
          sched.print(out);
        }

        if (!use_broken_hash) {
          firstState = hasher(out.str());
        }

        maceout << "firstState " << firstState << Log::endl;
        if (randomUtil->testSearchDepth() > -2) {
          systemStates[firstState] = randomUtil->getPath();
        }
      }
      else {
        nodeState = initialNodeState;
        nodeStateStr = initialNodeStateStr;
      }
    }

  static void dumpState();

    static bool isDuplicateState(SimApplicationCommon& app,
                                 SimNetworkCommon& net,
                                 SimSchedulerCommon& sched) {
      ADD_FUNC_SELECTORS;
      if (!use_hash) { return false; }
      int depthTest = randomUtil->testSearchDepth();
      if (depthTest > 0) { return false; } //If search is past the interesting range, state is not considered duplicate
      uint32_t tempState = 0;
      //SimApplication& app = SimApplication::Instance();
      nodeStateStr[SimCommon::getCurrentNode()] = app.toStateString(SimCommon::getCurrentNode());
      if (use_broken_hash) {
        nodeState[SimCommon::getCurrentNode()] = app.hashNode(SimCommon::getCurrentNode());
      } else {
        nodeState[SimCommon::getCurrentNode()] = hasher( nodeStateStr[SimCommon::getCurrentNode()]);
      }
      macedbg(1) << "current node " << SimCommon::getCurrentNode() << " state " << nodeState[SimCommon::getCurrentNode()] << Log::endl;
      // XXX - fix this for best first
//       if (depthTest < 0) { return false; } //If depth is before the search portion, don't bother noticing duplicate state
      if (depthTest < -1) { return false; } //If depth during the prefix portion, don't bother noticing duplicate state
      std::ostringstream out;
      for(int i = 0; i < num_nodes; i++) {
        if (use_broken_hash) {
          tempState += nodeState[i];
        }
        out <<  nodeStateStr[i];
      }

      //Sim& net = SimNetwork::Instance();
      if (use_broken_hash) {
        tempState += net.hashState();
      }
      net.printState(out);

      //Sim& sched = SimScheduler::Instance();
      if (use_broken_hash) {
        tempState += sched.hashState();
      }
      sched.printState(out);

      if (!use_broken_hash) {
        tempState = hasher(out.str());
      }
      
      macedbg(1) << "current path " << *randomUtil << Log::endl;
      macedbg(1) << "current state " << tempState << " known states " << systemStates << Log::endl;
      if (!macedbg(1).isNoop()) {
        macedbg(1) << out.str() << Log::endl;
      }

      if (systemStates.containsKey(tempState)) { 
        UIntList& selections = systemStates[tempState];
        UIntList current = randomUtil->getPath();
        if (current.size() < selections.size()) {
          macedbg(1) << "Affirming shorter path with same state" << Log::endl;
          systemStates[tempState] = current;
          return false;
        } else if (selections == current) {
          return false;
        }
        //macedbg(0) << "State Duplicated! " << selections << Log::endl;
        macedbg(0) << "State Duplicated!" << Log::endl;

        return true;
      }

      systemStates[tempState] = randomUtil->getPath();
      SimCommon::markUniqueState();
      return false;
    }

    static void Halt() {
      monitorOverride = true;
      SimCommon::printStats(randomUtil->getPhase() + "::ERROR");
      monitorOverride = false;
      if (print_errors) {
        Log::log("HALT") << "Output paths errorN.path for 0 < N < " << error_path << Log::endl;
      }
      raise(11);
    }
    static void Error(const std::string& type, bool exit = true) {
      if (print_errors) {
        Log::log("ERROR::SIM::"+type) << *randomUtil << Log::endl;
        static std::string error_tag = params::get<std::string>("ERROR_PATH_FILE_TAG", "");
        static std::string output_dir = params::get<std::string>("OUTPUT_PATH", "");
        std::ofstream out((output_dir+"error"+error_tag+boost::lexical_cast<std::string>(error_path++)+".path").c_str());
        monitorOverride = true;
        randomUtil->printPath(out);
        monitorOverride = false;
        out.close();
      } else {
        Log::log("ERROR::SIM::"+type) << "error occured of type " << type << Log::endl;
      }
      if (exit) {
        Log::log("ERROR::SIM::"+type) << "exiting via signal 11" << Log::endl;
        Halt();
      } else {
        Log::log("ERROR::SIM::"+type) << "continuing as requested, " << error_path << " paths so far" << Log::endl;
      }
    }

    static void logAddresses() {
      static const log_id_t printAddress = Log::getId("NodeAddress");
      for(int i = 0; i < SimCommon::getNumNodes(); i++) {
        Log::log(printAddress) << "Node " << i << " address " << SimCommon::getMaceKey(i) << Log::endl;
      }
    }

    static const char** eventTypes() {
      static const char* types[] = {"APP_EVENT", "NET_EVENT", "SCHED_EVENT"};
      return types;
    }

    static bool stoppingCondition(bool& isLive, bool& isSafe, int& searchDepthTest, mace::string& description) {
      static bool testEarly = params::get("TEST_PROPERTIES_EARLY", true);

      ADD_SELECTORS("stoppingCondition");

      searchDepthTest = randomUtil->testSearchDepth();

      isSafe = true;

      if (testEarly || searchDepthTest >= 0) {
        for(TestPropertyList::const_iterator i = TestProperties::list().begin(); i != TestProperties::list().end(); i++) {
          isSafe = (*i)->testSafetyProperties(description);
          if (!isSafe) {
            maceerr << "Safety property failed: " << description << Log::endl;
            ASSERTMSG(!assert_safety, "Safety property failed and assert_safety was true, usually for replay");
            Error(std::string("PROPERTY_FAILED::")+description);
          }
        } 

        if (!isSafe) { 
          isLive = false; // An unsafe path is definitely not a live path
          return true; // Unsafe paths definitely mean 
        }

        isLive = true;

        for(TestPropertyList::const_iterator i = TestProperties::list().begin(); i != TestProperties::list().end() && isLive; i++) {
          isLive &= (*i)->testLivenessProperties(description);
        }
      } else {
        description = "(not tested)";
        isLive = false;
      }

      if (isLive) {
        description = "(OK)";
      }

      return false;
    }

    static void signalMonitor() {
      ADD_SELECTORS("signalMonitor");
      macedbg(0) << "called" << Log::endl;
      monitorWaiting = false;
    }
    static void disableMonitor() {
      ADD_SELECTORS("disableMonitor");
      macedbg(0) << "called" << Log::endl;
      monitorOverride = true;
    }
    static void enableMonitor() {
      ADD_SELECTORS("enableMonitor");
      macedbg(0) << "called" << Log::endl;
      monitorWaiting = false;
      monitorOverride = false;
    }
    static bool expireMonitor() {
      ADD_SELECTORS("expireMonitor");
      macedbg(0) << "called" << Log::endl;
      bool w = monitorWaiting;
      monitorWaiting = true;
      macedbg(0) << "waiting: " << w << " override: " << monitorOverride << Log::endl;
      return w && !monitorOverride;
    }

    static void initializeVars();

};

}

#endif // SIMULATOR_COMMON_H
