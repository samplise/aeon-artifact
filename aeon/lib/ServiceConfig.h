#ifndef __SERVICE_CONFIG_H
#define __SERVICE_CONFIG_H

#include "params.h"
#include "ServiceFactory.h"

namespace mace {

  extern StringSet serviceConfigStack;

  template<class Interface> class ServiceConfig {
    public:
      typedef mace::map<mace::string, StringSet> NameAttributeMap;

      static NameAttributeMap attributes;

      static void registerService(const std::string& name, const StringSet& attr) {
        attributes[name] = attr;
      }

      static bool checkAttributes(const std::string& name, const StringSet& attr) {
        ADD_SELECTORS(std::string("ServiceConfig<")+Interface::name+std::string(">::checkAttributes"));
        if (!attributes.containsKey(name)) { macedbg(1) << "Service " << name << " not registered." << Log::endl; return false; }
        const StringSet& svattr = attributes[name];
        for (StringSet::const_iterator i = attr.begin(); i != attr.end(); i++) {
          if (!svattr.containsKey(*i)) {
            macedbg(1) << "Service " << name << " does not provide attribute " << *i << Log::endl;
            return false;
          }
        }
        return true;
      }

      static Interface& checkAndCreate(const std::string& key, std::string name, const StringSet& attr, bool unique) {
        ADD_SELECTORS(std::string("ServiceConfig<")+Interface::name+std::string(">::checkAndCreate"));
        if (!checkAttributes(name, attr)) { 
          maceerr << "Service " << name << " is not registered with attributes " << attr << Log::endl;
          dump();
          ABORT("Specified service fails to meet attribute check");
        }
        return create(key, name, unique);
      }

      static Interface& create(const std::string& key, std::string name, bool unique) {
        ADD_SELECTORS(std::string("ServiceConfig<")+Interface::name+std::string(">::create"));

        if (! serviceConfigStack.insert(name).second) {
          maceerr << "Error constructing services.  Dump: " 
                  << "Interface: " << Interface::name << " "
                  << "Duplicate Service to Construct: " << name << " "
                  << "Stack: " << serviceConfigStack << Log::endl;
          ABORT("Recursion in Service Configuration!");
        }

        maceout << "Creating service " << name << " for key " << key << Log::endl;

        Interface* s = &(ServiceFactory<Interface>::create(name, unique));
        serviceConfigStack.erase(name);

        return *s;
      }

      static void dump() {
        ADD_SELECTORS(std::string("ServiceConfig<")+Interface::name+std::string(">::dump"));
        maceout << "Registered services: " << attributes << Log::endl;
      }

      static Interface& configure(const std::string& key, const StringSet& attr, const StringSet& optattr, bool unique) {
        ADD_SELECTORS(std::string("ServiceConfig<")+Interface::name+std::string(">::configure"));

        //Look for specific key
        std::string iKey("ServiceConfig.");
        iKey += key;
        if (params::containsKey(iKey)) {
          return checkAndCreate(key, params::get<std::string>(iKey), attr, unique);
        }

        ASSERTMSG(!attributes.empty(), "Attempting to configure a service with an interface, but no services are registered with that interface!");

        ASSERTMSG(optattr.size() == 0 || optattr.size() == 1, "Can specify either 0 or 1 optional attribute.  More are not supported due to complexity of matching.");
        //NOTE: in general, this function is far too complicated, and not
        //perfect.  We're just hoping that its good enough.

        //Look for service default for specific attributes with optional attributes
        if ( !optattr.empty() ) {

          //Check on services with all attributes
          StringSet combined(attr);
          combined.insert(optattr.begin(), optattr.end());

          // Look for default with all attributes.
          iKey = std::string("ServiceConfig.") + Interface::name;
          for (StringSet::const_iterator i = combined.begin(); i != combined.end(); i++) {
            iKey += "+" + *i;
          }
          if (params::containsKey(iKey)) {
            // Note: only requiring the required attributes - since it was specified as the default with all attributes
            return checkAndCreate(key, params::get<std::string>(iKey), attr, unique);
          }

          // Check if optional default has all attributes
          iKey = std::string("ServiceConfig.") + Interface::name;
          for (StringSet::const_iterator i = optattr.begin(); i != optattr.end(); i++) {
            iKey += "+" + *i;
          }
          if (params::containsKey(iKey)) {
            // Note: checking all attributes on the default without optional attributes.
            if (checkAttributes(params::get<std::string>(iKey), combined)) {
              return create(key, params::get<std::string>(iKey), unique);
            }
          }

          if (!attr.empty()) {
            // Check if required default has all attributes.
            iKey = std::string("ServiceConfig.") + Interface::name;
            for (StringSet::const_iterator i = attr.begin(); i != attr.end(); i++) {
              iKey += "+" + *i;
            }
            if (params::containsKey(iKey)) {
              // Note: checking all attributes on the required default without optional attributes.
              if (checkAttributes(params::get<std::string>(iKey), combined)) {
                return create(key, params::get<std::string>(iKey), unique);
              }
            }
          }

          // Check if interface default has all attributes.
          iKey = std::string("ServiceConfig.") + Interface::name;
          if (params::containsKey(iKey)) {
            // Note: checking all attributes on the interface default without optional attributes.
            if (checkAttributes(params::get<std::string>(iKey), combined)) {
              return create(key, params::get<std::string>(iKey), unique);
            }
          }

          // Return a service with all attributes including optional ones.
          for (NameAttributeMap::const_iterator i = attributes.begin(); i != attributes.end(); i++) {
            if (checkAttributes(i->first, combined)) {
              return create(key, i->first, unique);
            }
          }

        }

        if ( !attr.empty() ) {

          // Check if non-optional default has all required attributes.
          iKey = std::string("ServiceConfig.") + Interface::name;
          for (StringSet::const_iterator i = attr.begin(); i != attr.end(); i++) {
            iKey += "+" + *i;
          }
          if (params::containsKey(iKey)) {
            // Note: checking all attributes on the default without optional attributes.
            return checkAndCreate(key, params::get<std::string>(iKey), attr, unique);
          }

          // Check if interface default has all required attributes.
          iKey = std::string("ServiceConfig.") + Interface::name;
          if (params::containsKey(iKey)) {
            // Note: checking all attributes on the default without optional attributes.
            if (checkAttributes(params::get<std::string>(iKey), attr)) {
              return create(key, params::get<std::string>(iKey), unique);
            }
          }

          //Find any service with appropriate attributes
          for (NameAttributeMap::const_iterator i = attributes.begin(); i != attributes.end(); i++) {
            if (checkAttributes(i->first, attr)) {
              return create(key, i->first, unique);
            }
          }

          maceerr << "Error finding service.  Dump: " 
                  << "Interface: " << Interface::name << " "
                  << "Key: " << key << " "
                  << "Attributes Requested: " << attr << " "
                  << "Optional Attributes Requested: " << optattr << " "
                  << "Unique: " << unique << " "
                  << "Registered services: " << attributes << Log::endl;

          ABORT("Could not find appropriate service.");
        }

        ASSERT(attr.empty());
        //ASSERT(no service has the optional attributes either)

        //Check if default interface service is specified?
        iKey = std::string("ServiceConfig.");
        iKey += Interface::name;
        if (params::containsKey(iKey)) {
          return create(key, params::get<std::string>(iKey), unique);
        }

        //Find any service.
        return create(key, attributes.begin()->first, unique);
      }

  };

  template<class Interface> mace::map<mace::string, StringSet> ServiceConfig<Interface>::attributes;

  template<> class ServiceConfig<void*> {
    public:
      template<typename T> static T get(const std::string& key, const T& def) {
        ADD_SELECTORS("ServiceConfig<parameters>::get");
        std::string scKey = std::string("ServiceConfig.") + key;
        const T& ret = params::get<T>(scKey, def);
        maceout << "Configuration key: " << key << " value: " << ret << Log::endl;
        return ret;
      }
  };

} //namespace mace

#endif //__SERVICE_CONFIG_H
