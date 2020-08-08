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

#include "Simulator.h"
#include "SearchRandomUtil.h"
#include "SearchStepRandomUtil.h"
#include "RandomRandomUtil.h"
//#include "BestFirstRandomUtil.h"
typedef SearchRandomUtil BestFirstRandomUtil;
#include "ReplayRandomUtil.h"
#include "LastNailRandomUtil.h"

namespace macesim {

DistributionMap __Simulator__::distributions;
bool __Simulator__::max_time_error;

void __Simulator__::dumpState() {
  ADD_SELECTORS("__Simulator__::dumpState");
  if (! maceout.isNoop()) {
    SimApplication& app = SimApplication::Instance();
    maceout << "Application State: " << app << std::endl;
    //         Sim& net = SimNetwork::Instance();
    //         maceout << "Network State: " << net << std::endl;
    //         Sim& sched = SimScheduler::Instance();
    //         maceout << "Scheduler State: " << sched << std::endl;
    maceout << "TimeUtil State: " << SimTimeUtil::PrintableInstance() 
        << Log::end;
  }
}

void __Simulator__::initializeVars() {
  use_hash = params::get("USE_STATE_HASHES", false);
  use_broken_hash = params::get("USE_BROKEN_HASH", false);
  loggingStartStep = params::get<unsigned>("LOGGING_START_STEP", 0);
  max_dead_paths = params::get("MAX_DEAD_PATHS", 1);
  num_nodes = params::get<int>("num_nodes");
  max_time_error = params::get<bool>("MAX_TIME_ERROR", true);

  // Make sure our random number generator is the appropriate one
  if (params::containsKey("RANDOM_REPLAY_FILE")) {
    use_hash = false;
    assert_safety = true;
    if (params::get<std::string>("RANDOM_REPLAY_FILE") == "-") {
      print_errors = true;
      RandomUtil::seedRandom(SimTimeUtil::realTimeu());
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
    std::string statfilestr = "lastnail_" + params::get<std::string>("STAT_FILE", "stats.log");
    params::set("ERROR_PATH_FILE_TAG", "-lastnail-");
    if (params::containsKey("MAX_PATH_QUARTILE_3")) {
      params::set("MAX_PATH_OUTLIER_DURATION", params::get<std::string>("MAX_PATH_QUARTILE_3"));
    }
    FILE* statfile = fopen(statfilestr.c_str(), "w");
    if (statfile == NULL) {
      Log::perror("Could not open stat file");
      exit(1);
    }
    Log::add("Sim::printStats",statfile,LOG_TIMESTAMP_DISABLED,LOG_NAME_DISABLED,LOG_THREADID_DISABLED);
    Log::add("DEBUG::Sim::printStats",statfile,LOG_TIMESTAMP_DISABLED,LOG_NAME_DISABLED,LOG_THREADID_DISABLED);
  } else {
    if (params::get("VARIABLE_SRAND", false)) {
      RandomUtil::seedRandom(SimTimeUtil::realTimeu());
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
    else if (params::get("USE_RANDOM_SEARCH", false)) {
      randomUtil = RandomRandomUtil::SetInstance();
      if (params::get("TRACE_TRANSITION_TIMES", false)) {
	Log::logToFile("transition_times.log", "TransitionTimes", "w", 
		       LOG_TIMESTAMP_DISABLED);
      }
    }
    else {
      randomUtil = SearchRandomUtil::SetInstance();
    }
    divergenceMonitor = params::get("RUN_DIVERGENCE_MONITOR",true);
    FILE* statfile = fopen(params::get<std::string>("STAT_FILE", "stats.log").c_str(), "w");
    if (statfile == NULL) {
      Log::perror("Could not open stat file");
      exit(1);
    }
    Log::add("Sim::printStats",statfile,LOG_TIMESTAMP_DISABLED,LOG_NAME_DISABLED,LOG_THREADID_DISABLED);
    Log::add("DEBUG::Sim::printStats",statfile,LOG_TIMESTAMP_DISABLED,LOG_NAME_DISABLED,LOG_THREADID_DISABLED);
  }
  if (!params::get("TRACE_TRANSITION_TIMES", false)) {
    loadDistributions();
  }
}

}
