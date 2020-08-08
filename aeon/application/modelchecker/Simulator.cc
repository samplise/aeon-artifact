/* 
 * Simulator.cc : part of the Mace toolkit for building distributed systems
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

#include "PrintNodeFormatter.h"
#include "Simulator.h"
#include "SearchRandomUtil.h"
#include "SearchStepRandomUtil.h"
//#include "BestFirstRandomUtil.h"
typedef SearchRandomUtil BestFirstRandomUtil;
#include "ReplayRandomUtil.h"
#include "LastNailRandomUtil.h"

namespace macesim {

void __Simulator__::dumpState() {
  static const bool formattedState = params::get("PRINT_STATE_FORMATTED", true);
  ADD_SELECTORS("__Simulator__::dumpState");
  if (! maceout.isNoop()) {
    SimApplication& app = SimApplication::Instance();

    if (formattedState) {
      mace::PrintNode pr("root", "");
      app.printNode(pr, "SimApplication");
      std::ostringstream os;
      mace::PrintNodeFormatter::print(os, pr);
      maceout << "Begin Printer State" << std::endl << os.str() << Log::endl;
      maceout << "End Printer State" << Log::endl;


      //       std::ostringstream longos;
      //       mace::PrintNodeFormatter::printWithTypes(longos, pr);
      //       maceout << longos.str() << std::endl;
    }
    else {
      maceout << "Application State: " << app << Log::endl;
    }
    maceout << "EventsPending State: " << SimEventWeighted::PrintableInstance() << Log::endl;
    SimNetwork& net = SimNetwork::Instance();
    maceout << "Network State: " << net << Log::endl;
//         Sim& sched = SimScheduler::Instance();
//         maceout << "Scheduler State: " << sched << Log::endl;
  }
}

void __Simulator__::initializeVars() {
  use_hash = params::get("USE_STATE_HASHES", false);
  use_broken_hash = params::get("USE_BROKEN_HASH", false);
  loggingStartStep = params::get<unsigned>("LOGGING_START_STEP", 0);
  max_dead_paths = params::get("MAX_DEAD_PATHS", 1);
  num_nodes = params::get<int>("num_nodes");

  // Make sure our random number generator is the appropriate one
  if (params::containsKey("RANDOM_REPLAY_FILE")) {
    use_hash = false;
    assert_safety = true;
    if (params::get<std::string>("RANDOM_REPLAY_FILE") == "-") {
      print_errors = true;
      RandomUtil::seedRandom(TimeUtil::timeu());
    } else {
      print_errors = false;
      RandomUtil::seedRandom(0);
    }
    max_steps_error = true;
    randomUtil = ReplayRandomUtil::SetInstance();
    divergenceMonitor = params::get("RUN_DIVERGENCE_MONITOR",false);
  } else if (params::containsKey("LAST_NAIL_REPLAY_FILE")) {
    RandomUtil::seedRandom(0);
    max_dead_paths = INT_MAX;
    use_hash = false;
    assert_safety = false;
    //     print_errors = false;
    print_errors = true;
    max_steps_error = false;
    randomUtil = LastNailRandomUtil::SetInstance();
    divergenceMonitor = params::get("RUN_DIVERGENCE_MONITOR",true);
    std::string sfilename(params::get<std::string>("OUTPUT_PATH", "") + "lastnail_" + params::get<std::string>("STAT_FILE", "stats.log"));
    params::set("ERROR_PATH_FILE_TAG", "-lastnail-");
    FILE* statfile = fopen(sfilename.c_str(), "w");
    if (statfile == NULL) {
      Log::perror("Could not open stat file");
      exit(1);
    }
    Log::add("Sim::printStats",statfile,LOG_TIMESTAMP_DISABLED,LOG_NAME_DISABLED,LOG_THREADID_DISABLED);
    Log::add("DEBUG::Sim::printStats",statfile,LOG_TIMESTAMP_DISABLED,LOG_NAME_DISABLED,LOG_THREADID_DISABLED);
  } else {
    if (params::get("VARIABLE_SRAND", false)) {
      RandomUtil::seedRandom(TimeUtil::timeu());
    }
    else {
      RandomUtil::seedRandom(0);
    }
    print_errors = true;
    assert_safety = false;
    max_steps_error = true;
    if (params::get("USE_BEST_FIRST", true)) {
      randomUtil = BestFirstRandomUtil::SetInstance();
    }
    else if (params::get("USE_STEP_SEARCH", false)) {
      randomUtil = SearchStepRandomUtil::SetInstance();
    }
    else {
      randomUtil = SearchRandomUtil::SetInstance();
    }
    if (params::get("TRACE_TRANSITION_TIMES", false)) {
      Log::logToFile("transition_times.log", "TransitionTimes", "w", 
                     LOG_TIMESTAMP_DISABLED);
    }
    divergenceMonitor = params::get("RUN_DIVERGENCE_MONITOR",true);
    std::string sfilename(params::get<std::string>("OUTPUT_PATH", "") + params::get<std::string>("STAT_FILE", "stats.log"));
    FILE* statfile = fopen(sfilename.c_str(), "w");
    if (statfile == NULL) {
      Log::perror("Could not open stat file");
      exit(1);
    }
    Log::add("Sim::printStats",statfile,LOG_TIMESTAMP_DISABLED,LOG_NAME_DISABLED,LOG_THREADID_DISABLED);
    Log::add("DEBUG::Sim::printStats",statfile,LOG_TIMESTAMP_DISABLED,LOG_NAME_DISABLED,LOG_THREADID_DISABLED);
  }
}

}
