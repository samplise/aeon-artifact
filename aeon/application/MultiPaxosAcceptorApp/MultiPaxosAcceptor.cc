#include "SysUtil.h"
#include "lib/mace.h"

#include "TcpTransport-init.h"
#include "load_protocols.h"
#include <signal.h>
#include <string>
#include "mlist.h"

#include "RandomUtil.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <pthread.h>
//#include "TagServiceClass.h"
//#include "Tag-init.h"
#include "ContextJobApplication.h"
#include "PaxosConsensusServiceClass.h"
 
using namespace std;
 
/*class TagResponseHandler : public TagDataHandler {
 
};*/ 
typedef mace::vector<mace::list<mace::string> > StringListVector;
typedef mace::vector<mace::string> StringVector;


std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while(std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  split(s, delim, elems);
  return elems;
}

void loadContextFromParam( const mace::string& service, mace::map< mace::string, ContextMappingType >& contexts, mace::map< mace::string, MaceAddr>& migrateContexts){
  if( ! params::containsKey("nodeset") || ! params::containsKey("mapping") || ! params::containsKey("logical_header") ){
    return;
  }
  NodeSet ns = params::get<NodeSet>("nodeset");
	MaceKey logical_header = params::get<MaceKey>("logical_header");
	
  StringVector mapping = split(params::get<mace::string>("mapping"), '\n');

  typedef mace::map<MaceAddr, mace::list<mace::string> > ContextMappingType;

  StringListVector node_context;

  ASSERT(ns.size() > 0);
	mace::list<mace::string> string_list;
	node_context.push_back(string_list);
  for( uint32_t i=0; i<ns.size(); i++ ) {
    mace::list<mace::string> string_list;
    node_context.push_back(string_list);
  }

  // Set for head node
  node_context[0].push_back( mace::ContextMapping::getHeadContext() ); //head context
  node_context[0].push_back( "" ); // global

  // key:value
  // context_peer_id:context_value
  // 2:A[0] 2:A[1] ...
  
  for( StringVector::const_iterator it = mapping.begin(); it != mapping.end(); it++ ) {
    StringVector kv = split(*it, ':');
    ASSERT(kv.size() == 2);
    
    uint32_t key;
    istringstream(kv[0]) >> key;
    ASSERT(key >= 0 && key < ns.size()+1);
    node_context[key].push_back(kv[1]);
  }

  ContextMappingType contextMap;

  int i=0;
  std::vector< MaceAddr > nodeAddrs;

	std::cout <<"logical_header = "<<logical_header<<std::endl;
	contextMap[ logical_header.getMaceAddr() ] = node_context[i++];
	nodeAddrs.push_back( logical_header.getMaceAddr() );

  for( NodeSet::iterator it = ns.begin(); it != ns.end(); it++ ) {
    std::cout << "nodeset[" << i << "] = " << *it << std::endl;
    contextMap[ (*it).getMaceAddr() ] = node_context[ i++ ];
    nodeAddrs.push_back(  (*it).getMaceAddr() );
  }

  contexts[ service ] = contextMap;

  if( !params::containsKey("migrate") ) return;
  StringVector migrate = split(params::get<mace::string>("migrate"), '\n');
  for( StringVector::const_iterator it = migrate.begin(); it != migrate.end(); it++ ) {
    StringVector kv = split(*it, ':');
    ASSERT(kv.size() == 2);
    
    uint32_t key;
    istringstream(kv[0]) >> key;
    ASSERT(key >= 0 && key < ns.size()+1);
    migrateContexts[ kv[1] ] =  nodeAddrs[key];
  }
}
 
int main(int argc, char* argv[]) {
  //Log::autoAdd("^Tag");
  mace::Init(argc, argv);
  load_protocols();
  uint64_t runtime =  (uint64_t)(params::get<double>("run_time", 2) * 1000 * 1000);
  mace::string service = "WCPaxos";
  
  mace::ContextJobApplication<PaxosConsensusServiceClass> app;
  app.installSignalHandler();

  params::print(stdout);

  typedef mace::map<MaceAddr, mace::list<mace::string> > ContextMappingType;
  mace::map< mace::string, ContextMappingType > contexts;
  mace::map< mace::string, MaceAddr> migrateContexts;

  loadContextFromParam( service,  contexts, migrateContexts );
  app.loadContext(contexts);

  std::cout << "Starting at time " << TimeUtil::timeu() << std::endl;


  // create new thread. one is used to launch the service, one is to initiates the migration request.

  app.startService( service );

  bool ishead = false;
  mace::list< mace::string > &nodectx = contexts[ service ][ Util::getMaceAddr() ];
  mace::list< mace::string >::iterator it = find( nodectx.begin(), nodectx.end(), mace::ContextMapping::getHeadContext() );
  if( it != nodectx.end() ){
    ishead = true;
  }

  if( ishead ){
    uint8_t serviceID = 0; 
    
    uint32_t migration_start = params::get<uint32_t>("migrate_start",1);
    SysUtil::sleepm( 1000* migration_start ); // sleep for one second

    for( mace::map< mace::string, MaceAddr>::iterator migctxIt = migrateContexts.begin(); migctxIt != migrateContexts.end(); migctxIt++ ){
      app.getServiceObject()->requestContextMigration( serviceID, migctxIt->first, migctxIt->second, false );
    }
  }
  app.waitService( runtime );

  if( ishead ){ // head node sents exit event
    app.globalExit();
  }

  return EXIT_SUCCESS;
} 
