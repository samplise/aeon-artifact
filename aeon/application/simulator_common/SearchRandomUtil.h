/* 
 * SearchRandomUtil.h : part of the Mace toolkit for building distributed systems
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
#ifndef SEARCH_RANDOM_UTIL_H
#define SEARCH_RANDOM_UTIL_H

#include <fstream>
#include <sstream>

#include "Printable.h"
#include "SimRandomUtil.h"
#include "mvector.h"
#include "params.h"

class SearchRandomUtil : public SimRandomUtil {
  protected:
    const unsigned NUM_RANDOM_SAMPLES;
    UIntList currentSelections;
    UIntList currentOutOf;
    UIntList repeatMaxTest;

    unsigned depth;
    int depthIncrement;
    bool doneEarly; //True when every path searched at this depth so far finished before the depth.
    bool verbose;

    SearchRandomUtil(bool v = false) : 
      SimRandomUtil(1000),
      NUM_RANDOM_SAMPLES(params::get("NUM_RANDOM_SAMPLES", 10)),
      currentSelections(), currentOutOf(), repeatMaxTest(), depth(0),
      depthIncrement(params::containsKey("SEARCH_DEPTH")?params::get<int>("SEARCH_DEPTH"):5),
      doneEarly(true),
      verbose(v)
    {
      UIntList i;
      i.push_back(0);
      setNext(i);
    }


  public:
    unsigned randIntImplSub(unsigned max) {
      ADD_SELECTORS("SearchRandomUtil::randInt");
      ASSERT(max > 0);
      macedbg(0) << " depth=" << depth << " currentSelections.size()=" << currentSelections.size() << " currentOutOf.size()=" << currentOutOf.size() << Log::endl;
      ASSERT(depth != 0);
      unsigned offset = _pathInts.size();
      if (offset < depth) {
        if (offset >= currentOutOf.size()) {
          currentOutOf.push_back(max);
          if (offset >= currentSelections.size()) {
            if (max > MAX_RAND_INT) {
              currentSelections.push_back(RandomUtil::randIntImpl(max));
            } else {
              currentSelections.push_back(0);
            }
            repeatMaxTest.push_back(0);
          }
        }
        macedbg(0) << "currentSelections[offset]=" << currentSelections[offset] << " currentOutOf[offset]="<<currentOutOf[offset] << " offset " << offset << Log::endl;
        ASSERT(currentSelections[offset] < max);
        if (max != currentOutOf[offset]) {
          maceerr << "max: " << max << " currentOutOf[ " << offset << " ]: " << currentOutOf[offset] << Log::endl;
          ASSERT(max == currentOutOf[offset]);
        }
        if (_pathInts.size() == depth - 1) { 
          doneEarly = false; 
        }
        return currentSelections[offset];
      }
      doneEarly = false;
      return max;
    }
    void setNext(const UIntList& selections) {
      currentSelections = selections;
      repeatMaxTest.resize(currentSelections.size());
      while(depth < currentSelections.size()) { depth += depthIncrement; }
      currentOutOf.resize(0);
    }
    bool hasNextSub(bool isLive) { 
      static unsigned maxSearchDepth = params::get("MAX_SEARCH_DEPTH", UINT_MAX);
      for (uint i = 0; i < depth && i < _pathInts.size(); i++) {
        if (currentOutOf[i] > MAX_RAND_INT) {
          if (repeatMaxTest[i] < NUM_RANDOM_SAMPLES) {
            return true;
          }
        } else if (currentSelections[i] + 1 < currentOutOf[i]) { return true; }
      }
      if (doneEarly) {
        return false;
      } else {
        return depth < maxSearchDepth;
      }
    }
    bool nextSub() { //advance to the next search sequence, returns true when the depth increases
      ADD_SELECTORS("SearchRandomUtil::next");
      bool ret = false;
      if (currentOutOf.size() != 0) { 
        int newDepth = std::min((size_t)depth, _pathInts.size()); 
        for (; newDepth > 0; newDepth--) {
          if (currentOutOf[newDepth-1] > MAX_RAND_INT) {
            currentSelections[newDepth-1] = RandomUtil::randIntImpl(currentOutOf[newDepth-1]);
            repeatMaxTest[newDepth-1]++;
            if (repeatMaxTest[newDepth-1] < NUM_RANDOM_SAMPLES) { break; }
          } else {
            currentSelections[newDepth-1]++;
            if (currentSelections[newDepth-1] < currentOutOf[newDepth-1]) { break; }
          }
        }
        currentSelections.resize(newDepth);
        currentOutOf.resize(newDepth);
        repeatMaxTest.resize(newDepth);
        if (newDepth == 0) {
          ASSERT(!doneEarly /*failed assertion means search is done!*/);
          doneEarly = true;
          depth += depthIncrement;
          currentSelections.reserve(depth);
          currentOutOf.reserve(depth);
          repeatMaxTest.reserve(depth);
          maceout << "Searching up to depths of " << depth << ", " << (Sim::getPathNum()-1) << " paths searched so far." << Log::endl;
          phase = "SearchRandom:depth "+boost::lexical_cast<std::string>(depth);
          ret = true;
        }
      }
      resetGustoToggles();
      addGustoToggle(depth+_prefix.size());
      return ret;
    }
    void print(std::ostream& out) const {
      out << "Path<sdepth=" << depth << ">";
      SimRandomUtil::print(out);
      out << " search sequence " << currentSelections;
      if (verbose) {
        out << " Out Of " << currentOutOf;
      }
    }
    bool implementsPrintPath() { return true; }
    bool pathIsDone() { return _pathInts.size() == depth;}
    void dumpState() {}
    int testSearchDepth() { 
      if (_pathInts.size() >= currentOutOf.size()) {
        if (_randomInts.empty()) {
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

    static SearchRandomUtil* SetInstance() { 
      ADD_SELECTORS("SearchRandomUtil::SetInstance");
      SearchRandomUtil* t = NULL; 
      if (_inst == NULL) { 
        _inst = t = new SearchRandomUtil(); 
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
        //if (params::containsKey("RANDOM_UTIL_START_SEARCH")) {
        //  std::string s = params::get<std::string>("RANDOM_UTIL_START_SEARCH");
        //  std::istringstream in(s);
        //  UIntList initial;
        //  uint a;
        //  in >> a;
        //  while(!in.eof()) {
        //    initial.push_back(a);
        //    in >> a;
        //  }
        //  t->setNext(initial);
        //}
      }
      return t;
    }
};

#endif // SEARCH_RANDOM_UTIL_H
