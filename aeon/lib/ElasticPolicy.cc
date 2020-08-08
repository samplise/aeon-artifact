#include "ElasticPolicy.h"
#include "ContextMapping.h"
#include "ContextBaseClass.h"
#include "ContextOwnership.h"

const uint8_t mace::ElasticityRule::GLOBAL_RULE;
const uint8_t mace::ElasticityRule::SLA_ACTOR_MARKER_RULE;
const uint8_t mace::ElasticityRule::INIT_PLACEMENT;

/*****************************class EventAccessInfo**********************************************/
void mace::EventAccessInfo::serialize(std::string& str) const{
  mace::serialize( str, &eventType );
  mace::serialize( str, &isTargetContext );
  
  mace::serialize( str, &totalFromEventCount );
  mace::serialize( str, &totalToEventCount );
  
  mace::serialize( str, &totalEventExecuteTime );
  
  mace::serialize( str, &fromContextCounts );
  mace::serialize( str, &fromMessagesSize );

  mace::serialize( str, &toContextCounts );
}

int mace::EventAccessInfo::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &eventType );
  serializedByteSize += mace::deserialize( is, &isTargetContext   );

  serializedByteSize += mace::deserialize( is, &totalFromEventCount   );
  serializedByteSize += mace::deserialize( is, &totalToEventCount   );

  serializedByteSize += mace::deserialize( is, &totalEventExecuteTime   );

  serializedByteSize += mace::deserialize( is, &fromContextCounts   );
  serializedByteSize += mace::deserialize( is, &fromMessagesSize   );

  serializedByteSize += mace::deserialize( is, &toContextCounts   );
  return serializedByteSize;
}

void mace::EventAccessInfo::addFromEventAccess(const uint64_t execute_time, const mace::string& create_ctx_name, const uint64_t& msg_size ) {
  totalFromEventCount ++;
  totalEventExecuteTime += execute_time;

  if( fromContextCounts.find(create_ctx_name) == fromContextCounts.end() ) {
    fromContextCounts[create_ctx_name] = 1;
    fromMessagesSize[create_ctx_name] = msg_size;
  } else {
    fromContextCounts[create_ctx_name] ++;
    fromMessagesSize[create_ctx_name] += msg_size;
  }
}

void mace::EventAccessInfo::addToEventAccess( const mace::string& to_ctx_name ) {
  totalToEventCount ++;
  
  if( toContextCounts.find(to_ctx_name) == toContextCounts.end() ) {
    toContextCounts[to_ctx_name] = 1;
  } else {
    toContextCounts[to_ctx_name] ++;
  }
}

mace::map< mace::string, uint64_t > mace::EventAccessInfo::getFromEventAccessCount( const mace::string& context_type ) const {
  mace::map< mace::string, uint64_t > ctx_eaccess_counts;

  for( mace::map< mace::string, uint64_t >::const_iterator iter = fromContextCounts.begin(); iter != fromContextCounts.end(); 
      iter ++ ) {

    if( Util::isContextType(iter->first, context_type) ) {
      ctx_eaccess_counts[ iter->first ] = iter->second;
    }
  }
  return ctx_eaccess_counts;
}

/*****************************class ContetxtInteractionInfo**********************************************/
void mace::ContetxtInteractionInfo::serialize(std::string& str) const{
  mace::serialize( str, &callerMethodCount );
  mace::serialize( str, &calleeMethodCount );
}

int mace::ContetxtInteractionInfo::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &callerMethodCount   );
  serializedByteSize += mace::deserialize( is, &calleeMethodCount   );
  return serializedByteSize;
}

void mace::ContetxtInteractionInfo::addCallerContext( mace::string const& methodType, uint64_t count ) {
  if( callerMethodCount.find(methodType) == callerMethodCount.end() ){
    callerMethodCount[methodType] = count;
  } else {
    callerMethodCount[methodType] += count;
  }
}
  
void mace::ContetxtInteractionInfo::addCalleeContext( mace::string const& methodType, uint64_t count ) {
  if( calleeMethodCount.find(methodType) == calleeMethodCount.end() ){
    calleeMethodCount[methodType] = count;
  } else {
    calleeMethodCount[methodType] += count;
  }
}


/*****************************class EventRuntimeInfo**********************************************/
void mace::EventRuntimeInfo::serialize(std::string& str) const{
  mace::serialize( str, &eventId );
  mace::serialize( str, &eventMethodType );
  mace::serialize( str, &curEventExecuteTime );
  mace::serialize( str, &curEventExecuteTimestamp );
  mace::serialize( str, &isTargetContext );
  mace::serialize( str, &createContextName );
  mace::serialize( str, &messageSize );
}

int mace::EventRuntimeInfo::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &eventId );
  serializedByteSize += mace::deserialize( is, &eventMethodType   );
  serializedByteSize += mace::deserialize( is, &curEventExecuteTime   );
  serializedByteSize += mace::deserialize( is, &curEventExecuteTimestamp   );
  serializedByteSize += mace::deserialize( is, &isTargetContext   );
  serializedByteSize += mace::deserialize( is, &createContextName   );
  serializedByteSize += mace::deserialize( is, &messageSize   );
  return serializedByteSize;
}

/*****************************class ContextRuntimeInfo**********************************************/
mace::ContextRuntimeInfo& mace::ContextRuntimeInfo::operator=( mace::ContextRuntimeInfo const& orig ){
  contextName = orig.contextName;

  curPeriodStartTimestamp = orig.curPeriodStartTimestamp;
  curPeriodEndTimestamp = orig.curPeriodEndTimestamp;
  timePeriod = orig.timePeriod;

  markerStartTimestamp = orig.markerStartTimestamp;
  markerTotalTimeperiod = orig.markerTotalTimeperiod;
  markerTotalCount = orig.markerTotalCount;
  
  eventAccessInfos = orig.eventAccessInfos;
  
  contextInterInfos = orig.contextInterInfos;

  eventRuntimeInfos = orig.eventRuntimeInfos;

  coaccessContextsCount = orig.coaccessContextsCount;

  return *this;
}

void mace::ContextRuntimeInfo::serialize(std::string& str) const{
  mace::serialize( str, &contextName );

  mace::serialize( str, &curPeriodStartTimestamp );
  mace::serialize( str, &curPeriodEndTimestamp );
  mace::serialize( str, &timePeriod );

  mace::serialize( str, &markerStartTimestamp );
  mace::serialize( str, &markerTotalTimeperiod );
  mace::serialize( str, &markerTotalCount );

  mace::serialize( str, &eventAccessInfos );
  mace::serialize( str, &contextInterInfos );
  mace::serialize( str, &eventRuntimeInfos );
  mace::serialize( str, &coaccessContextsCount );
}

