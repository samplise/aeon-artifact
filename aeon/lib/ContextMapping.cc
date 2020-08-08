/* 
 * ContextMapping.cc : part of the Mace toolkit for building distributed systems
 * 
 * Copyright (c) 2011, Wei-Chiu Chuang
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
#include "ContextMapping.h"
#include "ContextBaseClass.h"
#include <algorithm>
#include <functional>
pthread_mutex_t mace::ContextMapping::alock = PTHREAD_MUTEX_INITIALIZER;
std::map< uint64_t, std::set< pthread_cond_t* > > mace::ContextMapping::snapshotWaitingThreads;
const uint32_t mace::ContextMapping::headContext = mace::ContextMapping::HEAD_CONTEXT_ID;
//mace::string mace::ContextMapping::headContextName = "(head)";
mace::string mace::ContextMapping::headContextName = "globalContext";
std::map< uint32_t, MaceAddr > mace::ContextMapping::virtualNodes;
MaceKey mace::ContextMapping::vnodeMaceKey;
mace::map< mace::string, mace::map<MaceAddr, mace::list<mace::string> > > mace::ContextMapping::initialMapping;

mace::ContextMapping* mace::ContextMapping::latestContextMapping = NULL;

uint64_t mace::ContextMapping::last_version = 0;
bool mace::ContextMapping::update_flag = true;
uint32_t mace::ContextMapping::nextExternalCommNode = 0;

mace::vector<mace::MaceAddr> mace::ContextMapping::serverAddrs;
mace::map< MaceAddr, mace::ContextNodeInformation > mace::ContextMapping::nodeInfos;

const mace::string mace::ContextMapping::GLOBAL_CONTEXT_NAME = "globalContext";
const mace::string mace::ContextMapping::EXTERNAL_COMM_CONTEXT_NAME = "externalCommContext";
const mace::string mace::ContextMapping::EXTERNAL_SUB_COMM_CONTEXT_NAME = "externalCommContext";
const mace::string mace::ContextMapping::DISTRIBUTED_PLACEMENT = "DISTRIBUTED";



void mace::ContextMapEntry::print(std::ostream& out) const {
  out<< "ContextMapEntry(";
  out<< "addr="; mace::printItem(out, &(addr) ); out<<", ";
  //out<< "child="; mace::printItem(out, &(child) ); out<<", ";
  out<< "name="; mace::printItem(out, &(name) ); out<<", ";
  //out<< "parent="; mace::printItem(out, &(parent) ); out<<", ";
  out<< ")";

} // print

void mace::ContextMapEntry::printNode(PrintNode& pr, const std::string& name) const {
  mace::PrintNode printer(name, "ContextMapEntry" );
  
  mace::printItem( printer, "addr", &addr );
  //mace::printItem( printer, "child", &child );
  mace::printItem( printer, "name", &name );
  //mace::printItem( printer, "parent", &parent );
  pr.addChild( printer );
}

void mace::ContextMapping::print(std::ostream& out) const {
  out<< "ContextMapping(";
  out<< "head="; mace::printItem(out, &(head) ); out<<", ";
  out<< "mapping="; mace::printItem(out, &(mapping) ); out<<", ";
  out<< "nodes="; mace::printItem(out, &(nodes) ); out<<", ";
  out<< "nameIDMap="; mace::printItem(out, &(nameIDMap) ); out<<", ";
  out<< "currentVersion="; mace::printItem(out, &(current_version) );
  out<< ")";

} // print

void mace::ContextMapping::printNode(PrintNode& pr, const std::string& name) const {
  mace::PrintNode printer(name, "ContextMapping" );
  
  mace::printItem( printer, "head", &head );
  mace::printItem( printer, "mapping", &mapping );
  mace::printItem( printer, "nodes", &nodes );
  mace::printItem( printer, "nameIDMap", &nameIDMap );
  pr.addChild( printer );
}
typedef std::pair<uint64_t, const mace::ContextMapping* > ContextMapSnapshotType;
class MatchVersion: public std::binary_function< ContextMapSnapshotType, uint64_t, bool >{
public:
  bool operator()( const ContextMapSnapshotType& snapshot, const uint64_t targetVer) const{
    return (snapshot.first == targetVer );
  }
};
bool mace::ContextMapping::hasSnapshot(const uint64_t ver) const{
  ADD_SELECTORS("ContextMapping::hasSnapshot");
  /*VersionContextMap::reverse_iterator it = std::find_if( versionMap.rbegin(), versionMap.rend(), std::bind2nd( MatchVersion() , ver)  );
  return (it != versionMap.rend());*/
  VersionContextMap::const_iterator it = versionMap.find( ver );
  return (it != versionMap.end() );
}

