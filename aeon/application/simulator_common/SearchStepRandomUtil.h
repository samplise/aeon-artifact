/* 
 * SearchStepRandomUtil.h : part of the Mace toolkit for building distributed systems
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
#ifndef SEARCH_STEP_RANDOM_UTIL_H
#define SEARCH_STEP_RANDOM_UTIL_H

#include <fstream>
#include <sstream>

#include "Printable.h"
#include "SimRandomUtil.h"
#include "mvector.h"
#include "params.h"

class SearchStepRandomUtil : public SimRandomUtil {
  protected:
    UIntList currentSelections;
    UIntList currentOutOf;

    bool doneEarly; //True when every path searched at this depth so far finished before the depth.
    bool verbose;
    bool prefixStepsSet;

    unsigned stepDepth;
    unsigned stepDepthIncrement;
    unsigned prefixSteps;

    SearchStepRandomUtil(bool v = false)
        : currentSelections(), currentOutOf(), verbose(v),
        prefixStepsSet(false),
        stepDepth(params::containsKey("SEARCH_DEPTH") ? params::get<int>("SEARCH_DEPTH") : 5),
        stepDepthIncrement(stepDepth), prefixSteps(UINT_MAX)
    {
      UIntList i;
      i.push_back(0);
      currentSelections = i;
      currentOutOf.resize(0);
    }


  public:
    unsigned randIntImplSub(unsigned max) {
      ADD_SELECTORS("SearchStepRandomUtil::randInt");
      ASSERT(max > 0 && max < MAX_RAND_INT);
      macedbg(0) << "stepDepth " << stepDepth << " currentSelections.size()=" << currentSelections.size() << " currentOutOf.size()=" << currentOutOf.size() << Log::endl;
      ASSERT(stepDepth != 0);
      if (!prefixStepsSet) {
        prefixSteps = Sim::step;
        prefixStepsSet = true;
      }
      if (Sim::step < stepDepth + prefixSteps) {
        uint offset = _pathInts.size();
        if (offset >= currentOutOf.size()) {
          currentOutOf.push_back(max);
          if (offset >= currentSelections.size()) {
            currentSelections.push_back(0);
          }
        }
        macedbg(0) << "currentSelections[offset]=" << currentSelections[offset] << " currentOutOf[offset]="<<currentOutOf[offset] << " offset " << offset << Log::endl;
        ASSERT(currentSelections[offset] < max);
        if (max != currentOutOf[offset]) {
          maceerr << "max: " << max << " currentOutOf[ " << offset << " ]: " << currentOutOf[offset] << Log::endl;
          ASSERT(max == currentOutOf[offset]);
        }
        if (Sim::step == stepDepth + prefixSteps - 1) {
          doneEarly = false;
          requestGustoSet(true);
        }
        return currentSelections[offset];
      }
      requestGustoSet(true);
      doneEarly = false;
      return max;
    }
    bool hasNextSub(bool isLive) { 
      static unsigned maxSearchDepth = params::get("MAX_SEARCH_DEPTH", UINT_MAX);
      for (uint i = 0; i < currentSelections.size() && i < _pathInts.size(); i++) {
        if (currentSelections[i] + 1 < currentOutOf[i]) { return true; }
      }
      if (doneEarly) {
        return false;
      } else {
        return stepDepth < maxSearchDepth;
      }
    }
    bool nextSub() { //advance to the next search sequence, returns true when the depth increases
      ADD_SELECTORS("SearchStepRandomUtil::next");
      resetGustoToggles();
      if (!currentOutOf.empty()) {
        int newDepth = std::min(currentSelections.size(), _pathInts.size()); 
        for (; newDepth > 0; newDepth--) {
          currentSelections[newDepth-1]++;
          if (currentSelections[newDepth-1] < currentOutOf[newDepth-1]) { break; }
        }
        currentSelections.resize(newDepth);
        currentOutOf.resize(newDepth);
        if (newDepth == 0) {
          ASSERT(!doneEarly /*failed assertion means search is done!*/);
          doneEarly = true;
          stepDepth += stepDepthIncrement;
          maceout << "Searching up to " << stepDepth << " steps, " << Sim::getPathNum() << " paths searched so far." << Log::endl;
          phase = "SearchStepRandom:stepDepth "+boost::lexical_cast<std::string>(stepDepth);
          return true;
        }
      }
      return false;
    }
    void print(std::ostream& out) const {
      out << "Path<stdepth=" << stepDepth << ">";
      SimRandomUtil::print(out);
      out << " search sequence " << currentSelections;
      if (verbose) {
        out << " Out Of " << currentOutOf;
      }
    }
    bool implementsPrintPath() { return true; }
    bool pathIsDone() { return (prefixSteps != UINT_MAX && Sim::step >= stepDepth + prefixSteps); }
    void dumpState() {}
    int testSearchDepth() { 
      if (pathDepth() >= _prefix.size() + currentOutOf.size()) {
        if (prefixSteps == UINT_MAX || Sim::step < stepDepth + prefixSteps) {
          return 0;
        } else {
          return 1;
        }
      } else if (pathDepth() < _prefix.size()) {
        return -2;
      } else {
        return -1;
      }
    }

    static SearchStepRandomUtil* SetInstance() { 
      ADD_SELECTORS("SearchStepRandomUtil::SetInstance");
      SearchStepRandomUtil* t = NULL; 
      if (_inst == NULL) { 
        _inst = t = new SearchStepRandomUtil(); 
        if (params::containsKey("RANDOM_UTIL_PREFIX_FILE")) {
          std::ifstream in(params::get<std::string>("RANDOM_UTIL_PREFIX_FILE").c_str());
          ASSERTMSG(in.good(), Error reading file specified in RANDOM_UTIL_PREFIX_FILE); 
          
          UIntList initial;
          uint a;
          in >> a;
          while (!in.eof()) {
            macedbg(1) << "Pushing back " << a << " onto initial." << Log::endl;
            initial.push_back(a);
            in >> a;
          }
          macedbg(0) << "initial.size: " << initial.size() << " initial: " << initial << Log::endl;
          t->setPrefix(initial);
        }
      }
      return t;
    }
};

#endif // SEARCH_STEP_RANDOM_UTIL_H
