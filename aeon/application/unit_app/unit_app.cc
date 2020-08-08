/* 
 * unit_app.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2012, Wei-Chiu Chuang, Charles Killian
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
#include "ContextJobApplication.h"

/**
 * Uses the "service" variable and the ServiceFactory to instantiate a
 * NullServiceClass registered with the name service.  Runs for "run_time"
 * seconds.
 */
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
  if( ! params::containsKey("nodeset") || ! params::containsKey("mapping") ){
    return;
  }
  NodeSet ns = params::get<NodeSet>("nodeset");

  StringVector mapping = split(params::get<mace::string>("mapping"), '\n');

  typedef mace::map<MaceAddr, mace::list<mace::string> > ContextMappingType;

  StringListVector node_context;

  ASSERT(ns.size() > 0);
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
    ASSERT(key >= 0 && key < ns.size());
    node_context[key].push_back(kv[1]);
  }

  ContextMappingType contextMap;

  int i=0;
  std::vector< MaceAddr > nodeAddrs;
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
    ASSERT(key >= 0 && key < ns.size());
    migrateContexts[ kv[1] ] =  nodeAddrs[key];
  }

}
int main (int argc, char **argv)
{
  mace::Init(argc, argv);
  params::addRequired("service");
  load_protocols();
  mace::ContextJobApplication<NullServiceClass> app;
  app.installSignalHandler();

  if( params::containsKey("logdir") ){
    app.redirectLog( params::get<std::string>("logdir") );
  }
  // if -pid is set, set MACE_PORT based on -pid value. and open fifo channel to talk with heartbeat
  if( params::containsKey("pid") ){
    params::set("MACE_PORT", boost::lexical_cast<std::string>(20000 + params::get<uint32_t>("pid",0 )*5)  );
  }
  params::print(stdout);

  typedef mace::map<MaceAddr, mace::list<mace::string> > ContextMappingType;
  mace::map< mace::string, ContextMappingType > contexts;
  mace::map< mace::string, MaceAddr> migrateContexts;

  mace::string service = params::get<mace::string>("service");
  loadContextFromParam( service,  contexts, migrateContexts );
  app.loadContext(contexts);

  if( service == "FileSync" ){
      ASSERTMSG(params::containsKey("SYNC_NODES"), "Must list the nodes to sync with as SYNC_NODES");
      ASSERTMSG(params::containsKey("SYNC_DIR"), "Must list the directory to sync with as SYNC_DIR");
  }
  uint64_t runtime = (uint64_t)(params::get<double>("run_time", 0) * 1000 * 1000);

  std::cout << "Starting at time " << TimeUtil::timeu() << std::endl;
  app.startService( service);
  app.waitService( runtime );

  app.globalExit();

  return 0;
}