int mace::ContextRuntimeInfo::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &contextName );

  serializedByteSize += mace::deserialize( is, &curPeriodStartTimestamp );
  serializedByteSize += mace::deserialize( is, &curPeriodEndTimestamp   );
  serializedByteSize += mace::deserialize( is, &timePeriod   );

  serializedByteSize += mace::deserialize( is, &markerStartTimestamp );
  serializedByteSize += mace::deserialize( is, &markerTotalTimeperiod   );
  serializedByteSize += mace::deserialize( is, &markerTotalCount   );

  serializedByteSize += mace::deserialize( is, &eventAccessInfos   );
  serializedByteSize += mace::deserialize( is, &contextInterInfos  );
  serializedByteSize += mace::deserialize( is, &eventRuntimeInfos );

  serializedByteSize += mace::deserialize( is, &coaccessContextsCount );
  return serializedByteSize;
}

void mace::ContextRuntimeInfo::clear() {
  ScopedLock sl(runtimeInfoMutex);
  eventAccessInfos.clear();
  contextInterInfos.clear();
  eventRuntimeInfos.clear();
  coaccessContextsCount.clear();

  markerStartTimestamp.clear();
  markerTotalTimeperiod.clear();
  markerTotalCount.clear();
}

void mace::ContextRuntimeInfo::runEvent( const mace::Event& event ){
  ADD_SELECTORS("ContextRuntimeInfo::runEvent");
  ScopedLock sl(runtimeInfoMutex);
  mace::map<mace::OrderID, EventRuntimeInfo>::iterator iter = eventRuntimeInfos.find(event.eventId);
  if( iter == eventRuntimeInfos.end() ){
    EventRuntimeInfo eventRuntimeInfo(event.eventId, event.eventMethodType);

    if( this->contextName == event.target_ctx_name ){
      eventRuntimeInfo.isTargetContext = true;
    } else {
      eventRuntimeInfo.isTargetContext = false;
    }
    eventRuntimeInfo.createContextName = event.create_ctx_name;
    eventRuntimeInfos[event.eventId] = eventRuntimeInfo;
    // macedbg(1) << "Create EventRuntimeInfo for event("<< event.eventId <<") in context("<< this->contextName <<")!" << Log::endl;
  } 
  // contexts interaction
  if( this->contextName != event.target_ctx_name ){
    const mace::string& methodType = event.eventOpInfo.methodName;
    const mace::string& callerContext = event.eventOpInfo.fromContextName;

    if( contextInterInfos.find(callerContext) == contextInterInfos.end() ){
      ContetxtInteractionInfo ctxInterInfo;
      contextInterInfos[callerContext] = ctxInterInfo;
    }
    contextInterInfos[callerContext].addCallerContext( methodType, 1 );
    macedbg(1) << "Add routine("<< methodType <<") context("<< callerContext <<") -> context("<< contextName <<")!" << Log::endl;
  }
  
  eventRuntimeInfos[event.eventId].curEventExecuteTimestamp = TimeUtil::timeu();
}

void mace::ContextRuntimeInfo::addEventMessageSize( const mace::Event& event, const uint64_t msg_size ) {
  ADD_SELECTORS("ContextRuntimeInfo::addEventMessageSize");
  macedbg(1) << "event("<< event.eventId <<"): eventMethodType=" << event.eventMethodType << ", msgSize=" << msg_size << Log::endl; 
  ScopedLock sl(runtimeInfoMutex);
  mace::map<mace::OrderID, EventRuntimeInfo>::iterator iter = eventRuntimeInfos.find(event.eventId);
  if( iter == eventRuntimeInfos.end() ){
    EventRuntimeInfo eventRuntimeInfo(event.eventId, event.eventMethodType);

    if( this->contextName == event.target_ctx_name ){
      eventRuntimeInfo.isTargetContext = true;
    } else {
      eventRuntimeInfo.isTargetContext = false;
    }
    eventRuntimeInfo.createContextName = event.create_ctx_name;
    eventRuntimeInfos[event.eventId] = eventRuntimeInfo;
    // macedbg(1) << "Create EventRuntimeInfo for event("<< event.eventId <<") in context("<< this->contextName <<")!" << Log::endl;
  }

  eventRuntimeInfos[event.eventId].messageSize = msg_size;
}

void mace::ContextRuntimeInfo::stopEvent( const mace::OrderID& eventId ) {
  ADD_SELECTORS("ContextRuntimeInfo::stopEvent");
  // macedbg(1) << "To stop event " << eventId << " in context("<< this->contextName <<")" << Log::endl;
  ScopedLock sl(runtimeInfoMutex);
  if( eventRuntimeInfos.find(eventId) == eventRuntimeInfos.end()) {
    return;
  }
  eventRuntimeInfos[eventId].curEventExecuteTime += TimeUtil::timeu() - eventRuntimeInfos[eventId].curEventExecuteTimestamp;
}

void mace::ContextRuntimeInfo::addCalleeContext( mace::string const& calleeContext, mace::string const& methodType ){
  ScopedLock sl(runtimeInfoMutex);
  if( contextInterInfos.find(calleeContext) == contextInterInfos.end() ){
    ContetxtInteractionInfo ctxInterInfo;
    contextInterInfos[calleeContext] = ctxInterInfo;
  }
  contextInterInfos[calleeContext].addCalleeContext( methodType, 1 );
}
  
void mace::ContextRuntimeInfo::commitEvent( const mace::OrderID& eventId ) {
  ADD_SELECTORS("ContextRuntimeInfo::commitEvent");
  ScopedLock sl(runtimeInfoMutex);
  mace::map<mace::OrderID, EventRuntimeInfo>::iterator iter = eventRuntimeInfos.find(eventId);

  if( iter != eventRuntimeInfos.end() ){
    //macedbg(1) << "To delete EventRuntimeInfo for " << eventId << " in context("<< this->contextName <<")" << Log::endl;
    EventRuntimeInfo& eventRuntimeInfo = iter->second;
    if( !eventRuntimeInfo.isTargetContext ){
      return;
    }

    macedbg(1) << "Event("<< eventRuntimeInfo.eventMethodType <<"): context("<< eventRuntimeInfo.createContextName <<") -> context("<< contextName <<")!" << Log::endl;

    if( eventAccessInfos.find(eventRuntimeInfo.eventMethodType) == eventAccessInfos.end() ){
      EventAccessInfo eventAccessInfo( eventRuntimeInfo.eventMethodType, eventRuntimeInfo.isTargetContext );
      eventAccessInfo.addFromEventAccess(eventRuntimeInfo.curEventExecuteTime, eventRuntimeInfo.createContextName, 
        eventRuntimeInfo.messageSize);
      eventAccessInfos[eventRuntimeInfo.eventMethodType] = eventAccessInfo;
    } else {
      eventAccessInfos[eventRuntimeInfo.eventMethodType].addFromEventAccess(eventRuntimeInfo.curEventExecuteTime, 
        eventRuntimeInfo.createContextName, eventRuntimeInfo.messageSize );
    }

    eventRuntimeInfos.erase(iter);
  }
}

void mace::ContextRuntimeInfo::addCoaccessContext( mace::string const& context ) {
  ScopedLock sl(runtimeInfoMutex);
  if( coaccessContextsCount.find(context) == coaccessContextsCount.end() ) {
    coaccessContextsCount[ context ] = 1;
  } else {
    coaccessContextsCount[ context ] ++;
  }
}