mace::map<mace::string, mace::MaceAddr> mace::ContextMapping::scaleTo(const uint32_t& n) {
  mace::map<mace::MaceAddr, mace::vector<uint32_t> > old_node_n_context;

  uint32_t n_ctx_per_node = mapping.size() / n;
  for ( ContextMapType::iterator mit = mapping.begin (); mit != mapping.end (); mit++) {
    old_node_n_context[ (mit->second).addr ].push_back(mit->first);
  }

  mace::map<mace::MaceAddr, mace::vector<uint32_t> > new_node_n_context;
  if( old_node_n_context.size() < n ){
    new_node_n_context = old_node_n_context;
    mace::set<mace::MaceAddr> currAddrs;
    for( mace::map<mace::MaceAddr, mace::vector<uint32_t> >::iterator iter=old_node_n_context.begin(); iter!=old_node_n_context.end(); iter++ ){
      currAddrs.insert(iter->first);
    }

    uint32_t n_new_node = n - old_node_n_context.size();
    for( uint32_t i=0; i<serverAddrs.size(); i++ ){
      if( currAddrs.count(serverAddrs[i]) == 0 ){
        mace::vector<uint32_t> ctxIds;
        new_node_n_context[ serverAddrs[i] ] = ctxIds;
        n_new_node --;

        if( n_new_node == 0 ){
          break;
        }
      }
    }

    for( mace::map<mace::MaceAddr, mace::vector<uint32_t> >::iterator iter=old_node_n_context.begin(); 
        iter!=old_node_n_context.end(); iter++ ){

      if( (iter->second).size() <= n_ctx_per_node ){
        continue;
      }
      
      const mace::vector<uint32_t> ctxIds = iter->second;

      for( uint32_t i=n_ctx_per_node; i<ctxIds.size(); i++ ) {
        for( mace::map<mace::MaceAddr, mace::vector<uint32_t> >::iterator niter=new_node_n_context.begin(); 
            niter!=new_node_n_context.end(); niter++ ){

          if( niter->first == iter->first ){
            continue;
          }
          mace::vector<uint32_t>& new_ctxIds = niter->second;
          while( new_ctxIds.size() < n_ctx_per_node && i<ctxIds.size() ){
            new_ctxIds.push_back( ctxIds[i++] );
          }
        }
      }
    }
  } else {
    uint32_t n_node = 0;
    for( mace::map<mace::MaceAddr, mace::vector<uint32_t> >::iterator iter=old_node_n_context.begin(); 
        iter!=old_node_n_context.end(); iter++ ){
      if( n_node <= n && (iter->second).size() < n_ctx_per_node ){
        new_node_n_context[ iter->first ] = iter->second;
        n_node ++;
      }
    }

    for( mace::map<mace::MaceAddr, mace::vector<uint32_t> >::iterator iter=old_node_n_context.begin(); 
        iter!=old_node_n_context.end(); iter++ ){

      if( new_node_n_context.find(iter->first) != new_node_n_context.end() ){
        continue;
      }
      
      const mace::vector<uint32_t> ctxIds = iter->second;

      for( uint32_t i=0; i<ctxIds.size(); i++ ) {
        for( mace::map<mace::MaceAddr, mace::vector<uint32_t> >::iterator niter=new_node_n_context.begin(); 
          niter!=new_node_n_context.end(); niter++ ){

          mace::vector<uint32_t>& new_ctxIds = niter->second;
          while( new_ctxIds.size() < n_ctx_per_node && i<ctxIds.size() ){
            new_ctxIds.push_back( ctxIds[i++] );
          }
        }
      }
    }
  }

  mace::map<mace::string, mace::MaceAddr> newMapping;
  for( mace::map<mace::MaceAddr, mace::vector<uint32_t> >::iterator niter=new_node_n_context.begin(); 
      niter!=new_node_n_context.end(); niter++ ){

    const mace::vector<uint32_t>& new_ctxIds = niter->second;
    for( uint32_t i=0; i<new_ctxIds.size(); i++ ){
      mace::string contextName = mace::ContextMapping::getNameByID( *this, new_ctxIds[i] );
      newMapping[ contextName ] = niter->first;
    }
  }
  return newMapping;
}

