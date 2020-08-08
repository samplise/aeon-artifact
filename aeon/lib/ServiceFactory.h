#ifndef __SERVICE_FACTORY_H
#define __SERVICE_FACTORY_H

#include <string>
#include <sstream>
#include <map>
#include "Log.h"
#include "mace-macros.h"
#include "params.h"
#include "SimulatorBasics.h"

namespace mace {

  template<class Interface> class ServiceFactory {
    public:
      typedef Interface& (*factory)(bool);
      typedef std::map<std::string, factory> FactoryMap;
      typedef std::map<std::string, Interface*> CacheMap;

      static FactoryMap factories;
      static CacheMap services;

      static void registerService(factory f, const std::string& name) {
        factories[name] = f;
      }

      static void print(FILE* out) {
        std::ostringstream ostr;
        for (typename FactoryMap::iterator i = factories.begin(); i != factories.end(); i++) {
            ostr << i->first << " ";
        }
        fprintf(out, "Registered services: %s\n", ostr.str().c_str());
      }

      static Interface& create(std::string name, bool notshared) {
        ADD_SELECTORS(std::string("ServiceFactory<")+Interface::name+std::string(">::create"));
        typename FactoryMap::iterator factory = factories.find(name);
        ASSERTMSG(factory != factories.end(), "Service not registered for the interface with the requested name.");
        if (notshared) {
          macedbg(1) << "Private service requested, so creating new " << name << Log::endl;
          return factory->second(0);
        }
        else {
            if (macesim::SimulatorFlags::simulated()) {
                name = std::string("NODE") + macesim::SimulatorFlags::getCurrentNodeString() + std::string("::") + name;
            }
          typename CacheMap::const_iterator i = services.find(name);
          if (i != services.end()) {
            macedbg(1) << "Service " << name << " found, returning cached service" << Log::endl;
            //             printf("sv found, returning saved\n");
            return *(i->second);
          }
          else {
            macedbg(1) << "Service " << name << " not found, creating new" << Log::endl;
            //             printf("sv not found, creating new\n");
            //             Interface* sv = &(factories[name](1));
            //             services[name] = sv;
            //             sv->registerInstance();
            return factory->second(1);
          }
        }
        ABORT("Not possible to reach here!");
      }

      static void registerInstance(std::string name, Interface* ptr) {
        ADD_SELECTORS(std::string("ServiceFactory<")+Interface::name+std::string(">::registerInstance"));
        if (macesim::SimulatorFlags::simulated()) {
          name = std::string("NODE") + macesim::SimulatorFlags::getCurrentNodeString() + std::string("::") + name;
        }
        macedbg(1) << "Registering service named " << name << " address " << (intptr_t)ptr << Log::endl;
        if (services.find(name) == services.end()) {
          macedbg(1) << "Storing service." << Log::endl;
          services[name] = ptr;
        }
        else {
          macedbg(1) << "Instance already registered." << Log::endl;
        }
        return;
      }
      static void unregisterInstance(std::string name, Interface* ptr) {
        ADD_SELECTORS(std::string("ServiceFactory<")+Interface::name+std::string(">::unregisterInstance"));
        if (macesim::SimulatorFlags::simulated()) {
          name = std::string("NODE") + macesim::SimulatorFlags::getCurrentNodeString() + std::string("::") + name;
        }
        macedbg(1) << "Unregistering service named " << name << " address " << (intptr_t)ptr << Log::endl;
        typename CacheMap::iterator i = services.find(name);
        if (i != services.end() && i->second == ptr) {
          //           EXPECT(i->second == ptr);
          macedbg(1) << "Removing service." << Log::endl;
          services.erase(i);
        }
        else {
          macedbg(1) << "Instance already unregistered or not shared." << Log::endl;
        }
        return;
      }
  };

  template<class Interface> std::map<std::string, Interface& (*)(bool)> ServiceFactory<Interface>::factories;
  template<class Interface> std::map<std::string, Interface*> ServiceFactory<Interface>::services;

} //namespace mace

#endif //__SERVICE_FACTORY_H