void mace::ContextRuntimeInfo::addToEventAccess( const mace::string& method_type, const mace::string& to_ctx_name ) {
  ScopedLock sl(runtimeInfoMutex);
  if( eventAccessInfos.find(method_type) == eventAccessInfos.end() ) {
    EventAccessInfo info(method_type, false);
    eventAccessInfos[ method_type ] = info;
  } 
  eventAccessInfos[ method_type ].addToEventAccess(to_ctx_name);
}

uint64_t mace::ContextRuntimeInfo::getCPUTime() {
  ScopedLock sl(runtimeInfoMutex);
  uint64_t cpu_time = 0;
  for( mace::map<mace::string, EventAccessInfo>::const_iterator iter = eventAccessInfos.begin(); iter != eventAccessInfos.end(); 
      iter ++ ) {
    cpu_time += (iter->second).getTotalExecuteTime();
  }
  return cpu_time;
}

mace::string mace::ContextRuntimeInfo::getCoaccessContext( mace::string const& context_type ) {
  ScopedLock sl(runtimeInfoMutex);
  for( mace::map< mace::string, uint64_t >::const_iterator iter = coaccessContextsCount.begin(); iter != coaccessContextsCount.end(); 
      iter ++ ) {
    if( Util::isContextType(iter->first, context_type) && iter->second > mace::eMonitor::EVENT_COACCESS_THRESHOLD ) {
      return iter->first;
    }
  }
  return "";
}

uint32_t mace::ContextRuntimeInfo::getConnectionStrength( const mace::ContextMapping& snapshot, const mace::MaceAddr& addr ) {
  ScopedLock sl(runtimeInfoMutex);
  uint32_t strength = 0;
  for( mace::map<mace::string, ContetxtInteractionInfo>::const_iterator iter = contextInterInfos.begin(); iter != contextInterInfos.end();
      iter ++ ) {
    if( mace::ContextMapping::getNodeByContext(snapshot, iter->first) == addr ) {
      strength += (iter->second).getInterCount();
    }
  }
  return strength;
}

mace::vector<mace::string> mace::ContextRuntimeInfo::getInterctContextNames( mace::string const& context_type ) {
  ADD_SELECTORS("ContextRuntimeInfo::getInterctContextNames");
  ScopedLock sl(runtimeInfoMutex);
  mace::vector<mace::string> inter_ctx_names;
  for( mace::map<mace::string, ContetxtInteractionInfo>::const_iterator iter = contextInterInfos.begin(); iter != contextInterInfos.end();
      iter ++ ) {
    if( Util::isContextType(iter->first, context_type) ) {
      uint64_t inter_count = (iter->second).getInterCount();
      macedbg(1) << "context("<< iter->first <<") inter_count=" << inter_count << Log::endl;
      if( inter_count >= mace::eMonitor::INTER_CONTEXTS_STRONG_CONNECTED_THREAHOLD ) {
        inter_ctx_names.push_back( iter->first );
      }
    }
  }
  return inter_ctx_names;
}

mace::vector<mace::string> mace::ContextRuntimeInfo::getEventInterctContextNames( mace::string const& context_type ) {
  ADD_SELECTORS("ContextRuntimeInfo::getEventInterctContextNames");
  ScopedLock sl(runtimeInfoMutex);
  mace::vector<mace::string> inter_ctx_names;

  mace::map< mace::string, uint64_t> context_eaccess_counts;
  for( mace::map<mace::string, EventAccessInfo>::const_iterator iter = eventAccessInfos.begin(); iter != eventAccessInfos.end();
      iter ++ ) {

    mace::map< mace::string, uint64_t> ctx_eaccess_counts = (iter->second).getFromEventAccessCount(context_type);
    for( mace::map< mace::string, uint64_t>::iterator iter1 = ctx_eaccess_counts.begin(); iter1 != ctx_eaccess_counts.end(); iter1 ++ ) {
      if( context_eaccess_counts.find(iter1->first) == context_eaccess_counts.end() ){
        context_eaccess_counts[ iter1->first ] = iter1->second;
      } else {
        context_eaccess_counts[ iter1->first ] += iter1->second;
      }
    }
  }

  for( mace::map< mace::string, uint64_t>::iterator iter = context_eaccess_counts.begin(); iter != context_eaccess_counts.end(); 
      iter ++ ) {
    
    if( iter->second >= mace::eMonitor::INTER_CONTEXTS_STRONG_CONNECTED_THREAHOLD ) {
      inter_ctx_names.push_back( iter->first );
    }
  }

  return inter_ctx_names;
}

void mace::ContextRuntimeInfo::printRuntimeInformation(const uint64_t& total_ctx_execution_time, const double& server_cpu_usage, 
    const mace::ContextMapping& snapshot, const uint64_t& r ) {
  ADD_SELECTORS("ContextRuntimeInfo::printRuntimeInformation");

  double ctx_execution_time = (double)(this->getCPUTime());
  macedbg(1) << "Context("<< this->contextName <<") Round("<< r <<") execution time=" << ctx_execution_time << "; total execution time=" << total_ctx_execution_time << Log::endl;
  double ctx_cpu_usage = ( ctx_execution_time/total_ctx_execution_time ) * server_cpu_usage;

  mace::map< mace::MaceAddr, uint64_t > servers_comm_count;
  mace::map< mace::string, uint64_t > ctxs_event_size; 

  for( mace::map<mace::string, EventAccessInfo>::iterator iter1 = eventAccessInfos.begin(); iter1 != eventAccessInfos.end(); iter1 ++ ){
    mace::map< mace::string, uint64_t> ctxes_event_access_count = (iter1->second).getFromEventAccessCount();
    for( mace::map<mace::string, uint64_t>::iterator iter2 = ctxes_event_access_count.begin(); iter2 != ctxes_event_access_count.end();
        iter2 ++ ) {
      if( iter2->first == this->contextName ) {
        continue;
      }
      mace::MaceAddr addr = mace::ContextMapping::getNodeByContext( snapshot, iter2->first);
      if( servers_comm_count.find(addr) == servers_comm_count.end() ) {
        servers_comm_count[addr] = iter2->second;
      } else {
        servers_comm_count[addr] += iter2->second;
      }
    }

    mace::map< mace::string, uint64_t> ctxes_event_message_size = (iter1->second).getFromEventMessageSize();
    for( mace::map<mace::string, uint64_t>::iterator iter2 = ctxes_event_message_size.begin(); iter2 != ctxes_event_message_size.end();
        iter2 ++ ) {
      if( iter2->first == this->contextName ) {
        continue;
      }
      if( ctxs_event_size.find(iter2->first) == ctxs_event_size.end() ) {
        ctxs_event_size[iter2->first] = iter2->second;
      } else {
        ctxs_event_size[iter2->first] += iter2->second;
      }
    }
  }

  macedbg(1) << "Context("<< this->contextName <<") runtime information: " << Log::endl;
  macedbg(1) << "Context CPU usage: " << ctx_cpu_usage << Log::endl;
  macedbg(1) << "Context interaction: " << servers_comm_count << Log::endl;
  macedbg(1) << "Context message size: " << ctxs_event_size << Log::endl;
}

