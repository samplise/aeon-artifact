#ifndef __SIMULATOR_BASICS
#define __SIMULATOR_BASICS

#include "m_map.h"
#include "mvector.h"
#include "MaceTypes.h"

namespace macesim {

class TestProperties;
typedef mace::vector<TestProperties*, mace::SoftState> TestPropertyList;

class SimulatorFlags {
    private:
        static bool isSimulated;
        static int  currentNode;
        static mace::string currentNodeStr;
        static int  pathNum;

    public: 
        static bool simulated() { return isSimulated; }

    public:
        /// Only call this if you are running in a simulator.  Call it only once, before other stuff..
        static void configureSimulatorFlags() {
          isSimulated = true;
        }

        static void setCurrentNode(int i) { setCurrentNode(i, boost::lexical_cast<std::string>(i)); }
        static void setCurrentNode(int i, const mace::string& str) { currentNode = i; currentNodeStr = str; }
        static int getCurrentNode() { return currentNode; }
        static const mace::string& getCurrentNodeString() { return currentNodeStr; }
        static int getPathNum() { return pathNum; }
        static void nextPath() { pathNum++; }

};

class TestProperties {
  private:
    static TestPropertyList propertiesToTest;

  public:
    virtual bool testSafetyProperties(mace::string& description) = 0;
    virtual bool testLivenessProperties(mace::string& description) = 0;
    virtual ~TestProperties() {}

  public:
    static void addTest(TestProperties* t) {
      propertiesToTest.push_back(t);
    }
    static void clearTests() {
      for (TestPropertyList::iterator i = propertiesToTest.begin(); i != propertiesToTest.end(); i++) {
        delete *i;
      }
      propertiesToTest.clear();
    }
    static const TestPropertyList& list() { return propertiesToTest; }
};

template<class Service>
class SpecificTestProperties : public TestProperties {
  typedef mace::map<int, Service const *, mace::SoftState> _NodeMap_;
  typedef mace::map<MaceKey, int, mace::SoftState> _KeyMap_;
  private:
    _NodeMap_ serviceNodes;
    _KeyMap_ keyMap;

  public:
    SpecificTestProperties(const _NodeMap_& nodes) : serviceNodes(nodes), keyMap() {
      ASSERT(nodes.size());
      int n = 0;
      for (typename _NodeMap_::const_iterator i = nodes.begin(); i != nodes.end(); i++) {
        keyMap[i->second->localAddress()] = n;
        n++;
      }
    }

    SpecificTestProperties() : serviceNodes(), keyMap() { }

    bool testSafetyProperties(mace::string& description) {
      return Service::checkSafetyProperties(description, serviceNodes, keyMap);
    }
    bool testLivenessProperties(mace::string& description) {
      return Service::checkLivenessProperties(description, serviceNodes, keyMap);
    }

  private:
    typedef mace::map<int, SpecificTestProperties<Service>*, mace::SoftState> TestPropertyMap;

    static int lastPath;
    static TestPropertyMap testMap;

  public:
    static void registerInstance(int testId, Service const * sv) {
      if (lastPath != SimulatorFlags::getPathNum()) { 
        testMap.clear();
        lastPath = SimulatorFlags::getPathNum();
      }
      typename TestPropertyMap::iterator i = testMap.find(testId);
      if (i == testMap.end()) {
        SpecificTestProperties<Service>* s = new SpecificTestProperties<Service>();
        TestProperties::addTest(s);
        testMap[testId] = s;
        i = testMap.find(testId);
        ASSERT(i != testMap.end());
      }
      i->second->serviceNodes[SimulatorFlags::getCurrentNode()] = sv;

      //       std::cerr << "Registering instance " << (intptr_t) sv;
      //       std::cerr << " addr " << sv->localAddress();
      //       std::cerr << " node " << SimulatorFlags::getCurrentNode();
      //       std::cerr << " map " << i->second->keyMap.toString() << std::endl;

      i->second->keyMap[sv->localAddress()] = SimulatorFlags::getCurrentNode();
    }
};

template <class Service> int SpecificTestProperties<Service>::lastPath = -1;
template <class Service> mace::map<int, SpecificTestProperties<Service>*, mace::SoftState> SpecificTestProperties<Service>::testMap;

}

#endif
