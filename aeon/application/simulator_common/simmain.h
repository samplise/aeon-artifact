/* 
 * simmain.h : part of the Mace toolkit for building distributed systems
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
#ifndef SIMMAIN_H
#define SIMMAIN_H

#include "Log.h"
#include "Simulator.h"
#include "params.h"
#include "SimulatorBasics.h"

#include "load_protocols.h"

#include "SysUtil.h"

namespace SimulatorUDP_namespace {
    void load_protocol();
}
namespace SimulatorTCP_namespace {
    void load_protocol();
}

namespace macesim {
  
void* monitor(void* dud) {
  ADD_FUNC_SELECTORS;
  bool divergence_assert = params::containsKey("divergence_assert");
  maceout << "monitor begun" << Log::endl;
  int timeout = params::get("divergence_timeout", 5);
  __Simulator__::expireMonitor();
  while (true) {
    SysUtil::sleep(timeout);
    if (__Simulator__::expireMonitor()) {
      Log::enableLogging();
      __Simulator__::Error("DIVERGENCE", divergence_assert);
    }
  }
  return NULL;
}

void sigHandler(int sig) {
//   std::cerr << "received signal " << sig << " my pid=" << getpid() << std::endl;
  if (sig == SIGINT && __Simulator__::randomUtil->handleInt()) {
    return;
  }
  Log::enableLogging();
  Log::log("ERROR::SIM::SIGNAL") << "Caught Signal " << sig << ", terminating" << Log::endl;
  __Simulator__::Error("SIGNAL");
}

void setupSigHandler() {
  SysUtil::signal(SIGABRT, sigHandler);
  SysUtil::signal(SIGINT, sigHandler);
  //   SysUtil::signal(SIGINT, sigHandler);
}

void setupParams(int argc, char **argv) {
  params::addRequired("num_nodes", "Number of nodes to simulate");
  params::addRequired("CHECKED_SERVICE", "Which service string to use");
  params::loadparams(argc, argv);
  params::set("MACE_SIMULATE_NODES", "MaceMC");

  // log the params to console
  params::print(stdout);

  // log the params to the binary log
  params::log();
}

void setupLogging() {
  Log::configure();    
  Log::disableDefaultWarning();
  Log::disableDefaultError();
  //   LogSelectorTimestamp printTime = LOG_TIMESTAMP_HUMAN;
  static const LogSelectorTimestamp printTime = LOG_TIMESTAMP_DISABLED;
  static const LogSelectorThreadId printThreadId = LOG_THREADID_DISABLED;

  Log::add("ERROR", stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  Log::autoAdd(".*ERROR::SIM.*", stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  Log::add("SearchRandomUtil::next", stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  Log::add("SearchStepRandomUtil::next", stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  Log::add("BestFirstRandomUtil::next", stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  Log::add("HALT",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  Log::add("MCTest",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  Log::autoAdd(".*monitor.*", stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  Log::add("Sim::pathComplete", stdout, printTime, LOG_NAME_ENABLED);
  Log::add("Sim::printStats", stdout, printTime, LOG_NAME_ENABLED);
  //   Log::add("DEBUG::static bool __Simulator__::isDuplicateState()",stdout,printTime,LOG_NAME_ENABLED);
  Log::autoAdd(".*LastNailRandomUtil::hasNext.*", stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  Log::add("LastNailRandomUtil::next", stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  Log::autoAdd(".*LastNailRandomUtil::dumpState.*", stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  if (params::containsKey("TRACE_ALL")) {
    Log::autoAdd(".*",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  }
  if (params::containsKey("TRACE_ALL_BINARY")) {
    Log::autoAdd(".*",stdout, printTime, LOG_NAME_ENABLED, printThreadId, LOG_BINARY);
  }
  if (params::containsKey("TRACE_ALL_SQL")) {
    Log::autoAdd(".*",stdout, printTime, LOG_NAME_ENABLED, printThreadId, LOG_PGSQL);
  }

  if (params::containsKey("TRACE_SIMULATOR")) {
    Log::add("BestFirstRandomUtil::hasNext",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    Log::add("main",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    //     Log::autoAdd("Simulator");
    Log::add("__Simulator__::getReadyEvents",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    Log::autoAdd(".*__Simulator__::isDuplicateState.*",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    Log::autoAdd(".*__Simulator__::initState.*",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    Log::autoAdd(".*simulateNode.*",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    Log::autoAdd("SimulateEventBegin",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    Log::autoAdd("SimulateEventEnd",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    Log::autoAdd(".*simulateEvent.*",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    Log::add("SearchRandomUtil::randInt",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    Log::add("SearchStepRandomUtil::randInt",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    Log::add("BestFirstRandomUtil::randInt",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    Log::add("LastNailRandomUtil::randInt",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    Log::add("ReplayRandomUtil::randInt",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  }
  if (params::containsKey("TRACE_SUBST")) {
    std::istringstream in(params::get<std::string>("TRACE_SUBST"));
    while (in) {
      std::string s;
      in >> s;
      if (s.length() == 0) { break; }
      Log::autoAdd(s,stdout, printTime, LOG_NAME_ENABLED, printThreadId);
    }
  }
  if (params::containsKey("TRACE_STATE")) {
    Log::autoAdd(".*__Simulator__::dumpState.*");
  }
  if (params::containsKey("TRACE_STEP")) {
    Log::add("main",stdout, printTime, LOG_NAME_ENABLED, printThreadId);
  }

  // Not really part of logging, but did not want to create another function.
  macesim::SimulatorFlags::configureSimulatorFlags();
  load_protocols();
  SimulatorTCP_namespace::load_protocol();
  SimulatorUDP_namespace::load_protocol();
}

void clearAndReset(SimApplicationServiceClass **appNodes) {
  for (int i = 0; i < __Simulator__::num_nodes; i++) {
    Sim::setCurrentNode(i);
    appNodes[i]->maceExit();
  }

  ServiceClass::deleteServices();

  SimScheduler::Instance().reset();
  SimNetwork::Instance().reset();
  SimApplication::reset();
  NumberGen::Reset();

  delete[] appNodes;

  TestProperties::clearTests();
}

}

#endif // SIMMAIN_H