const std::pair< mace::MaceAddr, uint32_t> mace::ContextMapping::newMapping( const mace::string& contextName, 
    const mace::ElasticityRule& rule, const mace::string& pContextName ){
  ADD_SELECTORS ("ContextMapping::newMapping");

  macedbg(1) << "Create context("<< contextName <<") RuleType = " << (uint32_t)rule.ruleType<< ", behaviorType = " << (uint32_t)rule.behavior.behaviorType << Log::endl;

  // heuristic 1: if a default mapping is defined, use it.
  mace::map< mace::string , mace::MaceAddr >::const_iterator dmIt = defaultMapping.find( contextName );
  if( dmIt != defaultMapping.end() ){
    const mace::MaceAddr addr = dmIt->second;
    defaultMapping.erase( contextName );
    std::pair<bool, uint32_t> newNode = updateMapping( addr, contextName );
    return std::pair< mace::MaceAddr, uint32_t>(addr, newNode.second);
  }

  // to colocate it with its parent
  if( pContextName != "" ) {
    mace::MaceAddr addr = ContextMapping::getNodeByContext( this->getLatestContextMapping(), pContextName );
    std::pair<bool, uint32_t> newNode = updateMapping( addr, contextName );
    return std::pair< mace::MaceAddr, uint32_t>(addr, newNode.second);
  }

  // for( mace::map<MaceAddr, mace::ContextNodeInformation>::iterator iter = nodeInfos.begin(); iter != nodeInfos.end(); iter++ ){
  //   if( (iter->second).hasContext(contextName) ) {
  //     const mace::MaceAddr addr = iter->first;
  //     std::pair<bool, uint32_t> newNode = updateMapping( addr, contextName );
  //     return std::pair< mace::MaceAddr, uint32_t>(addr, newNode.second);
  //   }
  // }

  // if( rule.behavior.behaviorType == mace::ElasticityBehavior::WORKLOAD_BALANCE ){
  //   const mace::vector<mace::string>& wl_ctx_types = rule.behavior.contextTypes;

  //   mace::set<mace::string> ctx_types;
  //   for( uint32_t i=0; i<wl_ctx_types.size(); i++ ) {
  //     ctx_types.insert( wl_ctx_types[i] );
  //   } 
  //   uint32_t min_n_wl_ctx = 100000;
  //   MaceAddr min_addr;

  //   for( mace::map<MaceAddr, mace::ContextNodeInformation>::iterator iter = nodeInfos.begin(); iter != nodeInfos.end(); iter++ ){
  //     uint32_t n_ctx = (iter->second).getNContextByTypes( ctx_types, inactivateContexts );
      
  //     if( n_ctx < min_n_wl_ctx ){
  //       min_n_wl_ctx = n_ctx;
  //       min_addr = iter->first;
  //     }
  //   }
  //   std::pair<bool, uint32_t> newNode = updateMapping( min_addr, contextName );
  //   return std::pair< mace::MaceAddr, uint32_t>(min_addr, newNode.second);
  // } else if( rule.behavior.behaviorType == mace::ElasticityBehavior::RANDOM ) {
  //   MaceAddr addr;

  //   while( true ){
  //     uint32_t i = RandomUtil::randInt() % serverAddrs.size();
  //     if( i != 0 ){
  //       addr = serverAddrs[i];
  //       break;
  //     } 
  //   }
  //   std::pair<bool, uint32_t> newNode = updateMapping( addr, contextName );
  //   return std::pair< mace::MaceAddr, uint32_t>( addr, newNode.second);
  // }
  
      
  // heuristic 2: map the context to the same node as its parent context
  // Special case #1: global context map to head node
  if( contextName == GLOBAL_CONTEXT_NAME ){ 
    const mace::MaceAddr& headAddr = ContextMapping::getHead( *this ); // find head addr in the latest mapping
    ASSERTMSG( headAddr != SockUtil::NULL_MACEADDR, "Head node address is NULL_MACEADDR!" );
    std::pair<bool, uint32_t> newNode = updateMapping( headAddr, contextName );
    macedbg(1) << "Create global context object: "<< newNode.second << Log::endl;
    return std::pair< mace::MaceAddr, uint32_t>(headAddr, newNode.second);
  }

      // Special case #2: external communication context
  const mace::string contextTypeName = mace::ContextBaseClass::getTypeName( contextName );
  if( contextTypeName == ContextMapping::EXTERNAL_COMM_CONTEXT_NAME ) {
    const mace::MaceAddr& headAddr = ContextMapping::getHead( *this );
    std::pair<bool, uint32_t> newNode = updateMapping( headAddr, contextName );
    return std::pair< mace::MaceAddr, uint32_t>(headAddr, newNode.second);
  }

  // put context on head node
  const mace::MaceAddr& newAddr = ContextMapping::getHead( *this );
  ASSERTMSG( newAddr != SockUtil::NULL_MACEADDR, "Node address is NULL_MACEADDR!" );
  std::pair<bool, uint32_t> newNode = updateMapping( newAddr, contextName );
  return std::pair< mace::MaceAddr, uint32_t>(newAddr, newNode.second);
}

