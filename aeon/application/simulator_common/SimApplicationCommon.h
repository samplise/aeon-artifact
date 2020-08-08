/* 
 * SimApplicationCommon.h : part of the Mace toolkit for building distributed systems
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
#ifndef SIM_APP_COMMON_H
#define SIM_APP_COMMON_H

#include "MaceTypes.h"
#include "ScopedStackExecution.h"
#include "SimApplicationServiceClass.h"
#include "SimCommon.h"
#include "SimEvent.h"
#include "SimHandler.h"
#include "mace-macros.h"
#include "mvector.h"

class SimApplicationCommon : public SimCommon {
protected:
  class SimNodeHandlerCommon : public SimHandler, public PrintPrintable {
  protected:
    int nodeNum;
    int incarnationNumber;
    SimApplicationServiceClass* node;
    bool eventSimulated;
    bool pending;

  public:
    SimNodeHandlerCommon(int nodenum, SimApplicationServiceClass* nodeapp)
      : nodeNum(nodenum), incarnationNumber(0), node(nodeapp),
        eventSimulated(false), pending(false) {
      node->registerUniqueHandler(*this);
    }

    const MaceKey& getMaceKey(int n, registration_uid_t rid = -1) const {
      return SimCommon::getMaceKey(n);
    }
    int getNodeNumber(const MaceKey& node, registration_uid_t rid = -1) const {
      return SimCommon::getNode(node);
    }
    int getNodeNumber(registration_uid_t rid = -1) const {
      return nodeNum;
    }
    int getNodeCount(registration_uid_t rid = -1) const {
      return SimCommon::getNumNodes();
    }
    int getIncarnationNumber(registration_uid_t rid = -1) const {
      return incarnationNumber;
    }

    /// Add maceInit event to the queue
    virtual void addInitEvent() = 0;
    /// Add a failure event to the queue which triggers maceReset
    virtual void addFailureEvent() = 0;
    /// Used in the modelchecker to limit the number of failures
    virtual void checkAndClearFailureEvents() = 0;

    bool hasEventsWaiting() const {
      return !eventSimulated;
    }

    std::string simulateEvent(const Event& e) { // either a maceInit or a node reset
      ADD_SELECTORS("SimApplication::simulateEvent");
      switch(e.simulatorVector[0]) {
        case SimApplicationCommon::RESET:
          ASSERT(eventSimulated); // no failures unless there was a maceInit
          ASSERT(!SimCommon::gusto);
          maceout << "simulating failure of " << e.node << Log::endl;
          failureCount++;
          
          this->reincarnate();

          // Keep a tab on total number of failures
          checkAndClearFailureEvents();
          
          return "FAILURE";
        
        case SimApplicationCommon::APP:
        { // can only be a maceInit, only once
          ASSERT(!eventSimulated);
          
          maceout << "Calling maceInit() on " << e.node << Log::endl;
          node->maceInit(); // Hard-coded maceInit on the node
          // Allows us to simulate node bootstrapping in the search too
          
          eventSimulated = true;
          addFailureEvent();
          return "maceInit()";
        }
        
        default:
          ASSERT(e.simulatorVector[0] == SimApplicationCommon::RESET || e.simulatorVector[0] == SimApplicationCommon::APP);
      }
      ABORT("Not Possible!");
    }

    void initNode() {
      ADD_SELECTORS("SimApplication::initNode");
      ASSERT(!eventSimulated);
      // Selectively pre-initialize nodes
      // default (empty): initialize NONE
      // value < 0: initialize ALL
      // Specify list of nodes (0-based)
      typedef mace::deque<int> IntList;
      static const IntList initNodes = params::getList<int>("NODES_TO_PREINITIALIZE");
      if ((initNodes.size() && initNodes.front() < 0)
          || (std::find(initNodes.begin(), initNodes.end(), nodeNum) != initNodes.end())) {
        // Pre-initialize this node
        maceout << "Pre-Initializing node: " << nodeNum << Log::endl;
        node->maceInit();
        eventSimulated = true;
        addFailureEvent();
      } else {
        // Queue a maceInit for simulation
        addInitEvent();
      }
    }

    void reincarnate() {
      eventSimulated = false;
      node->maceReset();
      node->registerUniqueHandler(*this);
      incarnationNumber++;

      // Schedule a maceInit on the node
      addInitEvent();
    }
    uint32_t hashState() const {
      return nodeNum * incarnationNumber + incarnationNumber;
    }
    void reset() {
      reincarnate();
      incarnationNumber = 0;
    }
    void print(std::ostream& out) const {
      out << "[nodeHandler " << nodeNum << " incarnation " << incarnationNumber << "]";
    }
  };

protected:
  //   typedef mace::vector<ServiceClass*, mace::SoftState> ServiceClassVector;
  //   typedef mace::vector<ServiceClassVector, mace::SoftState> ServicePrintVector;
  typedef ServiceClass::ConstServiceList ServiceClassVector;
  typedef ServiceClass::ServiceListMap ServicePrintVector;

  SimApplicationCommon(SimApplicationServiceClass** tnodes) : SimCommon(), nodes(tnodes), nodeHandlers(new SimNodeHandlerCommon*[SimCommon::getNumNodes()]), toPrint(ServiceClass::getServiceListMap()) { }
  
  SimApplicationServiceClass** nodes;
  SimNodeHandlerCommon** nodeHandlers;

  const ServicePrintVector& toPrint;
  static int failureCount;

public:
  uint32_t hashState() const {
    uint32_t hash = 0;
    for(int i = 0; i < SimCommon::getNumNodes(); i++) {
      hash = (hash << 1) ^ nodes[i]->hashState();
    }
    return hash;
  }
  
  void printNode(mace::PrintNode& printer, const std::string& name) const {
    for (int i = 0; i < SimCommon::getNumNodes(); i++) {
      mace::PrintNode pr("node " + StrUtil::toString(i), "SimApplication");
      size_t sn = 0;
      const ServiceClassVector& toPr = toPrint.get(i);
      for (ServiceClassVector::const_iterator ii = toPr.begin(); ii != toPr.end(); ii++) {
        (*ii)->printNode(pr, (*ii)->getLogType());
        sn++;
      }
      printer.addChild(pr);
    }
  }
  void print(std::ostream& out) const {
     int i = SimCommon::getCurrentNode();
     out << " [ Node " << i << ": ";
     const ServiceClassVector& toPr = toPrint.get(i);
     for (ServiceClassVector::const_iterator ii = toPr.begin(); ii != toPr.end(); ii++) {
       out << " [SERVICE[ " << *(*ii) << " ]SERVICE] ";
     }
     out << " ] ";
  }
  void printState(std::ostream& out) const {
    for(int i = 0; i < SimCommon::getNumNodes(); i++) {
      out << " [ Node " << i << ": ";
      const ServiceClassVector& toPr = toPrint.get(i);
      for (ServiceClassVector::const_iterator ii = toPr.begin(); ii != toPr.end(); ii++) {
        out << " [SERVICE[ ";
        (*ii)->printState(out);
        out << " ]SERVICE] ";
      }
      out << " ] ";
    }
  }
  enum AppEventType { APP, RESET };
  
  bool hasEventsWaiting() const {
    for (int i = 0; i < SimCommon::getNumNodes(); ++i) {
      if (nodeHandlers[i]->hasEventsWaiting()) {
        return true;
      }
    }
    return false;
  }

  std::string simulateEvent(const Event& e) {
    ADD_SELECTORS("SimApplication::simulateEvent");
    ASSERT(e.node < SimCommon::getNumNodes());
    ASSERT(nodes != NULL);
    return nodeHandlers[e.node]->simulateEvent(e);
  }

  uint32_t hashNode(int node) const {
    ASSERT(node < SimCommon::getNumNodes());
    ASSERT(nodes != NULL);
    return nodes[node]->hashState()^nodeHandlers[node]->hashState();
  }
  std::string toString(int node) const {
    ASSERT(node < SimCommon::getNumNodes());
    ASSERT(nodes != NULL);
    std::ostringstream out;
    const ServiceClassVector& toPr = toPrint.get(node);
    for (ServiceClassVector::const_iterator ii = toPr.begin(); ii != toPr.end(); ii++) {
      out << " [SERVICE[ " << **ii << " ]SERVICE] ";
    }
    out << std::endl << nodeHandlers[node]->toString();
    return out.str();
  }
  std::string toStateString(int node) const {
    ASSERT(node < SimCommon::getNumNodes());
    ASSERT(nodes != NULL);
    std::ostringstream out;
    const ServiceClassVector& toPr = toPrint.get(node);
    for (ServiceClassVector::const_iterator ii = toPr.begin(); ii != toPr.end(); ii++) {
      out << " [SERVICE[ " << (*ii)->toStateString() << " ]SERVICE] ";
    }
    out << std::endl << nodeHandlers[node]->toStateString();
    return out.str();
  }
  
  virtual ~SimApplicationCommon() {
  }
};

#endif // SIM_APP_COMMON_H
