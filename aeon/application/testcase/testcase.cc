#include <math.h>

#include "lib/Log.h"
#include "lib/mace.h"
#include "lib/TimeUtil.h"
#include "lib/params.h"
#include "lib/mlist.h"
#include "mace-macros.h"

#include <iostream>
#include <fstream>

#include "load_protocols.h"
#include "../services/interfaces/NullServiceClass.h"
#include "ServCompUpcallHandler.h"
#include "ServCompServiceClass.h"
#include "ContextJobApplication.h"
#include "boost/format.hpp"

template <class Service> 
void launchTestCase(const mace::string& service, const uint64_t runtime  ){
  ADD_FUNC_SELECTORS
  macedbg(1)<<"Launch service: "<< service <<Log::endl;
  mace::ContextJobApplication<Service> app;
  app.installSignalHandler();

  StringVector paramContextOwnerships = app.split(params::get<mace::string>("ownership"), '\n');

  params::print(stdout);
  
  mace::vector< mace::pair<mace::string, mace::string> > contextOwnerships;
  for( StringVector::const_iterator it = paramContextOwnerships.begin(); it != paramContextOwnerships.end(); it++ ) {
    StringVector kv = app.split(*it, ':');
    ASSERT(kv.size() == 2);
    
    mace::pair<mace::string, mace::string> ownership(kv[0], kv[1]);
    contextOwnerships.push_back(ownership);
  }
  
  StringVector ns = app.split(params::get<mace::string>("nodeset"), '\n');
  StringVector mapping = app.split(params::get<mace::string>("mapping"), '\n');

  typedef mace::map<MaceAddr, mace::list<mace::string> > ContextMappingType;

  StringListVector node_context;

  ASSERT(ns.size() > 0);
  for( uint32_t i=0; i<ns.size(); i++ ) {
    mace::list<mace::string> string_list;
    node_context.push_back(string_list);
  }

  node_context[0].push_back( mace::ContextMapping::getHeadContext() ); //head context
  //node_context[0].push_back( mace::ContextMapping::GLOBAL_CONTEXT_NAME ); // global
  
  for( StringVector::const_iterator it = mapping.begin(); it != mapping.end(); it++ ) {
    StringVector kv = app.split(*it, ':');
    ASSERT(kv.size() == 2);
    
    uint32_t key;
    istringstream(kv[0]) >> key;
    ASSERT(key >= 0 && key < ns.size());
    node_context[key].push_back(kv[1]);
  }

  ContextMappingType contextMap;

  int i=0;
  for( StringVector::const_iterator it = ns.begin(); it != ns.end(); it++ ) {
    std::cout << "nodeset[" << i << "] = " << *it << std::endl;
    contextMap[ Util::getMaceAddr(*it) ] = node_context[ i++ ];
  }

  mace::map< mace::string, ContextMappingType > contexts;
  contexts[ service ] = contextMap;

  app.loadContext( contexts );
  app.loadOwnerships(contextOwnerships);

  std::cout << "Starting at time " << TimeUtil::timeu() << std::endl;
  if( params::get("use_console", false) == true ){
    app.createConsole();
  }
  app.startService( service );
  app.setTimedMigration();
  app.waitService( runtime );

  //app.globalExit();
}

pthread_mutex_t lock;
uint64_t lastApplicationUpcallTicket = 0;
template<class Service>
class DataHandler: public ServCompUpcallHandler {
public:
  void setService( Service* servobj ){ this->servobj = servobj; }

  ~DataHandler(){
    ADD_FUNC_SELECTORS

    macedbg(1)<<"data handler is deallocated"<<Log::endl;
  }
private:
  Service* servobj;

  void respond ( uint32_t param, registration_uid_t rid = -1){
    ADD_FUNC_SELECTORS
  }

  uint32_t ask( uint32_t param, registration_uid_t rid = -1){
    ADD_FUNC_SELECTORS

    // correctness: synchronous application upcall must proceed in the order of event ticket.
    ScopedLock sl( lock );

    ASSERT( ThreadStructure::myEvent().eventId.ticket >= lastApplicationUpcallTicket );

    lastApplicationUpcallTicket = ThreadStructure::myEvent().eventId.ticket;

    return param;
  }

};
template <class Service> 
void launchUpcallTestCase(const mace::string& service, const uint64_t runtime  ){
  DataHandler<Service> dh;
  mace::ContextJobApplication<Service, DataHandler<Service> > app;
  app.installSignalHandler();

  params::print(stdout);
  std::cout << "Starting at time " << TimeUtil::timeu() << std::endl;

  dh.setService( app.getServiceObject() );

  app.loadContext();
  app.startService( service, &dh );
  app.waitService( runtime );
}
void writeOutProf( int signum ){
  exit(EXIT_SUCCESS);
}
/**
 * Uses the "service" variable and the ServiceFactory to instantiate a
 * NullServiceClass registered with the name service.  Runs for "run_time"
 * seconds.
 */
int main (int argc, char **argv)
{
  SysUtil::signal( SIGINT, writeOutProf ); // intercept ctrl+c and call exit to force gprof output
  mace::Init(argc, argv);
  load_protocols();
  mace::string service;
  uint64_t runtime =  (uint64_t)(params::get<double>("run_time", 2) * 1000 * 1000);
  if( params::containsKey("service") ){
    service = params::get<mace::string>("service");
    launchTestCase<NullServiceClass>( service, runtime );
  }else{
    uint32_t test_case = params::get<uint32_t>("test_case");
    switch( test_case ){
      case 1:
        service = "TestCase1";
        launchTestCase<NullServiceClass>( service, runtime );
        break;
      case 2:
        service = "TestCase2";
        launchTestCase<NullServiceClass>( service, runtime );
        break;
      case 3:
        service = "TestCase3";
        launchTestCase<NullServiceClass>( service, runtime );
        break;
      case 4:
        service = "TestCase4";
        launchUpcallTestCase<ServCompServiceClass>( service, runtime );
        break;
      case 5:
        service = "TestCase5";
        launchUpcallTestCase<ServCompServiceClass>( service, runtime );
        break;
      case 6:
        service = "TestCase6";
        launchTestCase<NullServiceClass>( service, runtime );
        break;
      case 7:
        service = "TestCase7";
        launchTestCase<NullServiceClass>( service, runtime );
        break;
    }
  }

  return 0;
}
