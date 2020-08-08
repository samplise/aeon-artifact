/* 
 * BrokenTreeTest.cc : part of the Mace toolkit for building distributed systems
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
#include "LoadTest.h"
#include "Log.h"
#include "Sim.h"
#include "Simulator.h"
#include "mace-macros.h"
#include "ServiceConfig.h"

namespace macesim {

#ifdef UseBrokenTree
  class BrokenTreeMCTest : public MCTest {
    public:
      const mace::string& getTestString() {
        const static mace::string s("BrokenTree");
        return s;
      }

      void loadTest(SimApplicationServiceClass** appNodes, int num_nodes) {
        ADD_SELECTORS("BrokenTree::loadTest");
        macedbg(0) << "called." << Log::endl;
        
        params::set("ServiceConfig.BrokenTree.num_nodes", boost::lexical_cast<std::string>(num_nodes)); 
        params::set("root", "1.0.0.0"); 
        params::set("ServiceConfig.SimTreeApp.tree_", "BrokenTree"); 
        params::set("ServiceConfig.SimTreeApp.PEERSET_STYLE", "0"); 
        params::set("NODES_TO_PREINITIALIZE", "-1");

        for (int i = 0; i < num_nodes; i++) {
          Sim::setCurrentNode(i);
          appNodes[i] = &(mace::ServiceFactory<SimApplicationServiceClass>::create("SimTreeApp", false));

        }
      }

      virtual ~BrokenTreeMCTest() {}
  };

  void addBrokenTree() __attribute__((constructor));
  void addBrokenTree() {
    MCTest::addTest(new BrokenTreeMCTest());
  }
#endif
}
