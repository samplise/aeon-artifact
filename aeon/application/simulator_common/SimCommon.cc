/* 
 * SimCommon.cc : part of the Mace toolkit for building distributed systems
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
#include "SimCommon.h"

#define FLUSH_LIST(x) macedbg(0) << #x "_path_steps " << simSteps##x << Log::endl; simSteps##x.clear(); macedbg(0) << #x "_path_rand " << randSteps##x << Log::endl; randSteps##x.clear();
#define ADD_TO_LIST(x) { count##x++; simSteps##x.push_back(simSteps); randSteps##x.push_back(randSteps); if (simSteps##x.size() >= 100) { FLUSH_LIST(x) } }


bool SimCommon::gusto = false;
bool SimCommon::useGusto = false;
bool SimCommon::gustoReq = false;
bool SimCommon::isLive = false;
bool SimCommon::isSafe = true;
unsigned SimCommon::step = 0;
unsigned SimCommon::maxStep = 0;
int SimCommon::numNodes = 0;
std::string** SimCommon::logPrefixes = NULL;
std::string SimCommon::logPrefix;
MaceKey** SimCommon::keys = NULL;
std::string* SimCommon::nodeString = NULL;
bool SimCommon::inited = false;
unsigned SimCommon::phaseNum = 0;
unsigned SimCommon::pathCount = 0;
unsigned SimCommon::cumulativePathCount = 0;
unsigned SimCommon::uniqueStateCount = 0;
unsigned SimCommon::cumulativeUniqueStateCount = 0;
unsigned SimCommon::countTMS = 0;
unsigned SimCommon::cumulativeTmsCount = 0;
unsigned SimCommon::countOOE = 0;
unsigned SimCommon::cumulativeOoeCount = 0;
unsigned SimCommon::countSC = 0;
unsigned SimCommon::cumulativeScCount = 0;
unsigned SimCommon::countDuplicate = 0;
unsigned SimCommon::cumulativeDuplicateCount = 0;
unsigned SimCommon::liveCount = 0;
unsigned SimCommon::cumulativeLiveCount = 0;
mace::deque<unsigned> SimCommon::simStepsDuplicate;
mace::deque<unsigned> SimCommon::randStepsDuplicate;
mace::deque<unsigned> SimCommon::simStepsTMS;
mace::deque<unsigned> SimCommon::randStepsTMS;
mace::deque<unsigned> SimCommon::simStepsOOE;
mace::deque<unsigned> SimCommon::randStepsOOE;
mace::deque<unsigned> SimCommon::simStepsSC;
mace::deque<unsigned> SimCommon::randStepsSC;
uint64_t SimCommon::startTime;
uint64_t SimCommon::lastPhaseEnd;

void SimCommon::init(int nn) {
  ASSERT(!inited);
  inited = true;
  lastPhaseEnd = startTime;
  useGusto = params::get("USE_GUSTO", false);

  numNodes = nn;
  logPrefixes = new std::string*[numNodes];
  keys = new MaceKey*[numNodes];
  nodeString = new std::string[numNodes];
  for (int i = 0; i < numNodes; i++) {
    keys[i] = new MaceKey(ipv4, i+1);
    logPrefixes[i] = new std::string(keys[i]->toString()+" ");
    nodeString[i] = boost::lexical_cast<std::string>(i);
  }
  LogSelector::prefix = &logPrefix;
}

void SimCommon::printStats(const std::string& nextPhaseStr) {
  ADD_SELECTORS("Sim::printStats");
  uint64_t now = TimeUtil::timeu(); // SimTimeUtil::realTimeu();
  cumulativeUniqueStateCount += uniqueStateCount;
  cumulativeDuplicateCount += countDuplicate;
  cumulativeOoeCount += countOOE;
  cumulativeScCount += countSC;
  cumulativeTmsCount += countTMS;
  cumulativeLiveCount += liveCount;
  phaseNum++;
  if (!maceout.isNoop()) {
    FLUSH_LIST(Duplicate);
    FLUSH_LIST(OOE);
    FLUSH_LIST(SC);
    FLUSH_LIST(TMS);
    maceout << "Phase time " << (now - lastPhaseEnd) << " (cumulative) " << (now - startTime) << Log::endl;
    maceout << "Unique state count: " << uniqueStateCount << " (cumulative) " << cumulativeUniqueStateCount << Log::endl;
    maceout << "Paths Searched: " << pathCount << " (cumulative) " << cumulativePathCount << Log::endl;
    maceout << "Live path count: " << liveCount << " (cumulative) " << cumulativeLiveCount << Log::endl;
    maceout << "Abandonded duplicates: " << countDuplicate << " (cumulative) " << cumulativeDuplicateCount << Log::endl;
    maceout << "Out of events count: " << countOOE << " (cumulative) " << cumulativeOoeCount << Log::endl;
    maceout << "Stopping Condition count: " << countSC << " (cumulative) " << cumulativeScCount << Log::endl;
    maceout << "Too Many Steps count: " << countTMS << " (cumulative) " << cumulativeTmsCount << Log::endl;
    maceout << "Now on phase " << phaseNum << " : " << nextPhaseStr << Log::endl;
  }
  lastPhaseEnd = now;
  simStepsDuplicate.clear();
  randStepsDuplicate.clear();
  simStepsOOE.clear();
  randStepsOOE.clear();
  simStepsSC.clear();
  randStepsSC.clear();
  simStepsTMS.clear();
  randStepsTMS.clear();
  uniqueStateCount = 0;
  pathCount = 0;
  countDuplicate = 0;
  countOOE = 0;
  countSC = 0;
  countTMS = 0;
  liveCount = 0;
}