mace::MaceAddr mace::ContextMapping::getExternalCommContextNode( const mace::string& contextName ) const{
  ADD_SELECTORS ("ContextMapping::getExternalCommContextNode");
  macedbg(1) << "To get node of " << contextName << Log::endl;
  macedbg(1) << "defaultMapping=" << defaultMapping << Log::endl;
  // heuristic 1: if a default mapping is defined, use it.
  mace::map< mace::string , mace::MaceAddr >::const_iterator dmIt = defaultMapping.find( contextName );
  if( dmIt != defaultMapping.end() ){
    const mace::MaceAddr addr = dmIt->second;
    macedbg(1) << "Default node of " << contextName << " is " << addr << Log::endl;
    return addr;
  }

  // put context on head node
  const mace::MaceAddr& newAddr = ContextMapping::getHead( *this );
  macedbg(1) << "Get headnode " << newAddr << Log::endl;
  return newAddr;
}

void mace::ContextMapping::colocateContexts( mace::string const& ctx_name1, mace::string const& ctx_name2 ) {
  ADD_SELECTORS("ContextMapping::colocateContexts");
  macedbg(1) << "To colocate context("<< ctx_name1 <<") with context("<< ctx_name2 <<")!" << Log::endl;
  ScopedLock sl(alock);
  for( mace::map<MaceAddr, mace::ContextNodeInformation>::iterator iter = nodeInfos.begin(); iter != nodeInfos.end(); iter++ ){
    if( (iter->second).hasContext(ctx_name1) ) {
      (iter->second).addContext(ctx_name2);
      macedbg(1) << "To colocate context("<< ctx_name1 <<") with context("<< ctx_name2 <<") on server("<< iter->first <<")!" << Log::endl;
      break;
    }
  }
}
    
void mace::ContextMapping::separateContexts( mace::string const& ctx_name1, mace::string const& ctx_name2 ) {
  ScopedLock sl(alock);
  while( true ){
    uint32_t m = RandomUtil::randInt( serverAddrs.size() );
    if( !nodeInfos[ serverAddrs[m] ].hasContext(ctx_name1) ){
      nodeInfos[ serverAddrs[m] ].addContext( ctx_name2 );
      break;
    }
  }
}

void mace::ContextMapping::isolateContext( mace::string const& ctx_name ) {
  ScopedLock sl(alock);
  uint32_t min_n_context = 10000000;
  mace::MaceAddr min_addr;
  for( mace::map<MaceAddr, mace::ContextNodeInformation>::iterator iter = nodeInfos.begin(); iter != nodeInfos.end(); iter++ ){
    if( (iter->second).getTotalNContext() < min_n_context ) {
      min_n_context = (iter->second).getTotalNContext();
      min_addr = iter->first;
    }
  }
  nodeInfos[ min_addr ].addContext(ctx_name);
}

