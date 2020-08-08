/* 
 * SimCommon.h : part of the Mace toolkit for building distributed systems
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
#ifndef SIM_COMMON_H
#define SIM_COMMON_H

#include "SimulatorBasics.h"
#include "Log.h"
#include "MaceTime.h"
#include "MaceTypes.h"
#include "SimEvent.h"
#include "mace-macros.h"
#include "mdeque.h"
#include "params.h"

class SimCommon : public mace::PrintPrintable {
  protected:
    static int numNodes;
    static std::string** logPrefixes;
    static std::string logPrefix;
    static MaceKey** keys;
    static std::string* nodeString;
    static bool inited;

    static bool gusto;
    static bool gustoReq;
    static bool useGusto;

  public: //TODO: make non-public
    static bool isLive;
    static bool isSafe;
    static unsigned step;
    static unsigned maxStep;

  protected:
    //stats
    static unsigned phaseNum;
    static unsigned pathCount;
    static unsigned cumulativePathCount;
    static unsigned uniqueStateCount;
    static unsigned cumulativeUniqueStateCount;
    static unsigned countTMS;
    static unsigned cumulativeTmsCount;
    static unsigned countOOE;
    static unsigned cumulativeOoeCount;
    static unsigned countSC;
    static unsigned cumulativeScCount;
    static unsigned countDuplicate;
    static unsigned cumulativeDuplicateCount;
    static unsigned liveCount;
    static unsigned cumulativeLiveCount;
    static mace::deque<unsigned> simStepsDuplicate;
    static mace::deque<unsigned> randStepsDuplicate;
    static mace::deque<unsigned> simStepsTMS;
    static mace::deque<unsigned> randStepsTMS;
    static mace::deque<unsigned> simStepsOOE;
    static mace::deque<unsigned> randStepsOOE;
    static mace::deque<unsigned> simStepsSC;
    static mace::deque<unsigned> randStepsSC;
    static uint64_t startTime;
    static uint64_t lastPhaseEnd;

  public:
    enum PathEndCause { NO_MORE_EVENTS, DUPLICATE_STATE, STOPPING_CONDITION, TOO_MANY_STEPS };
    static void init(int nn);
    static int getNumNodes() { return numNodes; }
    static int getCurrentNode() { return macesim::SimulatorFlags::getCurrentNode(); }
    static const MaceKey& getCurrentMaceKey() { return getMaceKey(getCurrentNode()); }
    static const MaceKey& getMaceKey(int node) { ASSERT(node < numNodes); return *keys[node]; }
    static int getNode(const MaceKey& key) {
      return key.getMaceAddr().local.addr-1;
    }
    //     static int getNode(const MaceKey& key) {
    //       for(int i = 0; i < numNodes; i++) {
    //         if(*(keys[i]) == key) {
    //           return i;
    //         }
    //       }
    //     }
    static void setCurrentNode(int currentNode) {
      ASSERT(currentNode < numNodes);
      macesim::SimulatorFlags::setCurrentNode(currentNode, nodeString[currentNode]);
    }

    static void printStats(const std::string& nextPhaseStr);

    static bool reqGustoToggle() {
      gustoReq = !gustoReq;
      return gustoReq;
    }

    static void reqGusto(bool req) {
      gustoReq = req;
    }

    static void updateGusto() {
      gusto = gustoReq && useGusto;
    }

    static void clearGusto() {
      gusto = false;
      gustoReq = false;
    }

    static bool getGusto() {
      return gusto;
    }
    
    static bool getGustoReq() {
      return gustoReq;
    }
    
    static void markUniqueState() {
      uniqueStateCount++;
    }

    static char const * getCauseStr(PathEndCause cause) {
        switch(cause) {
          case NO_MORE_EVENTS: return "NO_MORE_EVENTS";
          case DUPLICATE_STATE: return "DUPLICATE_STATE";
          case STOPPING_CONDITION: return "STOPPING_CONDITION";
          case TOO_MANY_STEPS: return "TOO_MANY_STEPS";
          default: ASSERT(0);
        }
    }
    static void pathComplete(PathEndCause cause, bool isLive, bool isSafe,
                             uint32_t simSteps, uint32_t randSteps,
                             const mace::Printable* const randomUtil,
                             const mace::string& description);

    static unsigned getPathNum() { return cumulativePathCount; }

    virtual bool hasEventsWaiting() const = 0;
    virtual std::string simulateEvent(const Event& e) = 0;
    virtual uint32_t hashState() const = 0;
    virtual ~SimCommon() {}
};

#endif // SIM_COMMON_H
