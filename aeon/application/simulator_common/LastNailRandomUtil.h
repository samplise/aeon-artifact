/* 
 * LastNailRandomUtil.h : part of the Mace toolkit for building distributed systems
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
#ifndef LAST_NAIL_RANDOM_UTIL_H
#define LAST_NAIL_RANDOM_UTIL_H

#include "boost/lexical_cast.hpp"

#include "Printable.h"
#include "SimRandomUtil.h"
#include "mvector.h"
#include "params.h"

using boost::lexical_cast;

typedef mace::map<unsigned, UIntList> UIntListMap;
// typedef mace::vector<UIntList> IntListList;

class LastNailRandomUtil : public SimRandomUtil {
  protected:
    //These are set based on the path file input
    UIntList badPath;
    UIntList badPathOutOf;
    UIntListMap goodPaths;
    UIntListMap goodPathsOutOf;

    //These are for the current run
    unsigned prefixLength; //prefixLength is how many numbers to read from badPath before switching to random/extras.
    unsigned prefixLengthMod;
    unsigned pathCount; //tracks the number of paths tested at each prefixLength
    unsigned pathCountMax; //pronounce dead if pathCountMax paths are tested with no successes. 
    bool rampUp;

    bool verbose;
    bool printLivePaths;
    bool outputLivePaths;
    std::istream* infile;

    LastNailRandomUtil(bool v = false) : 
      SimRandomUtil(),
      prefixLength(params::get("LAST_NAIL_STARTING_PREFIX", 1)), 
      prefixLengthMod(1), 
      pathCount(0), 
      pathCountMax(params::get("NUM_RANDOM_PATHS", 20)), 
      rampUp(true), 
      verbose(v), 
      printLivePaths(params::get("LAST_NAIL_PRINT_PATHS", false)), 
      outputLivePaths(params::get("LAST_NAIL_OUTPUT_PATHS", true)), 
      infile((params::get<std::string>("LAST_NAIL_REPLAY_FILE") == std::string("-")
              ? &std::cin
              : new std::ifstream(params::get<std::string>("LAST_NAIL_REPLAY_FILE").c_str())))
    {
      ADD_FUNC_SELECTORS
      UIntList toggles;
      unsigned currentGusto = 0;
      do {
        (*infile) >> currentGusto; 
        maceout << "GustoDepth: " << currentGusto << Log::endl; 
        toggles.push_back(currentGusto);
        if (currentGusto == UINT_MAX) { break; }
      } while (true);
      setPrefix(toggles);
      unsigned input;
      (*infile) >> input;
      while (*infile) {
        badPathOutOf.push_back(input);
        (*infile) >> input;
        badPath.push_back(input);
        (*infile) >> input;
      }
      maceout << "Loaded a path of length: " << badPath.size() << Log::endl;
    }


  public:
    unsigned randIntImplSub(unsigned max) { 
      ADD_SELECTORS("LastNailRandomUtil::randInt");
      if (pathDepth() < prefixLength) {
        ASSERT(!DO_PREFIX_ASSERT || max == badPathOutOf[pathDepth()]);
        unsigned r = badPath[pathDepth()];
        maceout << "prefix: returning " << r << Log::endl;
        return r;
      }
      return max;
    }
    bool hasNextSub(bool isLive) { 
      ADD_FUNC_SELECTORS;
      //Note -- hasNext is called only at end of path
      if (isLive) {
        UIntList t;
        UIntList t2;
        for (unsigned i = 0; i < pathDepth(); i++) {
          t.push_back((i<prefixLength ? badPath[i] : _randomInts[i-prefixLength]));
          t2.push_back((i<prefixLength ? badPathOutOf[i] : _randomOutOf[i-prefixLength]));
        }
        goodPaths[prefixLength] = t;
        goodPathsOutOf[prefixLength] = t2;
        pathCount = pathCountMax; //done at this level
      }

      if (pathCount < pathCountMax) { return true; }
      pathCount = 0;

      //Adjust rampUp
      if (!isLive && rampUp) {
        rampUp = false;
        prefixLengthMod = prefixLengthMod >> 2;
      }

      //Adjust prefixLength
      if (isLive) {
        while (prefixLength + prefixLengthMod > badPath.size()) {
          prefixLengthMod = prefixLengthMod >> 1;
        } 
        prefixLength += prefixLengthMod;
      }
      else {
        prefixLength -= prefixLengthMod;
      }

      bool ret = prefixLengthMod != 0;

      //Adjust prefixLengthMod
      if (rampUp) {
        prefixLengthMod = prefixLengthMod << 1;
      }
      else {
        prefixLengthMod = prefixLengthMod >> 1;
      }

      maceout << "New prefix length: " << prefixLength << " prefixLengthMod: " << prefixLengthMod << Log::endl;
      if (prefixLength + prefixLengthMod > badPath.size()) {
        maceerr << "last nail search hit prefixLength = half of bad path. try increasing max_num_steps" << Log::endl;
      //  return false;
      }
      return ret; //note, rampUp implies prefixLengthMod > 0
    }
    bool nextSub() { //advance to the next search sequence, returns true when the depth increases
      resetGustoToggles();
      addGustoToggle(prefixLength, true);

      pathCount++;
      if (pathCount == 1) {
        phase = "LastNail:prefix "+boost::lexical_cast<std::string>(prefixLength);
      }
      return pathCount == 1;
    }
    void print(std::ostream& out) const {
      out << "Path<prefixlen=" << prefixLength << ">";
      SimRandomUtil::print(out);
    }
    bool implementsPrintPath() { return true; }
    bool pathIsDone() { return pathDepth() > prefixLength; /* in theory, this would be used to skip by earlier live portions (e.g. starting frm a live point).  For now, we just make this be whether or not we are in the random portion of the search. */ }
    void dumpState() {
      ADD_FUNC_SELECTORS;
      unsigned lastLivePrefix = 0;
      resetGustoToggles();
      UIntList toggles = getGustoToggles();
      for (UIntListMap::const_iterator i = goodPaths.begin(), i2 = goodPathsOutOf.begin(); i != goodPaths.end() && i2 != goodPathsOutOf.end(); i++, i2++) {
        lastLivePrefix = i->first;
        if (outputLivePaths) {
          std::ofstream out(("live"+lexical_cast<std::string>(i->first)+".path").c_str());
          bool gustoOn = false;
          for (UIntList::const_iterator j = toggles.begin(); j != toggles.end() && i->first > *j; j++) {
            out << *j << std::endl;
            gustoOn = !gustoOn;
          }
          if (!gustoOn) {
            out << i->first << std::endl;
          }
          out << UINT_MAX << std::endl;
          for (UIntList::const_iterator j = i->second.begin(), j2 = i2->second.begin(); j != i->second.end() && j2 != i2->second.end(); j++, j2++) {
            out << *j2 << " " << *j << std::endl;
          }
          out.close();
        }
        if (printLivePaths) {
          maceout << "Live Path<gustoDepths=" << toggles << " prefixlen=" << i->first << " randomwalk=" << i->second.size() << "> path " << i->second << Log::endl;
        } else {
          maceout << "Live Path<gustoDepths=" << toggles << " prefixlen=" << i->first << " randomwalk=" << i->second.size() << ">" << Log::endl;
        }
      }
      if (prefixLength + prefixLengthMod < badPath.size()) {
	maceout << "Last live path has prefixlen " << lastLivePrefix << " so last nail was random number position " << (lastLivePrefix+1) << " taken" << Log::endl;
      }
    }

    static LastNailRandomUtil* SetInstance() { LastNailRandomUtil* t = NULL; if (_inst == NULL) { _inst = t = new LastNailRandomUtil(); } return t;}
};

#endif // LAST_NAIL_RANDOM_UTIL_H
