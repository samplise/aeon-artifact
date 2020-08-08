/* 
 * Sim.cc : part of the Mace toolkit for building distributed systems
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

#define FLUSH_LIST(x) macedbg(0) << #x "_path_steps " << simSteps##x << Log::endl; simSteps##x.clear(); macedbg(0) << #x "_path_rand " << randSteps##x << Log::endl; randSteps##x.clear();
#define ADD_TO_LIST(x) { count##x++; simSteps##x.push_back(simSteps); randSteps##x.push_back(randSteps); if (simSteps##x.size() >= 100) { FLUSH_LIST(x) } }

void Sim::init(int nn) {
  startTime = TimeUtil::timeu();
  SimCommon::init(nn);
}

void Sim::pathComplete(PathEndCause cause, bool isLive, bool isSafe,
                       uint32_t simSteps, uint32_t randSteps,
                       const mace::Printable* const randomUtil,
                       const mace::string& description) {
  ADD_SELECTORS("Sim::pathComplete");
  unsigned mask = params::get("search_print_mask",0x00000fff);
  ASSERT(isSafe);
  cumulativePathCount++;
  pathCount++;
  liveCount++;
  
  if ((cumulativePathCount & mask) == 0) {
    maceout << "Path " << cumulativePathCount << " ( " << pathCount << " in phase) ended at simulator step: " << simSteps << " random step: " << randSteps << " cause: " << getCauseStr(cause) << " " << description << " " << (*randomUtil) << Log::endl;
  }
  {
    ADD_SELECTORS("Sim::printStats");
    switch(cause) {
      case NO_MORE_EVENTS: ADD_TO_LIST(OOE); break;
      case DUPLICATE_STATE: ADD_TO_LIST(Duplicate); break;
      case STOPPING_CONDITION: ADD_TO_LIST(SC); break;
      case TOO_MANY_STEPS: ADD_TO_LIST(TMS); break;
      default: ASSERT(0);
    }
  }
}
    
void Sim::setCurrentNode(int currentNode) {
  SimCommon::setCurrentNode(currentNode);
  LogSelector::prefix = logPrefixes[currentNode];
}

#undef FLUSH_LIST
#undef ADD_TO_LIST
