/* 
 * LoadTest.h : part of the Mace toolkit for building distributed systems
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
#ifndef LOAD_TEST_H
#define LOAD_TEST_H

#include <sstream>

#include "ServiceClass.h"
#include "SimulatorCommon.h"
#include "m_map.h"
#include "mstring.h"
#include "mvector.h"

#define TEST_PROPERTIES4(svname,ns,sv,nodes) propertiesToTest.push_back(new SpecificTestProperties< svname##ns::svname##sv >(nodes))

#define TEST_PROPERTIES(svname,nodes) TEST_PROPERTIES4(svname, _namespace, Service, nodes)

#define TEST_TCP_PROPERTIES(nodes) propertiesToTest.push_back(new SpecificTestProperties<SimulatorTCP_namespace::SimulatorTCPCommonService>(nodes))

namespace macesim {

class MCTest {
  private:
  typedef mace::map<mace::string, MCTest*, mace::SoftState> MCTestMap;
  //   static MCTestMap testMap;
  
  static MCTestMap& testMap() {
    static MCTestMap* m = new MCTestMap();
    return *m;
  }
  public:
    virtual void loadTest(SimApplicationServiceClass** appNodes, int num_nodes) = 0;
    virtual const mace::string& getTestString() = 0;
    static MCTest* getTest(const mace::string& testString) {
      MCTestMap::iterator i = testMap().find(testString);
      if (i == testMap().end()) {
        return NULL;
      }
      else {
        return i->second;
      }
    }
    static void addTest(MCTest* testGen) {
      ASSERT(!testMap().containsKey(testGen->getTestString()));
      testMap()[testGen->getTestString()] = testGen;
    }
    static mace::string getRegisteredTests() {
      std::ostringstream out;
      for (mace::map<mace::string, MCTest*, mace::SoftState>::const_iterator i = testMap().begin(); i != testMap().end(); i++) {
        out << i->first << " ";
      }
      return out.str();
    }

    virtual ~MCTest() {}
};

}

#endif // LOAD_TEST_H
