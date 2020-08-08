/* 
 * ServiceTests.h : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Charles Killian, James W. Anderson, Karthik Nagaraj
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

#ifdef UseRandTree
  class RandTreeMCTest : public MCTest {
    public:
      const mace::string& getTestString() {
        const static mace::string s("RandTree");
        return s;
      }

      void loadTest(SimApplicationServiceClass** appNodes, int num_nodes) {
        ADD_SELECTORS("RandTree::loadTest");
        macedbg(0) << "called." << Log::endl;
        
        params::set("ServiceConfig.SimTreeApp.tree_", "RandTree");
        params::set("ServiceConfig.SimTreeApp.PEERSET_STYLE", "1");

        for (int i = 0; i < num_nodes; i++) {
          Sim::setCurrentNode(i);
          appNodes[i] = &(mace::ServiceFactory<SimApplicationServiceClass>::create("SimTreeApp", false));
        }
      }

      virtual ~RandTreeMCTest() {}
  };

  void addRandTree() __attribute__((constructor));
  void addRandTree() {
    MCTest::addTest(new RandTreeMCTest());
  }
#endif


#ifdef UseRandTree_v2
  class RandTree_v2MCTest : public MCTest {
    public:
      const mace::string& getTestString() {
        const static mace::string s("RandTree_v2");
        return s;
      }

      void loadTest(SimApplicationServiceClass** appNodes, int num_nodes) {
        ADD_SELECTORS("RandTree_v2::loadTest");
        macedbg(0) << "called." << Log::endl;
        
        params::set("ServiceConfig.SimTreeApp.tree_", "RandTree_v2");
        params::set("ServiceConfig.SimTreeApp.PEERSET_STYLE", "1");
        
        for (int i = 0; i < num_nodes; i++) {
          Sim::setCurrentNode(i);
          appNodes[i] = &(mace::ServiceFactory<SimApplicationServiceClass>::create("SimTreeApp", false));
          
        }
      }

      virtual ~RandTree_v2MCTest() {}
  };

  void addRandTree_v2() __attribute__((constructor));
  void addRandTree_v2() {
    MCTest::addTest(new RandTree_v2MCTest());
  }
#endif


// This is available as part of BrokenTreeTest.cc in the model checker docs.  Removing from here as it may cause confusion.
// #ifdef UseBrokenTree
//   class BrokenTreeMCTest : public MCTest {
//     public:
//       const mace::string& getTestString() {
//         const static mace::string s("BrokenTree");
//         return s;
//       }
// 
//       void loadTest(SimApplicationServiceClass** appNodes, int num_nodes) {
//         ADD_SELECTORS("BrokenTree::loadTest");
//         macedbg(0) << "called." << Log::endl;
//         
//         params::set("ServiceConfig.BrokenTree.num_nodes", boost::lexical_cast<std::string>(num_nodes)); 
//         params::set("root", "1.0.0.0"); 
//         params::set("ServiceConfig.SimTreeApp.tree_", "BrokenTree"); 
//         params::set("ServiceConfig.SimTreeApp.PEERSET_STYLE", "0"); 
//         params::set("NODES_TO_PREINITIALIZE", "-1");
// 
//         for (int i = 0; i < num_nodes; i++) {
//           Sim::setCurrentNode(i);
//           appNodes[i] = &(mace::ServiceFactory<SimApplicationServiceClass>::create("SimTreeApp", false));
// 
//         }
//       }
// 
//       virtual ~BrokenTreeMCTest() {}
//   };
// 
//   void addBrokenTree() __attribute__((constructor));
//   void addBrokenTree() {
//     MCTest::addTest(new BrokenTreeMCTest());
//   }
// #endif


#ifdef UseOvercast  
  //class OvercastMCTest : public MCTest {
  //  public:
  //    const mace::string& getTestString() {
  //      const static mace::string s("Overcast");
  //      return s;
  //    }

  //    void loadTest(SimApplicationServiceClass** appNodes, int num_nodes) {
  //      ADD_SELECTORS("Overcast::loadTest");
  //      macedbg(0) << "called." << Log::endl;

  //      Overcast_namespace::_NodeMap_ overcastNodes;
  //      SimTreeApp_namespace::_NodeMap_ simTreeNodes;
  //      
  //      for (int i = 0; i < num_nodes; i++) {
  //        MaceKey key = Sim::getCurrentMaceKey();
  //        
  //        SimulatorTCPService* tcp = new SimulatorTCPService();
  //        
  //        Overcast_namespace::OvercastService* tree = new Overcast_namespace::OvercastService(*tcp);
  //        overcastNodes[i] = tree;
  //        
  //        SimTreeApp_namespace::SimTreeAppService* app = new SimTreeApp_namespace::SimTreeAppService(*tree, *tree, SimTreeApp_namespace::ROOT_ONLY, 1, key, num_nodes);
  //        simTreeNodes[i] = app;
  //        appNodes[i] = app;
  //      }

  //      //         TEST_PROPERTIES(Overcast, overcastNodes);
  //      //         TEST_PROPERTIES(SimTreeApp, simTreeNodes);
  //    }

  //    virtual ~OvercastMCTest() {}
  //};

  //void addOvercast() __attribute__((constructor));
  //void addOvercast() {
  //  MCTest::addTest(new OvercastMCTest());
  //}
#endif


#ifdef UsePastry
  class PastryMCTest : public MCTest {
    public:
      const mace::string& getTestString() {
        const static mace::string s("Pastry");
        return s;
      }

      void loadTest(SimApplicationServiceClass** appNodes, int num_nodes) {
        
        ADD_SELECTORS("Pastry::loadTest");
        macedbg(0) << "called." << Log::endl;
        
        params::set("ServiceConfig.SimOverlayRouterApp.ov_", "Pastry"); 
        params::set("ServiceConfig.SimOverlayRouterApp.PEERSET_STYLE", "0");
        
        for (int i = 0; i < num_nodes; i++) {
          Sim::setCurrentNode(i);
          appNodes[i] = &(mace::ServiceFactory<SimApplicationServiceClass>::create("SimOverlayRouterApp", false));
        }
      }

      virtual ~PastryMCTest() {}
  };

  void addPastry() __attribute__((constructor));
  void addPastry() {
    MCTest::addTest(new PastryMCTest());
  }
#endif

#ifdef UseScribeMS
  class ScribeMSMCTest : public MCTest {
    public:
      const mace::string& getTestString() {
        const static mace::string s("ScribeMS");
        return s;
      }

      void loadTest(SimApplicationServiceClass** appNodes, int num_nodes) {
        static const std::string dhtService = params::get<std::string>("DHT_SERVICE", "Pastry");
        
        ADD_SELECTORS("ScribeMS::loadTest");
        macedbg(0) << "called." << Log::endl;
        
        params::set("ServiceConfig.Overlay", dhtService);
        params::set("ServiceConfig.OverlayRouter", dhtService);
        params::set("ServiceConfig.SimGTreeApp.group_", "ScribeMS"); 
        params::set("ServiceConfig.SimGTreeApp.tree_", "ScribeMS"); 
        params::set("ServiceConfig.SimGTreeApp.PEERSET_STYLE", "0");

        for (int i = 0; i < num_nodes; i++) {
          Sim::setCurrentNode(i);

          appNodes[i] = &(mace::ServiceFactory<SimApplicationServiceClass>::create("SimGTreeApp", false));
        }

      }

      virtual ~ScribeMSMCTest() {}
  };

  void addScribeMS() __attribute__((constructor));
  void addScribeMS() {
    MCTest::addTest(new ScribeMSMCTest());
  }
#endif


#ifdef UseChord
  class ChordMCTest : public MCTest {
    public:
      const mace::string& getTestString() {
        const static mace::string s("Chord");
        return s;
      }

      void loadTest(SimApplicationServiceClass** appNodes, int num_nodes) {
        ADD_SELECTORS("Chord::loadTest");
        macedbg(0) << "called." << Log::endl;

        params::set("ServiceConfig.SimOverlayRouterApp.ov_", "Chord");
        params::set("ServiceConfig.SimOverlayRouterApp.PEERSET_STYLE", "2");
        
        for (int i = 0; i < num_nodes; i++) {
          Sim::setCurrentNode(i);
          appNodes[i] = &(mace::ServiceFactory<SimApplicationServiceClass>::create("SimOverlayRouterApp", false));
        }
      }

      virtual ~ChordMCTest() {}
  };

  void addChord() __attribute__((constructor));
  void addChord() {
    MCTest::addTest(new ChordMCTest());
  }
#endif


// #ifdef UsePaxos
//   class PaxosMCTest : public MCTest {
//     public:
//       const mace::string& getTestString() {
//         const static mace::string s("Paxos");
//         return s;
//       }
//       void loadTest(ServiceList& servicesToDelete, NodeServiceClassList& servicesToPrint, SimApplicationServiceClass** appNodes, int num_nodes) {
//         int base_port = params::get("MACE_PORT", 5377);
//         //         int queue_size = params::get("queue_size", 20);
//         mace::map<int, SimConsensusApp_namespace::SimConsensusAppService*, mace::SoftState> simAppNodes;
//         for(int i = 0; i < num_nodes; i++) {
//           ServiceClassList list;
//           Sim::setCurrentNode(i);
//           MaceKey key = Sim::getCurrentMaceKey();
//           SimulatorUDPService* udp = new SimulatorUDPService(base_port, i);
//           PaxosMembership_namespace::PaxosMembershipService* membership = new PaxosMembership_namespace::PaxosMembershipService(*udp);
//           membership->maceInit();

//           NodeSet allNodes;
//           for (int j = 0; j < num_nodes; j++) {
//             allNodes.insert(Sim::getMaceKey(j));
//           }

//           membership->setMembership(MaceKey::null, allNodes, -1);
//           servicesToDelete.push_back(membership);
//           list.push_back(membership);
//           Paxos_namespace::PaxosService* ch = new Paxos_namespace::PaxosService(*membership,*membership,*udp);
//           servicesToDelete.push_back(ch);
//           list.push_back(ch);
//           static int numProposers = params::get("NODES_TO_PROPOSE", allNodes.size());
//           SimConsensusApp_namespace::SimConsensusAppService* app = new SimConsensusApp_namespace::SimConsensusAppService(*ch, numProposers);
//           app->maceInit();
//           servicesToDelete.push_back(app);
//           list.push_back(app);
//           servicesToPrint.push_back(list);
//           simAppNodes[i] = app;
//           appNodes[i] = app;
//         }
//         propertiesToTest.push_back(new SpecificTestProperties<SimConsensusApp_namespace::SimConsensusAppService>(simAppNodes));
//         propertiesToTest.push_back(new SimEmptyProperty(SimNetwork::Instance()));
//       }
//       virtual ~PaxosMCTest() {}
//   };

//   void addPaxos() __attribute__((constructor));
//   void addPaxos() {
//     MCTest::addTest(new PaxosMCTest());
//   }
// #endif


#ifdef UseMaceTransport
  class MaceTransportMCTest : public MCTest {
    public:
      const mace::string& getTestString() {
        const static mace::string s("MaceTransport");
        return s;
      }

      void loadTest(SimApplicationServiceClass** appNodes, int num_nodes) {
        ADD_SELECTORS("MaceTransport::loadTest");
        macedbg(0) << "called." << Log::endl;
        
        params::set("ServiceConfig.MaceTransport.UPCALL_MESSAGE_ERROR", "1");
        params::set("ServiceConfig.MaceTransport.maxQueueSize", "1000");
        params::set("ServiceConfig.SimTransportApp.router", "MaceTransport");

        for (int i = 0; i < num_nodes; i++) {
          Sim::setCurrentNode(i);
          appNodes[i] = &(mace::ServiceFactory<SimApplicationServiceClass>::create("SimTransportApp", false));
          
        }

        TestProperties::addTest(new SimulatorEmptyProperty(SimNetwork::Instance()));
        TestProperties::addTest(new SimulatorEmptyProperty(SimScheduler::Instance()));
      }

      virtual ~MaceTransportMCTest() {}
  };

  void addMaceTransport() __attribute__((constructor));
  void addMaceTransport() {
    MCTest::addTest(new MaceTransportMCTest());
  }
#endif


#ifdef UseProvisionalTransport
  class ProvisionalTransportMCTest : public MCTest {
    public:
      const mace::string& getTestString() {
        const static mace::string s("ProvisionalTransport");
        return s;
      }
      void loadTest(SimApplicationServiceClass** appNodes, int num_nodes) {
        ADD_SELECTORS("ProvisionalTransport::loadTest");
        macedbg(0) << "called." << Log::endl;
        
        params::set("ServiceConfig.ProvisionalTransport.maxQueueSize", "1"); 
        params::set("ServiceConfig.ProvisionalTransport.queueThresholdArg", "1");

        for (int i = 0; i < num_nodes; i++) {
          Sim::setCurrentNode(i);
          appNodes[i] = &(mace::ServiceFactory<SimApplicationServiceClass>::create("SimPTransportApp", false));
          
        }

        TestProperties::addTest(new SimulatorEmptyProperty(SimNetwork::Instance()));
        //       TestProperties::addTest(new SimulatorEmptyProperty(SimScheduler::Instance()));
      }

      virtual ~ProvisionalTransportMCTest() {}
  };

  void addProvisionalTransport() __attribute__((constructor));
  void addProvisionalTransport() {
    MCTest::addTest(new ProvisionalTransportMCTest());
  }
#endif


#ifdef UseRanSubAggregator
  class RanSubAggregatorMCTest : public MCTest {
    public:
      const mace::string& getTestString() {
        const static mace::string s("RanSubAggregator");
        return s;
      }

      void loadTest(SimApplicationServiceClass** appNodes, int num_nodes) {
        ADD_SELECTORS("RanSubAggregator::loadTest");
        macedbg(0) << "called." << Log::endl;
      
        params::set("root", "1.0.0.0");
        params::set("ServiceConfig.SimAggregateApp.PEERSET_STYLE", "0");
        params::set("ServiceConfig.Overlay", "ReplayTree");
        params::set("ServiceConfig.Tree", "ReplayTree");
        
        for (int i = 0; i < num_nodes; i++) {
          Sim::setCurrentNode(i);
          
          appNodes[i] = &(mace::ServiceFactory<SimApplicationServiceClass>::create("SimAggregateApp", false));

        }

      }

      virtual ~RanSubAggregatorMCTest() {}
  };

  void addRanSubAggregator() __attribute__((constructor));
  void addRanSubAggregator() {
    MCTest::addTest(new RanSubAggregatorMCTest());
  }
#endif
  class SimulatorTCPMCTest : public MCTest {
    public:
      const mace::string& getTestString() {
        const static mace::string s("SimulatorTCP");
        return s;
      }

      void loadTest(SimApplicationServiceClass** appNodes, int num_nodes) {
        ADD_SELECTORS("SimulatorTCP::loadTest");
        macedbg(0) << "called." << Log::endl;
        
        params::set("ServiceConfig.SimTransportApp.router", "SimulatorTCP");
        
        for (int i = 0; i < num_nodes; i++) {
          Sim::setCurrentNode(i);

          appNodes[i] = &(mace::ServiceFactory<SimApplicationServiceClass>::create("SimTransportApp", false));
          
        }
        
        TestProperties::addTest(new SimulatorEmptyProperty(SimNetwork::Instance()));
        TestProperties::addTest(new SimulatorEmptyProperty(SimScheduler::Instance()));
      }
      
      virtual ~SimulatorTCPMCTest() {}
  };

  void addSimulatorTCP() __attribute__((constructor));
  void addSimulatorTCP() {
    MCTest::addTest(new SimulatorTCPMCTest());
  }
}