void mace::ContextRuntimeInfo::markStartTimestamp( mace::string const& marker ) {
  ScopedLock sl(runtimeInfoMutex);
  markerStartTimestamp[ marker ] = TimeUtil::timeu();
}

void mace::ContextRuntimeInfo::markEndTimestamp( mace::string const& marker ) {
  ScopedLock sl(runtimeInfoMutex);
  if( markerStartTimestamp.find(marker) == markerStartTimestamp.end() ){
    return;
  }

  uint64_t time_period = TimeUtil::timeu() - markerStartTimestamp[marker];
  if( markerTotalTimeperiod.find(marker) == markerTotalTimeperiod.end() ){
    markerTotalTimeperiod[ marker ] = time_period;
    markerTotalCount[ marker ] = 1;
  } else {
    markerTotalTimeperiod[ marker ] += time_period;
    markerTotalCount[ marker ] += 1;
  }
} 

mace::map< mace::MaceAddr, uint64_t > mace::ContextRuntimeInfo::getServerInteractionCount( mace::ContextMapping const& snapshot ) {
  ScopedLock sl(runtimeInfoMutex);
  mace::map< mace::MaceAddr, uint64_t > servers_comm_count;

  for( mace::map<mace::string, EventAccessInfo>::iterator iter1 = eventAccessInfos.begin(); iter1 != eventAccessInfos.end(); iter1 ++ ){
    mace::map< mace::string, uint64_t> ctxes_event_access_count = (iter1->second).getFromEventAccessCount();
    for( mace::map<mace::string, uint64_t>::iterator iter2 = ctxes_event_access_count.begin(); iter2 != ctxes_event_access_count.end();
        iter2 ++ ) {
      if( iter2->first == this->contextName ) {
        continue;
      }
      mace::MaceAddr addr = mace::ContextMapping::getNodeByContext( snapshot, iter2->first);
      if( servers_comm_count.find(addr) == servers_comm_count.end() ) {
        servers_comm_count[addr] = iter2->second;
      } else {
        servers_comm_count[addr] += iter2->second;
      }
    }
  }

  for( mace::map<mace::string, ContetxtInteractionInfo>::iterator iter1 = contextInterInfos.begin(); iter1 != contextInterInfos.end();
      iter1 ++ ) {
    mace::MaceAddr addr = mace::ContextMapping::getNodeByContext( snapshot, iter1->first);
    mace::map<mace::string, uint64_t>& callee_count = (iter1->second).calleeMethodCount;
    for( mace::map<mace::string, uint64_t>::iterator iter2 = callee_count.begin(); iter2 != callee_count.end();
        iter2 ++ ) {
      if( servers_comm_count.find(addr) == servers_comm_count.end() ) {
        servers_comm_count[addr] = iter2->second;
      } else {
        servers_comm_count[addr] += iter2->second;
      } 
    }
  }
  return servers_comm_count;
}

mace::map< mace::string, uint64_t > mace::ContextRuntimeInfo::getEstimateContextsInteractionSize() {
  ADD_SELECTORS("ContextRuntimeInfo::getEstimateContextsInteractionSize");
  ScopedLock sl(runtimeInfoMutex);
  mace::map< mace::string, uint64_t > ctxs_inter_size;

  uint64_t total_remote_msg_size;
  uint64_t total_remote_event_count;

  for( mace::map<mace::string, EventAccessInfo>::iterator iter1 = eventAccessInfos.begin(); iter1 != eventAccessInfos.end(); iter1 ++ ){
    mace::map< mace::string, uint64_t> ctxes_event_access_count = (iter1->second).getFromEventAccessCount();
    mace::map< mace::string, uint64_t> ctxes_event_msg_size = (iter1->second).getFromEventMessageSize();

    for( mace::map<mace::string, uint64_t>::iterator iter2 = ctxes_event_msg_size.begin(); iter2 != ctxes_event_msg_size.end();
        iter2 ++ ) {
      if( iter2->first == this->contextName ) {
        continue;
      }
      if( iter2->second != 0 ) {
        total_remote_msg_size += iter2->second;
        total_remote_event_count += ctxes_event_access_count[ iter2->first ];
      }
    }
  }

  if( total_remote_event_count == 0 ){
    return ctxs_inter_size;
  }

  uint64_t avg_msg_size = (uint64_t)( total_remote_msg_size / total_remote_event_count );
  
  uint64_t estimated_remote_msg_count = 0;
  uint64_t estimated_remote_msg_size = 0;

  for( mace::map<mace::string, EventAccessInfo>::iterator iter1 = eventAccessInfos.begin(); iter1 != eventAccessInfos.end(); iter1 ++ ){
    mace::map< mace::string, uint64_t> ctxes_event_access_count = (iter1->second).getFromEventAccessCount();
    mace::map< mace::string, uint64_t> ctxes_event_msg_size = (iter1->second).getFromEventMessageSize();

    for( mace::map<mace::string, uint64_t>::iterator iter2 = ctxes_event_msg_size.begin(); iter2 != ctxes_event_msg_size.end();
        iter2 ++ ) {
      if( iter2->first == this->contextName ) {
        continue;
      }

      uint64_t msg_size = 0;
      if( iter2->second == 0 ) {
        msg_size = ctxes_event_access_count[ iter2->first ] * avg_msg_size;
        estimated_remote_msg_count += ctxes_event_access_count[ iter2->first ];
        estimated_remote_msg_size += msg_size;
      } else {
        msg_size = iter2->second;
      }

      if( ctxs_inter_size.find(iter2->first) == ctxs_inter_size.end() ) {
        ctxs_inter_size[iter2->first] = msg_size;
      } else {
        ctxs_inter_size[iter2->first] += msg_size;
      }
    }
  }

  macedbg(1) << "avg_msg_size=" << avg_msg_size << ", estimated_remote_msg_count=" << estimated_remote_msg_count 
            << ", estimated_remote_msg_size=" << estimated_remote_msg_size << Log::endl;
  return ctxs_inter_size;
}

mace::map< mace::string, uint64_t > mace::ContextRuntimeInfo::getContextInteractionCount() {
  ScopedLock sl(runtimeInfoMutex);
  mace::map< mace::string, uint64_t > ctxs_comm_count;

  for( mace::map<mace::string, EventAccessInfo>::iterator iter1 = eventAccessInfos.begin(); iter1 != eventAccessInfos.end(); iter1 ++ ){
    mace::map< mace::string, uint64_t> ctxes_event_access_count = (iter1->second).getFromEventAccessCount();
    for( mace::map<mace::string, uint64_t>::iterator iter2 = ctxes_event_access_count.begin(); iter2 != ctxes_event_access_count.end();
        iter2 ++ ) {
      if( iter2->first == this->contextName ) {
        continue;
      }
      if( ctxs_comm_count.find(iter2->first) == ctxs_comm_count.end() ) {
        ctxs_comm_count[iter2->first] = iter2->second;
      } else {
        ctxs_comm_count[iter2->first] += iter2->second;
      }
    }
  }

  for( mace::map<mace::string, ContetxtInteractionInfo>::iterator iter1 = contextInterInfos.begin(); iter1 != contextInterInfos.end();
      iter1 ++ ) {
    mace::map<mace::string, uint64_t>& caller_count = (iter1->second).callerMethodCount;
    for( mace::map<mace::string, uint64_t>::iterator iter2 = caller_count.begin(); iter2 != caller_count.end();
        iter2 ++ ) {
      if( ctxs_comm_count.find(iter1->first) == ctxs_comm_count.end() ) {
        ctxs_comm_count[iter1->first] = iter2->second;
      } else {
        ctxs_comm_count[iter1->first] += iter2->second;
      } 
    }
  }
  return ctxs_comm_count;
}