void mace::ContextMapping::inactivateContext( const mace::string& ctx_name ) {
  ADD_SELECTORS("ContextMapping::inactivateContext");
  ScopedLock sl(alock);
  if( inactivateContexts.count(ctx_name) == 0 ) {
    inactivateContexts.insert(ctx_name);
    macedbg(1) << "To inactivate context " << ctx_name << Log::endl;
  }
}

void mace::ContextMapping::activateContext( const mace::string& ctx_name ) {
  ScopedLock sl(alock);
  inactivateContexts.erase(ctx_name);
}

std::pair< bool , uint32_t> mace::ContextMapping::updateMapping( const mace::MaceAddr & node, const mace::string & context ){
  ADD_SELECTORS ("ContextMapping::updateMapping");
  ScopedLock sl (alock);

  if( _hasContext( context ) ){
    const uint32_t contextID = findIDByName( context );
    mace::MaceAddr oldNode = mapping[ contextID ].addr;

        // XXX: head node??
    if( --nodes[ oldNode ] == 0 && oldNode != head){
      nodes.erase( oldNode );
    }
    mapping[ contextID ].addr = node;
    nodeInfos[ oldNode ].removeContext( context );
  }else{
    insertMapping( context, node );
  }

  bool newNode = false;
  if( nodes.find( node ) == nodes.end() ){
    newNode = true;
  }

  nodes[ node ] ++;
  nodeInfos[ node ].addContext( context );
  current_version ++;
  macedbg(1) << "current_version increased to " << current_version << Log::endl;

  return std::pair< bool, uint32_t >( newNode, nContexts );
}

/**************************************** class ContextNodeInformation *********************************************************/
void mace::ContextNodeInformation::serialize(std::string& str) const{
  mace::serialize( str, &nodeAddr );
  mace::serialize( str, &nodeId );
  mace::serialize( str, &contextTypeNumberMap );
  mace::serialize( str, &contextNames );
}

int mace::ContextNodeInformation::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &nodeAddr   );
  serializedByteSize += mace::deserialize( is, &nodeId   );
  serializedByteSize += mace::deserialize( is, &contextTypeNumberMap   );
  serializedByteSize += mace::deserialize( is, &contextNames   );
  return serializedByteSize;
}

uint32_t mace::ContextNodeInformation::getNContextByTypes( mace::set< mace::string> const& context_types, 
    mace::set<mace::string> const& inactivateContexts ) const {
  uint32_t n = 0;
  for( mace::set<mace::string>::const_iterator iter = contextNames.begin(); iter != contextNames.end(); iter ++ ) {
    if( inactivateContexts.count(*iter) > 0 ) {
      continue;
    }
    mace::string context_type = Util::extractContextType(*iter);
    if( context_types.count(context_type) > 0 ) {
      n ++;
    }
  }

  return n;
}

bool mace::ContextNodeInformation::hasContext( mace::string const& contextName ) const {
  if( contextNames.count(contextName) == 0 ){
    return false;
  } else {
    return true;
  }
}

void mace::ContextNodeInformation::addContext( mace::string const& contextName ) {
  ADD_SELECTORS("ContextNodeInformation::addContext");
  if( contextNames.count(contextName) > 0 ){
    return;
  }

  contextNames.insert( contextName );
  const mace::string context_type = Util::extractContextType( contextName );

  if( contextTypeNumberMap.find(context_type) == contextTypeNumberMap.end() ){
    contextTypeNumberMap[context_type] = 1;
  } else {
    contextTypeNumberMap[context_type] ++;
  }
  macedbg(1) << "Add context("<< contextName <<") of type("<< context_type <<") to node("<< nodeAddr <<")!" << Log::endl;
}
      
void mace::ContextNodeInformation::removeContext( mace::string const& contextName ) {
  if( contextNames.count(contextName) == 0 ){
    return;
  }

  contextNames.erase( contextName );

  const mace::string context_type = Util::extractContextType( contextName );
  contextTypeNumberMap[context_type] --;
}




