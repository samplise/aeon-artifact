/* 
 * ReplayRandomUtil.h : part of the Mace toolkit for building distributed systems
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
#ifndef REPLAY_RANDOM_UTIL_H
#define REPLAY_RANDOM_UTIL_H

#include <fstream>
#include <iostream>

#include "SimRandomUtil.h"
#include "params.h"

class ReplayRandomUtil : public SimRandomUtil {
  protected:
    static ReplayRandomUtil* realRandomUtil;
    bool interactive;
    std::istream* infile;

    //Used in interactive mode only.
    mace::vector<unsigned> simStepToPathStep;
    unsigned lastStepNum;
    bool runUntilLiveness;
    bool stopRunUntilLiveness;
    bool doPrompt;
    bool postRunGusto;

  public:
    unsigned randIntImplSub(unsigned foo) {
      ADD_SELECTORS("ReplayRandomUtil::randInt");
      static const bool ALLOW_RAND_MAX = params::get("ALLOW_RAND_MAX", false);
      if (runUntilLiveness && (stopRunUntilLiveness || Sim::isLive || Sim::step >= Sim::maxStep)) {
        runUntilLiveness = false;
        stopRunUntilLiveness = false;
        requestGustoSet(postRunGusto);
      }
      while (Sim::step > lastStepNum) {
        simStepToPathStep.push_back(pathDepth());
        lastStepNum++;
      }
      unsigned b;
      do {
        if (interactive && !runUntilLiveness && doPrompt) {
          if (foo <= 1) { return 0; }
          maceout << "Random number requested between 0 and " << foo << " (exclusive). i.e. [0," << foo << ")" << Log::endl;
        }
        else {
          maceout << "max " << foo << " currentPos " << pathDepth() << Log::endl;
          if (foo <= 1) { 
            maceout << "returning deterministic 0" << Log::endl;
            return 0; 
          }
        }
        if (!runUntilLiveness) {
          bool skipB = false;
          if (!interactive || !doPrompt) {
            unsigned a;
            (*infile) >> a;
            ASSERTMSG(!infile->eof(), "End of replay file reached.");
            if (a >= 1000000 && !ALLOW_RAND_MAX) {
              b = a;
              skipB = true;
            } else {
              ASSERTMSG(a == foo, "Unexpected/uncontrolled non-determinism detected: see http://www.macesystems.org/wiki/?do=search&id=%22a+%3D%3D+foo%22");
            }
          }
          ASSERTMSG(!infile->eof(), "End of replay file reached.");
          if (!skipB) {
            (*infile) >> b;
            ASSERTMSG(!infile->eof(), "End of replay file reached.");
          } else {
            skipB = false;
          }
        }
        if (!ALLOW_RAND_MAX) {
          if (b == 1000000 || runUntilLiveness) {
            //Dealer's Choice
            b = RandomUtil::randIntImpl(foo);
          }
          if (b == 1000001) {
            //Toggle Gusto
            bool req = requestGustoToggle();
            maceout << "Set gusto to: " << req << " (takes effect next step)" << Log::endl;
          } else if (b == 1000002) {
            //Save sequence to file <f>
            std::string filename;
            std::cin >> filename;
            std::ofstream out(filename.c_str());
            SimRandomUtil::printPath(out);
            out.close();
            maceout << "Saved sequence as: " << filename << Log::endl;
          } else if (b == 1000003) {
            //Run until isLive
            runUntilLiveness = true;
            stopRunUntilLiveness = false;
            postRunGusto = requestGustoSet(true);
          } else if (b == 1000004) {
            //Toggle prompt
            doPrompt = !doPrompt;
            maceout << "Set doPrompt to: " << doPrompt << Log::endl;
          } else if (b == 1000005) {
            //Save sequence up to step <s> to file <f>
            unsigned stepNum;
            std::cin >> stepNum;
            std::string filename;
            std::cin >> filename;
            std::ofstream out(filename.c_str());
            printPath(out, stepNum);
            out.close();
          } else if (b == 1000006) {
            exit(0);
          } else {
            //b is kosher.
          }
        }
        else {
          break;
        }
      } while (b >= 1000000);
      ASSERT(b < foo);
      return b;
    }
    //Note: inherit randIntImpl(...) from the base class.  It will call our impl by inheritance.

    bool handleInt() {
      if (runUntilLiveness) {
        stopRunUntilLiveness = true;
        return true;
      }
      else {
        return false;
      }
    }

    static ReplayRandomUtil* SetInstance() { 
      ReplayRandomUtil* realRandomUtil = NULL;
      if (_inst == NULL) { _inst = realRandomUtil = new ReplayRandomUtil(); } 
      return realRandomUtil; 
    }

    ReplayRandomUtil() : 
      SimRandomUtil(),
      interactive((params::get<std::string>("RANDOM_REPLAY_FILE")==std::string("-")?true:false)),
      infile((interactive ? &std::cin : new std::ifstream(params::get<std::string>("RANDOM_REPLAY_FILE").c_str()))), 
      lastStepNum(0), runUntilLiveness(false), doPrompt(false), postRunGusto(false)
    { 
      ADD_FUNC_SELECTORS;
      maceout << "Using REPLAY_RANDOM_FILE \"" << params::get<std::string>("RANDOM_REPLAY_FILE") << "\"" << " and therefore " << (interactive ? "std::cin" : "new ifstream") << Log::endl;
      ASSERTMSG(infile->good(), Error reading file specified in RANDOM_REPLAY_FILE); 
      unsigned currentGusto = 0;
      if (interactive) {
        simStepToPathStep.push_back(0);
      }
      UIntList toggles;
      do {
        maceout << "Please provide gusto toggle depth (" << UINT_MAX << " when no more toggles)" << Log::endl; 
        (*infile) >> currentGusto; 
        maceout << "Gusto Toggle at: " << currentGusto << Log::endl;
        toggles.push_back(currentGusto);
        if (currentGusto == UINT_MAX) { break; }
      } while (true);
      setPrefix(toggles);
    }
    virtual ~ReplayRandomUtil() {}

    bool hasNextSub(bool isLive) { return false; }
    bool nextSub() { return false; }
    bool pathIsDone() { return false; }
    void dumpState() {
      if (interactive) {
        ADD_SELECTORS("ReplayRandomUtil::dumpState");
        std::ofstream out("final-sequence.path");
        maceout << "Saving sequence as: final-sequence.path" << Log::endl;
        SimRandomUtil::printPath(out);
        out.close();
      }
    }
    void print(std::ostream& out) const { }

    bool implementsPrintPath() { return interactive; }
    void printPath(std::ostream& out, unsigned stepNum) const {
      if (stepNum > simStepToPathStep.size()) { 
        SimRandomUtil::printPath(out);
        return;
      }
      UIntList toggles = getGustoToggles();
      for (UIntList::const_iterator i = toggles.begin(); i != toggles.end() && *i <= simStepToPathStep[stepNum]; i++) {
        out << *i << std::endl;
      }
      //if (toggleCount % 2 != 0) {
      //  out << _pathInts.size() << std::endl; //Print out the gusto depth
      //}
      out << UINT_MAX << std::endl;
      unsigned position = 0;
      for (UIntList::const_iterator i = _pathInts.begin(), j = _outOf.begin(); i != _pathInts.end() && j != _outOf.end() && position < simStepToPathStep[stepNum]; i++, j++, position++) {
        out << *j << " " << *i << std::endl;
      }
    }
};

#endif // REPLAY_RANDOM_UTIL_H