mace::map< mace::string, uint64_t > mace::ContextRuntimeInfo::getFromAccessCountByTypes( const mace::set<mace::string>& context_types ) {
  ADD_SELECTORS("ContextRuntimeInfo::getFromAccessCountByTypes");
  ScopedLock sl(runtimeInfoMutex);
  mace::map< mace::string, uint64_t > ctxs_comm_count;

  for( mace::map<mace::string, EventAccessInfo>::iterator iter1 = eventAccessInfos.begin(); iter1 != eventAccessInfos.end(); iter1 ++ ){
    mace::map< mace::string, uint64_t> ctxes_event_access_count = (iter1->second).getFromEventAccessCount();

    for( mace::map<mace::string, uint64_t>::iterator iter2 = ctxes_event_access_count.begin(); iter2 != ctxes_event_access_count.end();
        iter2 ++ ) {
      if( iter2->first == this->contextName ) {
        continue;
      }

      mace::string context_type = Util::extractContextType(iter2->first);
      if( context_types.size() > 0 && context_types.count(context_type) == 0 ){
        continue;
      }
      if( ctxs_comm_count.find(iter2->first) == ctxs_comm_count.end() ) {
        ctxs_comm_count[iter2->first] = iter2->second;
      } else {
        ctxs_comm_count[iter2->first] += iter2->second;
      }
    }
  }

  for( mace::map<mace::string, ContetxtInteractionInfo>::iterator iter1 = contextInterInfos.begin(); iter1 != contextInterInfos.end();
      iter1 ++ ) {
    mace::string context_type = Util::extractContextType(iter1->first);
    if( context_types.size() > 0 && context_types.count(context_type) == 0 ){
      continue;
    }

    mace::map<mace::string, uint64_t>& caller_count = (iter1->second).callerMethodCount;
    for( mace::map<mace::string, uint64_t>::iterator iter2 = caller_count.begin(); iter2 != caller_count.end();
        iter2 ++ ) {
      if( ctxs_comm_count.find(iter1->first) == ctxs_comm_count.end() ) {
        ctxs_comm_count[iter1->first] = iter2->second;
      } else {
        ctxs_comm_count[iter1->first] += iter2->second;
      } 
    }
  }
  return ctxs_comm_count;
}

mace::map< mace::string, uint64_t > mace::ContextRuntimeInfo::getToAccessCountByTypes( const mace::set<mace::string>& context_types ) {
  ADD_SELECTORS("ContextRuntimeInfo::getToAccessCountByTypes");
  ScopedLock sl(runtimeInfoMutex);
  mace::map< mace::string, uint64_t > ctxs_comm_count;

  for( mace::map<mace::string, EventAccessInfo>::iterator iter1 = eventAccessInfos.begin(); iter1 != eventAccessInfos.end(); iter1 ++ ){
    mace::map< mace::string, uint64_t> ctxes_event_access_count = (iter1->second).getToEventAccessCount();

    for( mace::map<mace::string, uint64_t>::iterator iter2 = ctxes_event_access_count.begin(); iter2 != ctxes_event_access_count.end();
        iter2 ++ ) {
      if( iter2->first == this->contextName ) {
        continue;
      }

      mace::string context_type = Util::extractContextType(iter2->first);
      if( context_types.size() > 0 && context_types.count(context_type) == 0 ){
        continue;
      }
      if( ctxs_comm_count.find(iter2->first) == ctxs_comm_count.end() ) {
        ctxs_comm_count[iter2->first] = iter2->second;
      } else {
        ctxs_comm_count[iter2->first] += iter2->second;
      }
    }
  }

  for( mace::map<mace::string, ContetxtInteractionInfo>::iterator iter1 = contextInterInfos.begin(); iter1 != contextInterInfos.end();
      iter1 ++ ) {
    mace::string context_type = Util::extractContextType(iter1->first);
    if( context_types.size() > 0 && context_types.count(context_type) == 0 ){
      continue;
    }

    mace::map<mace::string, uint64_t>& caller_count = (iter1->second).calleeMethodCount;
    for( mace::map<mace::string, uint64_t>::iterator iter2 = caller_count.begin(); iter2 != caller_count.end();
        iter2 ++ ) {
      if( ctxs_comm_count.find(iter1->first) == ctxs_comm_count.end() ) {
        ctxs_comm_count[iter1->first] = iter2->second;
      } else {
        ctxs_comm_count[iter1->first] += iter2->second;
      } 
    }
  }
  return ctxs_comm_count;
}

uint64_t mace::ContextRuntimeInfo::getMarkerTotalLatency( const mace::string& marker ) {
  ScopedLock sl(runtimeInfoMutex);
  if( markerTotalTimeperiod.find(marker) == markerTotalTimeperiod.end() ) {
    return 0;
  }
  return markerTotalTimeperiod[marker];
}

double mace::ContextRuntimeInfo::getMarkerAvgLatency( const mace::string& marker ) {
  ScopedLock sl(runtimeInfoMutex);
  if( markerTotalTimeperiod.find(marker) == markerTotalTimeperiod.end() ) {
    return 0;
  }
  double avg_latency = markerTotalTimeperiod[marker] / markerTotalCount[marker];
  return avg_latency;
}

uint64_t mace::ContextRuntimeInfo::getMarkerCount( const mace::string& marker ) {
  ScopedLock sl(runtimeInfoMutex);
  if( markerTotalCount.find(marker) == markerTotalCount.end() ) {
    return 0;
  }
  return markerTotalCount[marker];
}
  
