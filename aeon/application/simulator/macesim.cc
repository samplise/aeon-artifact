/* 
 * simmain.cc : part of the Mace toolkit for building distributed systems
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
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>
#include "mace_constants.h"
#include <string>
#include <sstream>
#include "NumberGen.h"
#include "Log.h"
#include "mace.h"

#include "MaceTime.h"

#include "SimEvent.h"
#include "Sim.h"
#include "SimNetwork.h"
#include "SimScheduler.h"
#include "SimApplication.h"
#include "ThreadCreate.h"
#include "LoadTest.h"
#include "ScopedFingerprint.h"

#include "simmain.h" // simulator_common/
using namespace macesim;

#undef exit
// #undef ASSERT
// #define ASSERT(x)  if (!x) { Log::log("ERROR::ASSERT") << *randomUtil << Log::endl; std::ofstream out(("error"+lexical_cast<std::string>(error_path++)+".path").c_str()); randomUtil->printVSErrorPath(out); out.close(); abort(); }

using boost::lexical_cast;

void setupLoggingSimulator() {
  static const LogSelectorTimestamp printTime = LOG_TIMESTAMP_DISABLED;
  static const LogSelectorThreadId printThreadId = LOG_THREADID_DISABLED;
  
  Log::add("WARNING::Simulator::evaluateDistribution", stdout,
      printTime, LOG_NAME_ENABLED, printThreadId);
  Log::add("stoppingCondition", stdout,
      printTime, LOG_NAME_ENABLED, printThreadId);
}

int main(int argc, char **argv)
{
//   BaseMaceService::_printLower = true;
  setupSigHandler();

  // First load running parameters 
  setupParams(argc, argv);

  // logging
  setupLogging();
  setupLoggingSimulator();
 
  // Checking for the service being tested
  Log::log("MCTest") << "Registered tests: " << MCTest::getRegisteredTests() << Log::endl;
  MCTest* test = MCTest::getTest(params::get<std::string>("CHECKED_SERVICE"));
  if (test == NULL) {
    Log::err() << "Invalid checked service type " << params::get<std::string>("CHECKED_SERVICE") << " specified!" << Log::endl;
    ASSERT(0);
  }
  
  // Make sure the running scheduler is our simulator one.
  SimScheduler::Instance();
  __Simulator__::initializeVars();

  if (__Simulator__::divergenceMonitor) {
    pthread_t tid;
    runNewThread(&tid, monitor, NULL, NULL);
  }

  int num_nodes = __Simulator__::num_nodes;
  Sim::init(num_nodes);
  Sim::maxStep = params::get("max_num_steps", UINT_MAX);
  SimNetwork::SetInstance();

  params::StringMap paramCopy = params::getParams();

  __Simulator__::logAddresses();
  Log::log("main") << "Starting simulation" << Log::endl;

  int num_dead_paths = 0;
  const unsigned maxPaths = (params::containsKey("MAX_PATHS")?params::get<int>("MAX_PATHS"):UINT_MAX);
  mace::string description;
  
  // Loop over the total number of paths (<= MAX_PATHS) that will be searched
  // Init all the nodes for each path, run path till completion (all properties satisfied)
  do {
    ADD_SELECTORS("main");

    //XXX Make common
    macesim::SimulatorFlags::nextPath();

    if (__Simulator__::randomUtil->next()) {
      __Simulator__::disableMonitor();
      Sim::printStats(__Simulator__::randomUtil->getPhase());
      __Simulator__::enableMonitor();
    }

    Sim::clearGusto();

    SimApplicationServiceClass** appNodes = new SimApplicationServiceClass*[num_nodes];

    SimEvent::SetInstance();
    SimTimeUtil::SetInstance();

    test->loadTest(appNodes, num_nodes);
    // Prevent apps from calling maceInit in ServiceTests
    ASSERTMSG(!SimEvent::hasMoreEvents(), "Events queued before SimApplication initialized!");

    SimApplication::SetInstance(appNodes);

    if (__Simulator__::use_hash) {
      __Simulator__::initState();
    }

    if (__Simulator__::loggingStartStep > 0) {
      __Simulator__::doLogging = false;
      Log::disableLogging();
    }

    Sim::PathEndCause cause = Sim::TOO_MANY_STEPS;
    
    macedbg(1) << "Initial events: " << SimTimeUtil::PrintableInstance() << Log::endl;
    
    // Execute a single path completely with maxStep steps (default: 80k)
    // Pick an event to execute, and execute it creating more events to execute.
    // Test the properties at the end of each event
    for (Sim::step = 0; Sim::step < Sim::maxStep; Sim::step++) {
      if (Sim::step == __Simulator__::loggingStartStep) {
        __Simulator__::doLogging = true;
        Log::enableLogging();
      }
      maceout << "Now on simulator step " << Sim::step << " (Rem, maxStep = " << Sim::maxStep << ")" << Log::endl;
      __Simulator__::signalMonitor();

      if (SimEvent::hasMoreEvents()) {
        // Pick the next event which is at the least time. Completely deterministic
        Event ev = SimTimeUtil::beginNextEvent();
        
        try {
          // Execute the event!
          __Simulator__::simulateNode(ev);
          //
        } catch(const Exception& e) {
          maceerr << "Uncaught Mace Exception " << e << Log::endl;
          ABORT("Uncaught Mace Exception");
        } catch(const std::exception& se) {
          maceerr << "Uncaught std::exception " << se.what() << Log::endl;
          ABORT("Uncaught std::exception");
        } catch(...) {
          maceerr << "Uncaught unknown throw" << Log::endl;
          ABORT("Uncaught unknown throw");
        }
	
        if (__Simulator__::distributionsLoaded()) {
	  std::string eventName = mace::ScopedFingerprint::getFingerprint();
	  double frac = (double)__Simulator__::randomUtil->randInt() / UINT_MAX;
	  uint64_t val = __Simulator__::evaluateDistribution(eventName, frac);
	  SimTimeUtil::addTime(val);
	}
	else {
          // Unknown distribution. This is probably the tracing period. Add default 10ms
	  SimTimeUtil::addTime(10000); //10 ms
	}

        __Simulator__::dumpState();
        Sim::updateGusto();
        if (__Simulator__::isDuplicateState()) {
          cause = Sim::DUPLICATE_STATE;
          break;
        }
        else if (__Simulator__::stoppingCondition(Sim::isLive, Sim::isSafe, description)) {
          cause = Sim::STOPPING_CONDITION;
          break;
        }
      } else {
        __Simulator__::stoppingCondition(Sim::isLive, Sim::isSafe, description);
        cause = Sim::NO_MORE_EVENTS;
        break;
      }
    } 

    Sim::pathComplete(cause, Sim::isLive, Sim::isSafe, Sim::step, __Simulator__::randomUtil->pathDepth(), __Simulator__::randomUtil, description);

    //this could certainly move to Sim
    if (cause == Sim::NO_MORE_EVENTS && !Sim::isLive) {
      __Simulator__::Error(mace::string("NO_MORE_STEPS::")+description, ++num_dead_paths >= __Simulator__::max_dead_paths);
    } else if ( cause == Sim::STOPPING_CONDITION && __Simulator__::max_time_error && !Sim::isLive ) {
      __Simulator__::Error(mace::string("PATH_TOO_LONG::")+description, ++num_dead_paths >= __Simulator__::max_dead_paths);
    } else if ( cause == Sim::TOO_MANY_STEPS && __Simulator__::max_steps_error && !Sim::isLive ) {
      __Simulator__::Error(mace::string("MAX_STEPS::")+description, ++num_dead_paths >= __Simulator__::max_dead_paths);
    }

    clearAndReset(appNodes);
    params::setParams(paramCopy);

  } while (Sim::getPathNum() < maxPaths && __Simulator__::randomUtil->hasNext(Sim::isLive));
  Log::enableLogging();
  Log::log("HALT") << "Simulation complete!" << Log::endl;
  __Simulator__::disableMonitor();
  Sim::printStats(__Simulator__::randomUtil->getPhase() + " -- possibly incomplete");
  __Simulator__::enableMonitor();
  Log::log("HALT") << "Final RandomUtil State: " << *__Simulator__::randomUtil << Log::endl;
  __Simulator__::randomUtil->dumpState();

  printf("***falling out of main***\n");
  return 0;

}

