/* 
 * BestFirstRandomUtil.h : part of the Mace toolkit for building distributed systems
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
#ifndef BEST_FIRST_RANDOM_UTIL_H
#define BEST_FIRST_RANDOM_UTIL_H

#include <fstream>
#include <sstream>

#include "Printable.h"
#include "SimRandomUtil.h"
#include "mvector.h"
#include "params.h"

class BestFirstRandomUtil : public SimRandomUtil {
protected:
  uint currentPos;
  uint userPrefix;
  UIntList prefix;
  UIntList currentSelections;
  UIntList currentOutOf;
  UIntList extras;

  unsigned searched;
  unsigned depth;
  int depthIncrement;
//   bool doneEarly; //True when every path searched at this depth so far
// 		  //finished before the depth.
  bool verbose;

  typedef mace::deque<UIntList> UIntListStack;
  UIntListStack stack1;
  UIntListStack stack2;
  UIntListStack* nextStack;
  UIntListStack* searchStack;

  bool firstTime;

  BestFirstRandomUtil(bool v = false) :
    currentPos(0), userPrefix(0), currentSelections(), currentOutOf(),
    searched(0), depth(0),
    depthIncrement(params::get("SEARCH_DEPTH", 5)),
    verbose(v), prefixGustoDepthToggles(), currentPrefixGustoToggle()
    nextStack(&stack1), searchStack(&stack2), firstTime(true)
  {
    UIntList i;
    i.push_back(0);
    setNext(i);
  }  // BestFirstRandomUtil


public:
  unsigned randIntImpl() { 
    ASSERT(false);
    return randIntImpl(UINT_MAX); 
  } // randIntImpl

  unsigned randIntImpl(unsigned max) {
    ADD_SELECTORS("BestFirstRandomUtil::randInt");
    ASSERT(max > 0);
    macedbg(0) << "max=" << max << " depth=" << depth
	       << " currentPos=" << currentPos
	       << " currentSelections.size()=" << currentSelections.size()
	       << " currentOutOf.size()=" << currentOutOf.size() << Log::endl;
    maceout << "max " << max << " currentPos " << currentPos << Log::endl;
    if (max <= 1) { 
      maceout << "returning deterministic 0" << Log::endl;
      return 0; 
    }
    ASSERT(depth != 0);
    if (currentPos < depth + prefix.size()) {
      if (currentPos < prefix.size()) {
	maceout << "prefix: returning " << prefix[currentPos] << Log::endl;
	if ((currentPrefixGustoToggle != prefixGustoDepthToggles.end()) &&
	   (currentPos == *currentPrefixGustoToggle)) { 
	  currentPrefixGustoToggle++;
          Sim::reqGustoToggle();
	}
	return prefix[currentPos++];
      }
      if (currentPos == prefix.size()) {
        Sim::reqGusto(false);
      }
      uint offset = currentPos - prefix.size();
      if (offset >= currentOutOf.size()) {
	currentOutOf.push_back(max);
	if (offset >= currentSelections.size()) {
	  currentSelections.push_back(0);
	}
      }
      macedbg(0) << "currentSelections[currentPos]=" << currentSelections[offset]
		 << " currentOutOf[currentPos]="<<currentOutOf[offset]
		 << " currentPos " << currentPos << " offset " << offset << Log::endl;
      maceout << "search: returning " << currentSelections[offset] << Log::endl;
      ASSERT(currentSelections[offset] < max);
      ASSERT(max == currentOutOf[offset]);
      currentPos++;
      if (currentPos >= depth + prefix.size()) {
        Sim::reqGusto(true);
// 	doneEarly = false;
      }
      return currentSelections[offset];
    }
    unsigned r = RandomUtil::randIntImpl(max);
    extras.push_back(r);
    maceout << "random: returning " << r << Log::endl;
    currentPos++;
//     doneEarly = false;
    return r; //just trying to get to the next outer step.  May be a
	      //case for failure-oblivious computing.
  } // randIntImpl

  void setPrefix(const UIntList& selections) {
    UIntList::const_iterator i = selections.begin();
    while(*i != UINT_MAX) {
      i++;
    }
    prefixGustoDepthToggles.assign(selections.begin(), i);
    currentPrefixGustoToggle = prefixGustoDepthToggles.begin();
    i++;
    prefix.assign(i, selections.end());
  } // setPrefix

  void setNext(const UIntList& selections) {
    currentSelections = selections;
    while(depth < currentSelections.size()) { depth += depthIncrement; }
    currentOutOf.resize(0);
  } // setNext

  bool hasNext(bool isLive) { 
    ADD_SELECTORS("BestFirstRandomUtil::hasNext");
    static unsigned maxSearchDepth = params::get("MAX_SEARCH_DEPTH", UINT_MAX);
    uint s = prefix.size() + depth;
    if (currentPos > s && s < maxSearchDepth) {
      assert(currentSelections.size() == depth);
      UIntList tmp = prefix;
      for (UIntList::const_iterator i = currentSelections.begin();
	   i != currentSelections.end(); i++) {
	tmp.push_back(*i);
      }
      maceout << "adding " << tmp << " to stack" << Log::endl;
      nextStack->push_back(tmp);
    }
    for (uint i = 0; i < depth && i < currentPos - prefix.size(); i++) {
      if (currentSelections[i] + 1 < currentOutOf[i]) {
	return true;
      }
    }
    return !(nextStack->empty() && searchStack->empty());
//     return !doneEarly && depth < maxSearchDepth;
  } // hasNext

  bool next() {
    //advance to the next search sequence, returns true when the depth increases
    ADD_SELECTORS("BestFirstRandomUtil::next");
    static int mask = params::get("search_print_mask", 0x00000fff);
    static uint maxPaths = params::get("MAX_PATHS", UINT_MAX);
    if (firstTime) { 
      firstTime = false;
      maceout << "returning false " << *this << Log::endl;
      return false;
    }
    //     if (currentOutOf.size() == 0) {
    //       maceout << "returning false " << *this << Log::endl;
    //       return false;
    //     }
    if ((searched & mask) == 0) {
      maceout << "Now searching path " << (searched) << " just finished " << *this
	      << Log::endl;
    }
    if (searched == maxPaths) {
      Log::log("HALT") << "Exiting due to searched (" << searched << ") == maxPaths ("
		       << maxPaths << ")" << Log::endl;
      exit(10);
    }
    searched++;
    currentPrefixGustoToggle = prefixGustoDepthToggles.begin();
    int newDepth = std::min(depth, currentPos - prefix.size()); 
    if (newDepth < 0) { newDepth = 0; }
    currentPos = 0;
    while (newDepth > 0) {
      currentSelections[newDepth-1]++;
      if (currentSelections[newDepth-1] < currentOutOf[newDepth-1]) {
	break;
      }
      newDepth--;
    }
    currentSelections.resize(newDepth);
    currentOutOf.resize(newDepth);
    extras.resize(0);
    bool ret = false;
    if (newDepth == 0) {
//       ASSERT(!doneEarly /*failed assertion means search is done!*/);
//       doneEarly = true;
//       depth += depthIncrement;
      if (searchStack->empty()) {
	maceout << "Searching up to depths of " << prefix.size() + depth << ", " << (searched-1)
		<< " paths searched so far." << Log::endl;
	phase = "SearchRandom:depth "+boost::lexical_cast<std::string>(depth);
	UIntListStack* tmp = searchStack;
	searchStack = nextStack;
	nextStack = tmp;
// 	maceout << "searchStack.size=" << searchStack->size() << Log::endl;
	ret = true;
      }
      prefix = searchStack->front();
      searchStack->pop_front();
      currentSelections.resize(0);
      currentOutOf.resize(0);
