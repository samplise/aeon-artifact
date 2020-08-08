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
#include <algorithm>
#include <functional>
pthread_mutex_t mace::ContextMapping::alock = PTHREAD_MUTEX_INITIALIZER;
std::map< uint64_t, std::set< pthread_cond_t* > > mace::ContextMapping::snapshotWaitingThreads;
const uint32_t mace::ContextMapping::headContext = mace::ContextMapping::HEAD_CONTEXT_ID;
mace::string mace::ContextMapping::headContextName = "(head)";
std::map< uint32_t, MaceAddr > mace::ContextMapping::virtualNodes;
MaceKey mace::ContextMapping::vnodeMaceKey;
mace::map< mace::string, mace::map<MaceAddr, mace::list<mace::string> > > mace::ContextMapping::initialMapping;
//bool mace::ContextMapping::mapped = false;
//mace::MaceAddr mace::ContextMapping::head = SockUtil::NULL_MACEADDR;
void mace::ContextMapEntry::print(std::ostream& out) const {
  out<< "ContextMapEntry(";
  out<< "addr="; mace::printItem(out, &(addr) ); out<<", ";
  out<< "child="; mace::printItem(out, &(child) ); out<<", ";
  out<< "name="; mace::printItem(out, &(name) ); out<<", ";
  out<< "parent="; mace::printItem(out, &(parent) ); out<<", ";
  out<< ")";

} // print

void mace::ContextMapEntry::printNode(PrintNode& pr, const std::string& name) const {
  mace::PrintNode printer(name, "ContextMapEntry" );
  
  mace::printItem( printer, "addr", &addr );
  mace::printItem( printer, "child", &child );
  mace::printItem( printer, "name", &name );
  mace::printItem( printer, "parent", &parent );
  pr.addChild( printer );
}

void mace::ContextMapping::print(std::ostream& out) const {
  out<< "ContextMapping(";
  out<< "head="; mace::printItem(out, &(head) ); out<<", ";
  out<< "mapping="; mace::printItem(out, &(mapping) ); out<<", ";
  out<< "nodes="; mace::printItem(out, &(nodes) ); out<<", ";
  out<< "nameIDMap="; mace::printItem(out, &(nameIDMap) );
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