/***************************** class ElasticityCondition **********************************************/
mace::set<mace::string> mace::ElasticityCondition::satisfyCondition( 
    const std::map<mace::string, ContextRuntimeInfoForElasticity>& contextsRuntimeInfo, const ServerRuntimeInfo& server_info,
    const mace::set<mace::string>& checkContexts, const ContextStructure& ownerships ) const {
  
  ADD_SELECTORS("ElasticityCondition::satisfyCondition");
  mace::set< mace::string > satisfied_contexts;

  if( checkContexts.size() == 0 ){
    return satisfied_contexts;
  }

  // macedbg(1) << "conditionType=" << (uint32_t)conditionType << ", compareType=" << (uint32_t)compareType << ", marker=" << marker << ", threshold=" << threshold << Log::endl;
  // macedbg(1) << "checkContexts: " << checkContexts << Log::endl;

  if( conditionType == MARKER_LATENCY_PERCENT ){
    // if( checkContexts.size() <= 1 ){
    //   return satisfied_contexts;
    // }

    // ASSERT(marker != "");
    // mace::map<mace::string, double> marker_latencys;
    // for( uint32_t i=0; i<contexts.size(); i++ ){
    //   marker_latencys[ contexts[i]->contextName ] = contexts[i]->getMarkerAvgLatency( marker ); 
    // }

    // if( marker_latencys.size() == 0 ) {
    //   return satisfied_contexts;
    // }

    // for( mace::set<mace::string>::const_iterator iter = checkContexts.begin(); iter != checkContexts.end(); iter ++ ) {
    //   ASSERT( marker_latencys.find(*iter) != marker_latencys.end() );
    //   double latency = marker_latencys[*iter];

    //   uint32_t count = 0;
    //   if( compareType == COMPARE_ME ) {
    //     for( mace::map<mace::string, double>::iterator marker_iter = marker_latencys.begin(); marker_iter != marker_latencys.end();
    //         marker_iter ++ ) {
    //       if( latency >= marker_iter->second ){
    //         count ++;
    //       }
    //     }

    //     double percent = count / marker_latencys.size();
    //     macedbg(1) << "context("<< *iter <<") percent=" << percent << Log::endl;
    //     if( percent >= threshold ){
    //       satisfied_contexts.insert( *iter );
    //     }
    //   }
    // }
  } else if( conditionType == ACCESS_COUNT_PERCENT ) {
    if( checkContexts.size() <= 1 ){
      return satisfied_contexts;
    }
    mace::map<mace::string, uint64_t> access_counts;
    for( std::map<mace::string, ContextRuntimeInfoForElasticity>::const_iterator iter = contextsRuntimeInfo.begin();
        iter != contextsRuntimeInfo.end(); iter ++ ){
      access_counts[ iter->first ] = (iter->second).getTotalFromAccessCount(); 
      macedbg(1) << "context("<< iter->first <<") total_from_access_count=" << access_counts[iter->first] << Log::endl;
    }

    for( mace::set<mace::string>::const_iterator iter = checkContexts.begin(); iter != checkContexts.end(); iter ++ ) {
      ASSERT( access_counts.find(*iter) != access_counts.end() );
      uint64_t access_count = access_counts[*iter];

      double count = 0;
      for( mace::map<mace::string, uint64_t>::iterator access_iter = access_counts.begin(); access_iter != access_counts.end();
          access_iter ++ ) {
        if( access_count >= access_iter->second ){
          count ++;
        }
      }
      double percent = count / access_counts.size();
      macedbg(1) << "context("<< *iter <<") percent=" << percent << Log::endl;
      
      if( compareType == COMPARE_ME && percent >= threshold ) {
        satisfied_contexts.insert( *iter );
      } else if( compareType == COMPARE_LE && percent <= threshold) {
        satisfied_contexts.insert( *iter );
      }
    }
  } else if( conditionType == SERVER_CPU_USAGE) {
    if( compareType == COMPARE_ME ) {
      if( server_info.CPUUsage >= threshold ) {
        for( mace::set<mace::string>::const_iterator iter = checkContexts.begin(); iter != checkContexts.end(); iter ++ ){
          satisfied_contexts.insert(*iter);
        }
      }
    }
  } else if( conditionType == REFERENCE ) {
    for( mace::set<mace::string>::const_iterator iter = checkContexts.begin(); iter != checkContexts.end(); iter ++ ){
      mace::vector<mace::string> p_ctx_names = this->getReferenceParentContextNames(ownerships, *iter);
      if( p_ctx_names.size() > 0 ){
        satisfied_contexts.insert( *iter );
      }
    }
  } else if( conditionType == EVENT_ACCESS_COUNT_PERCENT ) {
    if( checkContexts.size() <= 1 ){
      return satisfied_contexts;
    }
    mace::map<mace::string, uint64_t> access_counts;
    for( std::map<mace::string, ContextRuntimeInfoForElasticity>::const_iterator iter = contextsRuntimeInfo.begin();
        iter != contextsRuntimeInfo.end(); iter ++ ){
      access_counts[ iter->first ] = (iter->second).getTotalClientRequestNumber(); 
      macedbg(1) << "context("<< iter->first <<") total_event_access_count=" << access_counts[iter->first] << Log::endl;
    }

    for( mace::set<mace::string>::const_iterator iter = checkContexts.begin(); iter != checkContexts.end(); iter ++ ) {
      ASSERT( access_counts.find(*iter) != access_counts.end() );
      uint64_t access_count = access_counts[*iter];

      double count = 0;
      for( mace::map<mace::string, uint64_t>::iterator access_iter = access_counts.begin(); access_iter != access_counts.end();
          access_iter ++ ) {
        if( access_count > 0 && access_count >= access_iter->second ){
          count ++;
        }
      }
      double percent = count / access_counts.size();
      macedbg(1) << "context("<< *iter <<") percent=" << percent << Log::endl;
      
      if( compareType == COMPARE_ME && percent >= threshold ) {
        satisfied_contexts.insert( *iter );
      } else if( compareType == COMPARE_LE && percent <= threshold) {
        satisfied_contexts.insert( *iter );
      }
    }
  }

  return satisfied_contexts;
}

bool mace::ElasticityCondition::isSLAMaxValueCondition() const {
  if( conditionType == SLA_MAX_VALUE ){
    return true;
  } else {
    return false;
  }
}

mace::vector<mace::string> mace::ElasticityCondition::getReferenceParentContextNames( const ContextStructure& ownerships, 
    const mace::string& ctx_name ) const {
  mace::vector<mace::string> names;
  ASSERT( conditionType == REFERENCE );
  mace::string p_ctx_type = getParentContextType(ctx_name);

  mace::vector<mace::string> p_ctx_names = ownerships.getAllParentContexts(ctx_name);
  for( uint32_t i=0; i<p_ctx_names.size(); i++ ) {
    if( Util::isContextType( p_ctx_names[i], p_ctx_type ) ) {
      names.push_back(p_ctx_names[i]);
    }
  }
  return names;
}

mace::string mace::ElasticityCondition::getParentContextType(const mace::string& ctx_name) const {
  mace::string ctx_type = Util::extractContextType(ctx_name);
  ASSERT( conditionType == REFERENCE && contextTypes.size() == 2 );

  mace::set<mace::string>::iterator iter = contextTypes.begin();
  mace::string ctx_type1 = *iter;
  iter ++;
  mace::string ctx_type2 = *iter;

  mace::string p_ctx_type = "";
  if( ctx_type == ctx_type1 ){
    p_ctx_type = ctx_type2;
  } else if( ctx_type == ctx_type2 ){
    p_ctx_type = ctx_type1;
  }

  return p_ctx_type;
}

void mace::ElasticityCondition::serialize(std::string& str) const{
  mace::serialize( str, &conditionType );
  mace::serialize( str, &compareType );
  mace::serialize( str, &contextTypes );
  mace::serialize( str, &marker );
  mace::serialize( str, &threshold );

}

