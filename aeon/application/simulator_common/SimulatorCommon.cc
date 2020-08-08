/* 
 * SimulatorCommon.cc : part of the Mace toolkit for building distributed systems
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
#include "SimulatorCommon.h"

namespace macesim {

std::vector<uint32_t> __SimulatorCommon__::nodeState;
std::vector<std::string> __SimulatorCommon__::nodeStateStr;
std::vector<uint32_t> __SimulatorCommon__::initialNodeState;
std::vector<std::string> __SimulatorCommon__::initialNodeStateStr;
mace::hash_map<uint32_t, UIntList> __SimulatorCommon__::systemStates;
uint32_t __SimulatorCommon__::firstState;
bool __SimulatorCommon__::divergenceMonitor = true;
bool __SimulatorCommon__::use_broken_hash;
bool __SimulatorCommon__::use_hash;
bool __SimulatorCommon__::assert_safety;
bool __SimulatorCommon__::max_steps_error;
//bool __SimulatorCommon__::max_time_error;
bool __SimulatorCommon__::print_errors;
bool __SimulatorCommon__::monitorOverride;
bool __SimulatorCommon__::monitorWaiting;
//DistributionMap __SimulatorCommon__::distributions;
int __SimulatorCommon__::error_path = 1;
SimRandomUtil* __SimulatorCommon__::randomUtil;
hash_string __SimulatorCommon__::hasher;
unsigned __SimulatorCommon__::loggingStartStep = 0;
bool __SimulatorCommon__::doLogging = true;
int __SimulatorCommon__::max_dead_paths = 1;
int __SimulatorCommon__::num_nodes = 0;

}