//       macedbg(1) << "starting with path " << prefix << Log::endl;
      Log::log(selectorId->log, 1) << "starting with path " << prefix << Log::endl;
      currentSelections.reserve(depth);
      currentOutOf.reserve(depth);
    }
    if ((searched & mask) == 0) {
      verbose = true;
      macedbg(0) << "Now searching " << *this << Log::endl;
      verbose = false;
    }
    return ret;
  } // next

  void print(std::ostream& out) const {
    UIntList r(prefix.begin() + userPrefix, prefix.end());
    r.insert(r.end(), currentSelections.begin(), currentSelections.end());
    out << "Path<prefixlen=" << userPrefix
	<< " randomwalk=" << extras.size()
	<< " depth=" << currentPos
	<< " sdepth=" << depth + prefix.size() - userPrefix
	<< " pnum=" << (searched)
	<< "> search sequence " << r;
    if (verbose) {
      out << " Out Of " << currentOutOf;
    }
  } // print

  bool implementsPrintPath() { return true; }

  void printPath(std::ostream& out) const {
    for (UIntList::const_iterator i = prefixGustoDepthToggles.begin();
	i != prefixGustoDepthToggles.end(); i++) {
      out << *i << std::endl;
    }
    if (prefixGustoDepthToggles.size() % 2 != 0) {
      out << prefix.size() << std::endl;
    }
    out << (depth + prefix.size()) << std::endl; //Print out the gusto depth
    out << UINT_MAX << std::endl;
    for (UIntList::const_iterator i = prefix.begin(); i != prefix.end(); i++) {
      out << *i << std::endl;
    }
      
    for (UIntList::const_iterator i = currentSelections.begin();
	i != currentSelections.end(); i++) {
      out << *i << std::endl;
    }
    for (UIntList::const_iterator i = extras.begin(); i != extras.end(); i++) {
      out << *i << std::endl;
    }
  } // printPath

  unsigned pathDepth() const {
    return currentPos;
  } // pathDepth

  bool pathIsDone() {
    return currentPos >= depth + prefix.size();
  } // pathIsDone

  void dumpState() {}

  int testSearchDepth() { 
    if (currentPos >= userPrefix + currentOutOf.size()) {
      if (currentPos <= depth + prefix.size()) {
	return 0;
      } else {
	return 1;
      }
    } else if (currentPos < userPrefix) {
      return -2;
    } else {
      return -1;
    }
  } // testSearchDepth

  UIntList getPath() const {
//     std::cerr << "userPrefix=" << userPrefix << " currentPos=" << currentPos
// 	 << " currentSelections.size=" << currentSelections.size()
// 	 << " prefix.size=" << prefix.size() << std::endl;
    ASSERT(currentPos == 0 || (userPrefix < currentPos &&
			       currentPos <= currentSelections.size() + prefix.size()));
    if (currentPos < prefix.size()) {
      return UIntList(prefix.begin() + userPrefix, prefix.begin() + currentPos);
    }
    UIntList r(prefix.begin() + userPrefix, prefix.end());
    if (currentPos > prefix.size()) {
      r.insert(r.end(), currentSelections.begin(),
	       currentSelections.begin()+(currentPos - prefix.size()));
    }
    return r;
  } // getPath

  static BestFirstRandomUtil* SetInstance() { 
    BestFirstRandomUtil* t = NULL; 
    if (_inst == NULL) { 
      _inst = t = new BestFirstRandomUtil(); 
      if (params::containsKey("RANDOM_UTIL_PREFIX_SEARCH")) {
	std::string s = params::get<std::string>("RANDOM_UTIL_PREFIX_SEARCH");
	std::istringstream in(s);
	UIntList initial;
	uint a;
        in >> a;
	while(!in.eof()) {
	  initial.push_back(a);
          in >> a;
	}
	t->setPrefix(initial);
      }
      else if (params::containsKey("RANDOM_UTIL_PREFIX_FILE")) {
	std::ifstream in(params::get<std::string>("RANDOM_UTIL_PREFIX_FILE"));
	UIntList initial;
	uint a;
        in >> a;
	while(!in.eof()) {
	  initial.push_back(a);
          in >> a;
	}
	t->setPrefix(initial);
      }
      if (params::containsKey("RANDOM_UTIL_START_SEARCH")) {
	std::string s = params::get<std::string>("RANDOM_UTIL_START_SEARCH");
	std::istringstream in(s);
	UIntList initial;
	uint a;
        in >> a;
	while(!in.eof()) {
	  initial.push_back(a);
          in >> a;
	}
	t->setNext(initial);
      }
    }
    return t;
  } // SetInstance

}; // class BestFirstRandomUtil

#endif  // BEST_FIRST_RANDOM_UTIL_H