int mace::ElasticityCondition::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &conditionType );
  serializedByteSize += mace::deserialize( is, &compareType );
  serializedByteSize += mace::deserialize( is, &contextTypes );
  serializedByteSize += mace::deserialize( is, &marker );
  serializedByteSize += mace::deserialize( is, &threshold );
  return serializedByteSize;
}

/***************************** class ElasticityAndConditions **********************************************/
void mace::ElasticityAndConditions::serialize(std::string& str) const{
  mace::serialize( str, &conditions );
}

int mace::ElasticityAndConditions::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &conditions );
  return serializedByteSize;
}

mace::vector< mace::string > mace::ElasticityAndConditions::satisfyAndConditions( const mace::vector<mace::string>& contexts, 
    const std::map<mace::string, ContextRuntimeInfoForElasticity>& contextsRuntimeInfo, const ServerRuntimeInfo& server_info,
    const ContextStructure& ownerships ) const {
  mace::set< mace::string > ctx_names;
  mace::vector<mace::string> satisfied_contexts;

  for( uint32_t i=0; i<contexts.size(); i++ ){
    ctx_names.insert( contexts[i] );
  }

  for( uint64_t i=0; i<conditions.size(); i++ ) {
    ctx_names = conditions[i].satisfyCondition( contextsRuntimeInfo, server_info, ctx_names, ownerships );
  }

  for( mace::set<mace::string>::iterator iter = ctx_names.begin(); iter != ctx_names.end(); iter ++ ) {
    satisfied_contexts.push_back(*iter);
  }

  return satisfied_contexts;
}

bool mace::ElasticityAndConditions::isSLAMaxValueCondition() const {
  if( conditions.size() == 1 && conditions[0].isSLAMaxValueCondition() ){
    return true;
  } else {
    return false;
  }
}

bool mace::ElasticityAndConditions::includeReferenceCondition() const {
  if( conditions.size() == 1 && conditions[0].conditionType == ElasticityCondition::REFERENCE ){
    return true;
  } else {
    return false;
  }
}

mace::ElasticityCondition mace::ElasticityAndConditions::getReferenceCondition() const {
  if( conditions.size() == 1 && conditions[0].conditionType == ElasticityCondition::REFERENCE ){
    return conditions[0];
  } else {
    mace::ElasticityCondition cond;
    return cond;
  }
}

mace::vector<mace::string> mace::ElasticityAndConditions::getReferenceParentContextNames( const ContextStructure& ownerships, 
    const mace::string& ctx_name ) const {
  mace::vector<mace::string> names;
  for( uint32_t i=0; i<conditions.size(); i++ ){
    mace::vector<mace::string> p_names = conditions[i].getReferenceParentContextNames( ownerships, ctx_name );
    if( p_names.size() > 0 ){
      for( uint32_t j=0; j<p_names.size(); j++ ){
        names.push_back(p_names[i]);
      }
      break;
    }
  }
  return names;
}

/***************************** class ElasticityBehavior **********************************************/
void mace::ElasticityBehavior::serialize(std::string& str) const{
  mace::serialize( str, &behaviorType );
  mace::serialize( str, &resourceType );
  mace::serialize( str, &contextNames );
  mace::serialize( str, &contextTypes );
}

int mace::ElasticityBehavior::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &behaviorType );
  serializedByteSize += mace::deserialize( is, &resourceType );
  serializedByteSize += mace::deserialize( is, &contextNames );
  serializedByteSize += mace::deserialize( is, &contextTypes );
  return serializedByteSize;
}



/***************************** class ElasticityRule **********************************************/
mace::set<mace::string> mace::ElasticityRule::satisfyGlobalRuleConditions( const mace::vector<mace::string>& contexts, 
    const std::map<mace::string, ContextRuntimeInfoForElasticity>& contextsRuntimeInfo, const ServerRuntimeInfo& server_info,
    const ContextStructure& ownerships ) const {
  ADD_SELECTORS("ElasticityRule::satisfyGlobalRuleConditions");

  mace::set<mace::string> satisfied_contexts;
  if( ruleType != GLOBAL_RULE ){
    return satisfied_contexts;
  }

  mace::vector<mace::string> checkContexts;
  if( relatedContextTypes.count("ANY") > 0 ) {
    checkContexts = contexts;
  } else {
    for( mace::vector<mace::string>::const_iterator iter = contexts.begin(); iter != contexts.end(); iter ++) {
      mace::string context_type = Util::extractContextType(*iter);
      if( relatedContextTypes.count(context_type) > 0 ) {
        checkContexts.push_back(*iter);
      }
    }
  }

  // find satisfied contexts
  for( uint64_t i=0; i<conditions.size(); i++ ) {
    mace::vector<mace::string> s_ctx_names = conditions[i].satisfyAndConditions(checkContexts, contextsRuntimeInfo, server_info, ownerships);
    for( uint32_t j=0; j<s_ctx_names.size(); j++ ) {
      if( satisfied_contexts.count(s_ctx_names[j]) == 0 ){
        satisfied_contexts.insert(s_ctx_names[j]);
      }
    }
  }

  return satisfied_contexts;
}

bool mace::ElasticityRule::isSLAMaxValueRule() const {
  if( conditions.size() == 1 && conditions[0].isSLAMaxValueCondition() ){
    return true;
  } else {
    return false;
  }
}

bool mace::ElasticityRule::relatedContextType( const mace::string& context_type ) const {
  if(relatedContextTypes.count(context_type) > 0 ){
    return true;
  } else {
    return false;
  }
}

mace::ElasticityCondition mace::ElasticityRule::getReferenceCondition() const {
  for( uint64_t i=0; i<conditions.size(); i++ ) {
    if( conditions[i].includeReferenceCondition() ) {
      return conditions[i].getReferenceCondition();
    }
  }

  mace::ElasticityCondition cond;
  return cond;
}

bool mace::ElasticityRule::includeReferenceCondition() const {
  for( uint64_t i=0; i<conditions.size(); i++ ) {
    if( conditions[i].includeReferenceCondition() ) {
      return true;
    }
  }
  return false;
}

mace::ElasticityBehaviorAction* mace::ElasticityRule::generateElasticityAction( 
    const ContextRuntimeInfoForElasticity& ctx_runtime_info, 
    const mace::ContextMapping& snapshot, const ContextStructure& ownerships,
    const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info,
    mace::map< mace::MaceAddr, double >& servers_cpu_usage, mace::map< mace::MaceAddr, double >& servers_cpu_time, 
    mace::map< mace::MaceAddr, uint64_t >& servers_creq_count,
    mace::map< mace::MaceAddr, mace::set<mace::string> >& migration_ctx_sets ) const {

  ADD_SELECTORS("ElasticityRule::generateElasticityAction");

  const mace::string context_name = ctx_runtime_info.contextName;
  double local_cpu_usage = servers_cpu_usage[Util::getMaceAddr()];
  uint64_t local_creq_count = servers_creq_count[Util::getMaceAddr()];
  double ctx_exec_time = ctx_runtime_info.contextExecTime;

  if( behavior.behaviorType == ElasticityBehavior::ISOLATE && behavior.resourceType == ElasticityBehavior::RES_CPU ) {
    // double migrated_local_cpu = 100 * ( servers_cpu_usage[Util::getMaceAddr()]*servers_cpu_time[Util::getMaceAddr()]*0.01 - ctx_exec_time ) / (servers_cpu_time[Util::getMaceAddr()]);
    
    uint64_t from_access_count = ctx_runtime_info.getTotalFromAccessCount();
    if( from_access_count == 0 ){
      return NULL;
    }
    
    double smallest_cpu_usage = 100.0;
    mace::MaceAddr target_addr;

    for( mace::map< mace::MaceAddr, double >::iterator iter = servers_cpu_usage.begin(); iter != servers_cpu_usage.end(); iter ++ ) {
      if( iter->first == Util::getMaceAddr() ) {
        continue;
      }

      double cpu_usage = iter->second;
      double accept_cpu_usage = 100 * ( cpu_usage*servers_cpu_time[iter->first]*0.01 + ctx_exec_time ) / servers_cpu_time[iter->first];
      if( accept_cpu_usage <= local_cpu_usage && accept_cpu_usage < smallest_cpu_usage ) {
        smallest_cpu_usage = accept_cpu_usage;
        target_addr = iter->first;
      }
    }

    if( smallest_cpu_usage < 100 ) {
      double benefit = local_cpu_usage - smallest_cpu_usage;
      macedbg(1) << "context("<< ctx_runtime_info.contextName <<") will migrate to server("<< target_addr <<") with benefit("<< benefit <<")!" << Log::endl;
      ElasticityBehaviorAction* action = new ElasticityBehaviorAction( mace::ElasticityBehaviorAction::EBACTION_MIGRATION, target_addr, 
        ctx_runtime_info.contextName, benefit );
      action->specialRequirement = mace::MigrationContextInfo::IDLE_CPU;
      return action;
    }  
  } else if( behavior.behaviorType == ElasticityBehavior::ISOLATE && behavior.resourceType == ElasticityBehavior::RES_NET ) {
    uint64_t creq_count = ctx_runtime_info.getTotalClientRequestNumber();
    macedbg(1) << "context("<< ctx_runtime_info.contextName <<") creq_count="<< creq_count <<", local_creq_count=" << local_creq_count << Log::endl;
      
    uint64_t smallest_creq_count = 0;
    mace::MaceAddr target_addr;
    bool flag = false;

    for( mace::map< mace::MaceAddr, uint64_t >::iterator iter = servers_creq_count.begin(); iter != servers_creq_count.end(); iter ++ ) {
      if( iter->first == Util::getMaceAddr() ) {
        continue;
      }

      uint64_t remote_creq_count = iter->second;
      uint64_t accept_creq_count = creq_count + remote_creq_count;
      macedbg(1) << "server("<< iter->first <<") accept_creq_count=" << accept_creq_count << Log::endl;
      if( (smallest_creq_count == 0 || smallest_creq_count > accept_creq_count) && accept_creq_count < local_creq_count ) {
        smallest_creq_count = accept_creq_count;
        target_addr = iter->first;
        flag = true;
      }
    }

    if( flag ) {
      double benefit = local_creq_count - smallest_creq_count;
      macedbg(1) << "context("<< ctx_runtime_info.contextName <<") will migrate to server("<< target_addr <<") with benefit("<< benefit <<")!" << Log::endl;
      ElasticityBehaviorAction* action = new ElasticityBehaviorAction( mace::ElasticityBehaviorAction::EBACTION_MIGRATION, target_addr, 
        ctx_runtime_info.contextName, benefit );
      action->specialRequirement = mace::MigrationContextInfo::IDLE_NET;
      return action;
    }  
  } else if( behavior.behaviorType == ElasticityBehavior::COLOCATE ) {
    mace::vector<mace::string> p_names;
    for( uint32_t i=0; i<conditions.size(); i++ ) {
      mace::vector<mace::string> names = conditions[i].getReferenceParentContextNames( ownerships, context_name );
      for( uint32_t j=0; j<names.size(); j++ ) {
        p_names.push_back( names[j] );
      }
    }
    ASSERT( p_names.size() != 0 );

    double smallest_cpu_usage = 100;
    mace::MaceAddr target_addr;

    for( uint32_t i=0; i<p_names.size(); i++ ){
      mace::MaceAddr p_addr = mace::ContextMapping::getNodeByContext( snapshot, p_names[i] );
      if( p_addr == Util::getMaceAddr() ) {
        continue;
      }
    
      double cpu_usage = servers_cpu_usage[p_addr];
      double cpu_time = servers_cpu_time[p_addr];
      double accept_cpu_usage = 100 * ( cpu_usage*cpu_time*0.01 - ctx_exec_time ) / cpu_time;

      if( accept_cpu_usage < mace::eMonitor::CPU_BUSY_THREAHOLD && accept_cpu_usage < smallest_cpu_usage ){
        target_addr = p_addr;
        smallest_cpu_usage = accept_cpu_usage;
      }

    }
    if( smallest_cpu_usage < 100 ){
      ElasticityBehaviorAction* action = new ElasticityBehaviorAction( mace::ElasticityBehaviorAction::EBACTION_MIGRATION, target_addr, 
        context_name, smallest_cpu_usage );
      action->specialRequirement = mace::MigrationContextInfo::BUSY_CPU;
      return action;
    }

        
  }

  return NULL;
}

void mace::ElasticityRule::serialize(std::string& str) const{
  mace::serialize( str, &ruleType );
  mace::serialize( str, &priority );
  mace::serialize( str, &conditions );
  mace::serialize( str, &relatedContextTypes );
  mace::serialize( str, &behavior );
}

int mace::ElasticityRule::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &ruleType );
  serializedByteSize += mace::deserialize( is, &priority );
  serializedByteSize += mace::deserialize( is, &conditions );
  serializedByteSize += mace::deserialize( is, &relatedContextTypes );
  serializedByteSize += mace::deserialize( is, &behavior );
  return serializedByteSize;
}

/***************************** class ElasticityConfiguration **********************************************/

void mace::ElasticityConfiguration::readElasticityPolicies() {

}

mace::vector< mace::ElasticityRule > mace::ElasticityConfiguration::getRulesByContextType( const mace::string& context_type ) const {
  mace::vector< mace::ElasticityRule > rules;
  for( uint32_t i=0; i<elasticityRules.size(); i++ ){
    if( elasticityRules[i].relatedContextTypes.count(context_type) > 0 || elasticityRules[i].relatedContextTypes.count("ANY") > 0 ) {
      rules.push_back( elasticityRules[i] );
    }
  }
  return rules;
}

mace::ElasticityRule mace::ElasticityConfiguration::getContextInitialPlacement( const mace::string& context_type ) const {
  for( uint32_t i=0; i<elasticityRules.size(); i++ ){
    if(  elasticityRules[i].ruleType == mace::ElasticityRule::INIT_PLACEMENT && 
        elasticityRules[i].relatedContextTypes.count(context_type) > 0 ) {
      return elasticityRules[i];
    }
  }

  mace::ElasticityRule rule;
  return rule;
}

