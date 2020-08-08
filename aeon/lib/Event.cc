#include "Event.h"
#include "mace.h"
#include "HeadEventDispatch.h"
#include "ThreadStructure.h"
#include "ContextMapping.h"
#include "SpecialMessage.h"
#include "ContextService.h"

uint64_t mace::Event::nextTicketNumber = 1;
uint64_t mace::Event::lastWriteContextMapping = 0;
bool mace::Event::isExit = false;
mace::OrderID mace::Event::exitEventId;

////////////////// class EvetMessageRecord ///////////////////

bool mace::operator==( mace::EventMessageRecord const& r1, mace::EventMessageRecord const& r2){
  if( r1.sid == r2.sid && r1.dest == r2.dest && r1.message == r2.message && r1.rid == r2.rid ){
    return true;
  }
  return false;
}

////////////////// class EventRequestWrapper ///////////////

mace::EventRequestWrapper & mace::EventRequestWrapper::operator=( mace::EventRequestWrapper const& right ){
#ifndef EVENTREQUEST_USE_SHARED_PTR
  sid = right.sid;
  request = right.request;
#else
  sid = right.sid;
  request = right.request;

#endif
  return *this;
}

mace::EventRequestWrapper::EventRequestWrapper( mace::EventRequestWrapper const& right ): sid( right.sid ), request(){
#ifndef EVENTREQUEST_USE_SHARED_PTR
  request = right.request;
#else
  request = right.request;
#endif
}

mace::EventRequestWrapper::~EventRequestWrapper(){
  //ADD_SELECTORS("ContextService::(destructor)");
#ifndef EVENTREQUEST_USE_SHARED_PTR
  //delete request;
#endif
  //maceout<< "0x"<< (uint64_t)request.get() << " unique? " << request.unique() << Log::endl;
}

void mace::EventRequestWrapper::print(std::ostream& out) const {
  out<< "EventRequestWrapper(";
  out<< "sid="; mace::printItem(out, &(sid) ); out<<", ";
  out<< "request="<< (*request) ;
  out<< ")";
}
void mace::EventRequestWrapper::printNode(PrintNode& pr, const std::string& name) const {
  mace::PrintNode printer(name, "EventRequestWrapper" );
  mace::printItem( printer, "sid", &sid );
  mace::printItem( printer, "request", &request );
  pr.addChild( printer );
}
void mace::EventRequestWrapper::serialize(std::string& str) const{
    mace::serialize( str, &sid );
    request->serialize( str );
}
int mace::EventRequestWrapper::deserialize(std::istream & is) throw (mace::SerializationException){
    int serializedByteSize = 0;
    serializedByteSize += mace::deserialize( is, &sid );

    BaseMaceService* serviceInstance = BaseMaceService::getInstance( sid );
    mace::Message* ptr;
    serializedByteSize += serviceInstance->deserializeMethod( is, ptr );
    request = RequestType(ptr);

    return serializedByteSize;
}



////////////////// EventUpcallWrapper ///////////////

mace::EventUpcallWrapper & mace::EventUpcallWrapper::operator=( mace::EventUpcallWrapper const& right ){
  sid = right.sid;
  upcall = right.upcall;
  return *this;
}
mace::EventUpcallWrapper::EventUpcallWrapper( mace::EventUpcallWrapper const& right ): sid( right.sid ), upcall(){

  upcall = right.upcall;
}
mace::EventUpcallWrapper::~EventUpcallWrapper(){
}
void mace::EventUpcallWrapper::print(std::ostream& out) const {
  out<< "EventUpcallWrapper(";
  out<< "sid="; mace::printItem(out, &(sid) ); out<<", ";
  out<< "upcall="<< (*upcall) ;
  out<< ")";
}
void mace::EventUpcallWrapper::printNode(PrintNode& pr, const std::string& name) const {
  mace::PrintNode printer(name, "EventUpcallWrapper" );
  mace::printItem( printer, "sid", &sid );
  mace::printItem( printer, "upcall", &upcall );
  pr.addChild( printer );
}
void mace::EventUpcallWrapper::serialize(std::string& str) const{
    mace::serialize( str, &sid );
    upcall->serialize( str );
}
int mace::EventUpcallWrapper::deserialize(std::istream & is) throw (mace::SerializationException){
    int serializedByteSize = 0;
    serializedByteSize += mace::deserialize( is, &sid );

    BaseMaceService* serviceInstance = BaseMaceService::getInstance( sid );
    mace::Message* ptr;
    serializedByteSize += serviceInstance->deserializeMethod( is, ptr );
    upcall = ptr;

    return serializedByteSize;
}

////////////////// EventSkipRecord ///////////////

// void mace::EventSkipRecord::print(std::ostream& out) const {
//   out<< "EventSkipRecord(";
//   mace::printItem(out, &(preEventInfos) );
//   out<<")";
// }

// void mace::EventSkipRecord::printNode(PrintNode& pr, const std::string& name) const {
//   mace::PrintNode printer(name, "EventSkipRecord" );
  
//   mace::printItem( printer, "preEventInfos", &preEventInfos );
//   pr.addChild( printer );
// }

// void mace::EventSkipRecord::serialize(std::string& str) const{
//   mace::serialize( str, &preEventInfos );
// }

// int mace::EventSkipRecord::deserialize(std::istream & is) throw (mace::SerializationException){
//   int serializedByteSize = 0;
//   serializedByteSize += mace::deserialize( is, &preEventInfos );
//   return serializedByteSize;
// }

// mace::vector<mace::PreEventInfo> mace::EventSkipRecord::find( const mace::string& context_name ) const{
//   PreEventInfosType::const_iterator sit = preEventInfos.find( context_name );
//   if(sit == preEventInfos.end()){
//     mace::vector<mace::PreEventInfo> rv;
//     return rv;
//   }else{
//     const mace::set<PreEventInfo>& preInfos = sit->second;
//     mace::vector<PreEventInfo> preInfosVec;
//     mace::set<PreEventInfo>::const_iterator sIter = preInfos.begin();
//     for(; sIter!=preInfos.end(); sIter++) {
//       preInfosVec.push_back(*sIter);
//     }
//     return preInfosVec;
//   }
// }

// void mace::EventSkipRecord::set( const mace::string& context_name, const mace::PreEventInfo& eventInfo ){
//   PreEventInfosType::iterator sit = preEventInfos.find( context_name );
//   if(sit == preEventInfos.end()) {
//     preEventInfos[context_name].insert( eventInfo );
//   }else {
//     mace::set<PreEventInfo>& preInfos = sit->second;
//     bool existing_flag = false;
//     for( mace::set<PreEventInfo>::iterator iter = preInfos.begin(); iter != preInfos.end(); iter ++ ) {
//       if( (*iter).eventId == eventInfo.eventId ) {
//         existing_flag = true;
//         break;
//       }
//     }
//     if( !existing_flag ){
//       preInfos.insert(eventInfo);
//     }
//   }
// }

// void mace::EventSkipRecord::removePreEvent( OrderID const& eventId ) {
//   mace::set<mace::string> toDelete;
//   for(PreEventInfosType::iterator m_iter = preEventInfos.begin(); m_iter != preEventInfos.end(); m_iter ++ ) {
//     mace::set<PreEventInfo>& info_set = m_iter->second;
//     for( mace::set<PreEventInfo>::iterator s_iter = info_set.begin(); s_iter != info_set.end(); s_iter ++ ) {
//       const PreEventInfo& info = *s_iter;
//       if( info.eventId == eventId ) {
//         info_set.erase(s_iter);
//         break;
//       }
//     }
//     if( info_set.size() == 0 ) {
//       toDelete.insert(m_iter->first);
//     }
//   }

//   for( mace::set<mace::string>::iterator iter = toDelete.begin(); iter != toDelete.end(); iter ++ ) {
//     preEventInfos.erase(*iter);
//   }
// }

///////////////// class EventOperationInfo ///////////////////////////
mace::string mace::EventOperationInfo::getPreAccessContextName( mace::string const& ctx_name ) const {
  ADD_SELECTORS("EventOperationInfo::getPreAccessContextName");
  ASSERT( accessedContexts.size() > 0 );

  macedbg(1) << "context=" << ctx_name << "; accessedContexts=" << accessedContexts << Log::endl;
  bool exist_flag = false;
  mace::string pre_access_context = "";

  for( int i = (int)(accessedContexts.size())-1; i>=0; i--) {
    if( accessedContexts[i] == ctx_name ){
      exist_flag = true;
      if( i-1>=0 ){
        pre_access_context = accessedContexts[i-1];
      }
      break;
    }
  } 
  ASSERT( exist_flag);
  return pre_access_context;
}

bool mace::EventOperationInfo::hasAccessed( const mace::string& ctx_name ) const {
  for( uint32_t i=0; i<accessedContexts.size(); i++ ) {
    if(accessedContexts[i] == ctx_name){
      return true;
    }
  }
  return false;
}

void mace::EventOperationInfo::setContextDAGVersion( const mace::string& ctx_name, const uint32_t& ver ) {
  ADD_SELECTORS("EventOperationInfo::setContextDAGVersion");
  contextDAGVersions[ctx_name] = ver;
  //macedbg(1) << "Event("<< this->eventId <<"): context("<< ctx_name <<") DAG version=" << ver << Log::endl;
}

void mace::EventOperationInfo::addAccessedContext( mace::string const& ctx_name ) {
  ADD_SELECTORS("EventOperationInfo::addAccessedContext");
  // macedbg(1) << "Add access context("<< ctx_name <<") for " << this->eventId << Log::endl;
  for( uint32_t i=0; i<accessedContexts.size(); i++ ) {
    if(accessedContexts[i] == ctx_name){
      maceerr << "context("<< ctx_name <<") already exists in " << accessedContexts << Log::endl;
      ASSERT(false);
    }
  }
  accessedContexts.push_back(ctx_name);
}

void mace::EventOperationInfo::print(std::ostream& out) const {
  out<< "EventOperationInfo(";
  out<< "EventID="; mace::printItem(out, &(eventId) ); out << ", ";
  out<< "opType="; mace::printItem(out, &(opType) ); out << ", ";
  out<< "toContextName="; mace::printItem(out, &(toContextName)); out << ", ";
  out<< "fromContextName="; mace::printItem(out, &(fromContextName)); out << ", ";
  out<< "ticket="; mace::printItem(out, &(ticket)); out << ", ";
  out<< "eventOpType="; mace::printItem(out, &(eventOpType) ); out << ", ";
  out<< "requireContextName="; mace::printItem(out, &(requireContextName) ); out << ", ";
  // out<< "methodName="; mace::printItem(out, &(methodName) ); out << ", ";
  // out<< "newCreateContexts="; mace::printItem(out, &(newCreateContexts) ); out << ", ";
  // out<< "accessedContexts="; mace::printItem(out, &(accessedContexts) ); out << ", ";
  // out<< "contextDAGVersions="; mace::printItem(out, &(contextDAGVersions) );
  out<<")";
}

void mace::EventOperationInfo::printNode(PrintNode& pr, const std::string& name) const {
  /*
  mace::PrintNode printer(name, "EventOperationInfo" );
  mace::printItem( printer, "EventID", &eventId );
  mace::printItem( printer, "opType", &opType );
  mace::printItem( printer, "toContextName", &toContextName);
  mace::printItem( printer, "fromContextName", &fromContextName );
  mace::printItem( printer, "ticket", &ticket );
  mace::printItem( printer, "controlContextName", &controlContextName );
  pr.addChild( printer );
  */
}

void mace::EventOperationInfo::serialize(std::string& str) const{
  mace::serialize( str, &eventId );
  mace::serialize( str, &opType );
  mace::serialize( str, &fromContextName );
  mace::serialize( str, &toContextName );
  mace::serialize( str, &ticket );
  mace::serialize( str, &eventOpType );
  mace::serialize( str, &methodName );
  mace::serialize( str, &accessedContexts );
  mace::serialize( str, &requireContextName );
  mace::serialize( str, &permitContexts );
  mace::serialize( str, &newCreateContexts );
  mace::serialize( str, &contextDAGVersions );
}

int mace::EventOperationInfo::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &eventId );
  serializedByteSize += mace::deserialize( is, &opType );
  serializedByteSize += mace::deserialize( is, &fromContextName );
  serializedByteSize += mace::deserialize( is, &toContextName );
  serializedByteSize += mace::deserialize( is, &ticket );
  serializedByteSize += mace::deserialize( is, &eventOpType );
  serializedByteSize += mace::deserialize( is, &methodName );
  serializedByteSize += mace::deserialize( is, &accessedContexts );
  serializedByteSize += mace::deserialize( is, &requireContextName );
  serializedByteSize += mace::deserialize( is, &permitContexts );
  serializedByteSize += mace::deserialize( is, &newCreateContexts );
  serializedByteSize += mace::deserialize( is, &contextDAGVersions );
  return serializedByteSize;
}

////////////////// class EventOrderInfo ///////////////
void mace::EventOrderInfo::serialize(std::string& str) const{
  mace::serialize( str, &lockedContexts );
  mace::serialize( str, &localLockRequests );
}

int mace::EventOrderInfo::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &lockedContexts );
  serializedByteSize += mace::deserialize( is, &localLockRequests );
  return serializedByteSize;
}

mace::EventOrderInfo& mace::EventOrderInfo::operator=(const EventOrderInfo& orig) {
  ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
  this->lockedContexts = orig.lockedContexts;
  this->localLockRequests = orig.localLockRequests;
  return *this;
}

void mace::EventOrderInfo::setLocalLockRequests( mace::vector<mace::EventOperationInfo> const& local_lock_requests, 
    mace::vector<mace::string> const& locked_contexts) {

  localLockRequests = local_lock_requests;
  lockedContexts = locked_contexts;
}

////////////////// class Event ///////////////
const uint8_t mace::Event::EVENT_OP_WRITE;
const uint8_t mace::Event::EVENT_OP_OWNERSHIP;

void mace::Event::initialize( ){
  if( !eventContexts.empty() ){
    eventContexts.clear();
  }
  if( !eventSnapshotContexts.empty() ){
    eventSnapshotContexts.clear();
  }

  eventMessages.clear();
  // check if this node is the head node?
  this->eventContextMappingVersion = lastWriteContextMapping;
}

void mace::Event::initialize2( const OrderID& eventId, const uint8_t& event_op_type, const mace::string& create_ctx_name, 
    const mace::string& target_ctx_name, const mace::string& eventMethodType, const int8_t eventType, 
    const uint64_t contextMappingVersion,  const uint64_t contextStructureVersion) {
  this->eventId = eventId;
  this->eventOpType = event_op_type;
  this->create_ctx_name = create_ctx_name; 
  this->target_ctx_name = target_ctx_name;
  this->eventMethodType = eventMethodType;
  this->eventType = eventType;
  this->eventContextMappingVersion = contextMappingVersion;
  this->eventContextStructureVersion = contextStructureVersion;
}

void mace::Event::initialize3( const mace::string& migration_ctx_name, const uint64_t ctxMappingVer ){
  if( !eventContexts.empty() ){
    eventContexts.clear();
  }
  if( !eventSnapshotContexts.empty() ){
    eventSnapshotContexts.clear();
  }

  eventMessages.clear();
  ownershipOps.clear();
  
  // check if this node is the head node?
  this->eventContextMappingVersion = ctxMappingVer;
  this->create_ctx_name = "Global"; 
  this->target_ctx_name = migration_ctx_name;
  this->eventOpType = mace::Event::EVENT_OP_WRITE;
}
  
mace::Event& mace::Event::operator=(const Event& orig) {
  ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
  eventId = orig.eventId;
    
  create_ctx_name = orig.create_ctx_name;
  target_ctx_name = orig.target_ctx_name;
  eventMethodType = orig.eventMethodType;

  eventOpInfo = orig.eventOpInfo;
  eventOpInfoCopy = orig.eventOpInfoCopy;
  eventOrderInfo = orig.eventOrderInfo;

  eventType = orig.eventType;
  eventContexts = orig.eventContexts;
  eventSnapshotContexts = orig.eventSnapshotContexts;
  eventContextMappingVersion = orig.eventContextMappingVersion;
  subevents = orig.subevents;
  eventMessages = orig.eventMessages;
  eventUpcalls = orig.eventUpcalls;

  createCommitFlag = orig.createCommitFlag;
  eventContextStructureVersion = orig.eventContextStructureVersion;
  externalMessageTicket = orig.externalMessageTicket;
  ownershipOps = orig.ownershipOps;
  eventOpType = orig.eventOpType;
  return *this;
}

void mace::Event::print(std::ostream& out) const {
  out<< "Event(";
  out<< "eventId="; mace::printItem(out, &(eventId) ); out<<", ";
  out<< "create context="; mace::printItem(out, &(create_ctx_name)); out<<", ";
  out<< "target context="; mace::printItem(out, &(target_ctx_name)); out<<", ";
  out<< "eventOpInfo="; mace::printItem(out, &eventOpInfo);
  out<< "eventOpInfoCopy="; mace::printItem(out, &eventOpInfoCopy);
  
  out<< "eventType="; 
  
  switch( eventType ){
    case STARTEVENT: out<<"STARTEVENT"; break;
    case ENDEVENT: out<<"ENDEVENT"; break;
    case TIMEREVENT: out<<"TIMEREVENT"; break;
    case ASYNCEVENT: out<<"ASYNCEVENT"; break;
    case UPCALLEVENT: out<<"UPCALLEVENT"; break;
    case DOWNCALLEVENT: out<<"DOWNCALLEVENT"; break;
    case MIGRATIONEVENT: out<<"MIGRATIONEVENT"; break;
    case NEWCONTEXTEVENT: out<<"NEWCONTEXTEVENT"; break;
    case UNDEFEVENT: out<<"UNDEFEVENT"; break;
    case BROADCASTEVENT: out<<"BROADCASTEVENT"; break;
    case ROUTINEEVENT: out<<"ROUTINEEVENT"; break;
    default: mace::printItem(out, &(eventType) ); break;
  }
  
  out<<", ";

  out<< "eventContexts="; mace::printItem(out, &(eventContexts) ); out<<", ";
  out<< "eventSnapshotContexts="; mace::printItem(out, &(eventSnapshotContexts) ); out<<", ";
  out<< "eventContextMappingVersion="; mace::printItem(out, &(eventContextMappingVersion) ); out<<", ";
  out<< "subevents="; mace::printItem(out, &subevents); out<<", ";
  out<< "eventMessages="; mace::printItem(out, &eventMessages); out<<", ";
  out<< "eventUpcalls="; mace::printItem(out, &eventUpcalls); out<<", ";
  out<< "ownershipOps="; mace::printItem(out, &ownershipOps); out<<", ";
  uint16_t uint16_op_type = eventOpType;
  out<< "eventOpType="; mace::printItem(out, &uint16_op_type );
  out<< ")";

} // print

void mace::Event::printNode(PrintNode& pr, const std::string& name) const {
  /*
  mace::PrintNode printer(name, "Event" );
  
  mace::printItem( printer, "eventId", &eventId );
  mace::printItem( printer, "startId", &startId );
  mace::printItem( printer, "target context name", &target_ctx_name );
  mace::printItem( printer, "eventType", &eventType );
  mace::printItem( printer, "eventContexts", &eventContexts );
  mace::printItem( printer, "eventSnapshotContexts", &eventSnapshotContexts );
  mace::printItem( printer, "eventContextMappingVersion", &eventContextMappingVersion );
  mace::printItem( printer, "eventOrderRecords", &eventOrderRecords );
  mace::printItem( printer, "subevents", &subevents );
  mace::printItem( printer, "eventMessages", &eventMessages );
  mace::printItem( printer, "eventUpcalls", &eventUpcalls );
  pr.addChild( printer );
  */
}

void mace::Event::serialize(std::string& str) const{
  mace::serialize( str, &eventType );
  mace::serialize( str, &eventId   );
  if( eventType == UNDEFEVENT ) {
    return;
  }
  
  mace::serialize( str, &create_ctx_name   );
  mace::serialize( str, &target_ctx_name   );
  mace::serialize( str, &eventMethodType );

  mace::serialize( str, &eventOpInfo );
  mace::serialize( str, &eventOpInfoCopy );
  mace::serialize( str, &eventOrderInfo );
  
  mace::serialize( str, &eventContexts   );
  mace::serialize( str, &eventSnapshotContexts   );
  mace::serialize( str, &eventContextMappingVersion   );
  mace::serialize( str, &subevents   );
  mace::serialize( str, &eventMessages   );
  mace::serialize( str, &eventUpcalls   );
  mace::serialize( str, &createCommitFlag   );
  mace::serialize( str, &eventContextStructureVersion );
  mace::serialize( str, &externalMessageTicket );
  mace::serialize( str, &ownershipOps );
  mace::serialize( str, &eventOpType );
}

int mace::Event::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &eventType );
  serializedByteSize += mace::deserialize( is, &eventId   );
        
  // if the eventType is UNDEFEVENT, this event is used in an event request,
  // so the rest of the fields are meaningless.
  if( eventType == UNDEFEVENT )
    return serializedByteSize;

  serializedByteSize += mace::deserialize( is, &create_ctx_name );
  serializedByteSize += mace::deserialize( is, &target_ctx_name );
  serializedByteSize += mace::deserialize( is, &eventMethodType );

  serializedByteSize += mace::deserialize( is, &eventOpInfo );
  serializedByteSize += mace::deserialize( is, &eventOpInfoCopy );
  serializedByteSize += mace::deserialize( is, &eventOrderInfo );
  
  serializedByteSize += mace::deserialize( is, &eventContexts   );
  serializedByteSize += mace::deserialize( is, &eventSnapshotContexts   );
  serializedByteSize += mace::deserialize( is, &eventContextMappingVersion   );
  
  serializedByteSize += mace::deserialize( is, &subevents   );
  serializedByteSize += mace::deserialize( is, &eventMessages   );
  serializedByteSize += mace::deserialize( is, &eventUpcalls   );
  serializedByteSize += mace::deserialize( is, &createCommitFlag );
  serializedByteSize += mace::deserialize( is, &eventContextStructureVersion );
  serializedByteSize += mace::deserialize( is, &externalMessageTicket );
  serializedByteSize += mace::deserialize( is, &ownershipOps );
  serializedByteSize += mace::deserialize( is, &eventOpType );
  return serializedByteSize;
}

void mace::Event::sendDeferredMessages(){
  ThreadStructure::ScopedContextID sc( ContextMapping::getHeadContextID() );
  for( DeferredMessageType::iterator msgIt = eventMessages.begin(); msgIt != eventMessages.end(); msgIt++ ){
    BaseMaceService* serviceInstance = BaseMaceService::getInstance( msgIt->sid );
    serviceInstance->dispatchDeferredMessages( msgIt->dest, msgIt->message, msgIt->rid );
  }
}
bool mace::Event::deferExternalMessage( uint8_t instanceUniqueID, MaceKey const& dest,  std::string const&  message, registration_uid_t const rid ){
  ADD_SELECTORS("Event::deferExternalMessage");
  macedbg(1)<<"defer an external message sid="<<(uint16_t)instanceUniqueID<<", dest="<<dest<<", rid="<<rid<<Log::endl;
  EventMessageRecord emr(instanceUniqueID, dest, message, rid );
  eventMessages.push_back( emr );
  return true;

}
void mace::Event::executeApplicationUpcalls(){
  mace::string dummyString;
  for( DeferredUpcallType::iterator msgIt = eventUpcalls.begin(); msgIt != eventUpcalls.end(); msgIt++ ){
    BaseMaceService* serviceInstance = BaseMaceService::getInstance( msgIt->sid );

    mace::ApplicationUpcall_Message* upcall = static_cast< mace::ApplicationUpcall_Message* >( msgIt->upcall );
    serviceInstance->executeDeferredUpcall( upcall, dummyString );
  }
  clearEventUpcalls();
}
void mace::Event::enqueueDeferredEvents(){
  createToken();

  uint8_t svid = subevents.begin()->sid;
  AsyncEventReceiver* sv = BaseMaceService::getInstance(svid);
  ContextService* _service = static_cast<ContextService*>(sv);

  mace::ContextBaseClass* ctx_obj = _service->getContextObjByName(this->target_ctx_name);

  mace::Event::EventRequestType::const_iterator iter = subevents.begin();
  for( ; iter != subevents.end(); iter++ ) {
    ctx_obj->enqueueCreateEvent(sv, (HeadEventDispatch::eventfunc)&BaseMaceService::createEvent,  iter->request, true );
  }
}

void mace::Event::newEventID( const int8_t type){
  ADD_SELECTORS("Event::newEventID");
  static uint32_t eventCreateIncrement = params::get("EVENT_CREATE_INCREMENT", 1);
  // if end event is generated, raise a flag
  if( type == ENDEVENT ){
    isExit = true;
  }
  //eventId = ThreadStructure::myEventID();

  //Accumulator::Instance(Accumulator::EVENT_CREATE_COUNT)->accumulate(1); // increment committed event number
  if( eventId.ticket % eventCreateIncrement ==0){
    Accumulator::Instance(Accumulator::EVENT_CREATE_COUNT)->accumulate(eventCreateIncrement); // increment committed event number
  }

  eventType = type;
  //macedbg(1) << "Event ticket " << eventID << " sold! "<< Log::endl;//<< *this << Log::endl;
}

void mace::Event::addServiceID( const uint8_t serviceId ) {
  /*
  mace::Event::SkipRecordType::iterator iter = eventOrderRecords.find(serviceId);
  if( iter == eventOrderRecords.end() ) {
    mace::Event::EventSkipRecordType eventSkipRecord;
    eventOrderRecords[serviceId] = eventSkipRecord;
  }
  */
}

void mace::Event::checkSubEvent() const {
  ADD_SELECTORS("Event::checkSubEvent");
  
  EventRequestType copy = subevents;
  if( !subevents.empty() ) {
    EventRequestType::iterator iter = copy.begin();
    mace::AsyncEvent_Message* message = static_cast<AsyncEvent_Message*>(iter->request);
    const mace::__asyncExtraField& extra = message->getExtra(); 
    macedbg(1) << "First AsyncEvent Target Context: " << extra.targetContextID << Log::endl;
  }
  
}

mace::string mace::Event::getParentContextName(mace::string const& childContextName) const {
  mace::string parentContextName = "";
  for( uint32_t i=0; i<ownershipOps.size(); i++ ) {
    const mace::EventOperationInfo& opInfo = ownershipOps[i];
    if( opInfo.opType == mace::EventOperationInfo::ADD_OWNERSHIP_OP && opInfo.toContextName == childContextName ) {
      parentContextName = opInfo.fromContextName;
      break;
    }
  }
  return parentContextName;
}



void mace::Event::null_func() {
  ADD_SELECTORS("Event::null_func");
  macedbg(1) << "Executed!" << Log::endl;
}

mace::OrderID mace::EventBroadcastInfo::newBroadcastEventID( const uint32_t contextId ) {
  mace::map<uint32_t, uint64_t>::iterator iter = broadcastIDRecord.find(contextId);

  if( iter == broadcastIDRecord.end() ) {
    mace::OrderID newId( contextId, 1 );
    broadcastIDRecord[contextId] = 2;
    return newId;
  } else {
    mace::OrderID newId( contextId, iter->second ++ );
    return newId;
  }
}

bool mace::Event::checkParentChildRelation(mace::string const& parentContextName, mace::string const& childContextName ) const {
  for(uint32_t i=0; i<ownershipOps.size(); i++) {
    const mace::EventOperationInfo& opInfo = ownershipOps[i];
    if( opInfo.opType == mace::EventOperationInfo::ADD_OWNERSHIP_OP && opInfo.fromContextName == parentContextName && 
        opInfo.toContextName == childContextName) {
      return true;
    }
  }
  return false;
}

void mace::Event::enqueueOwnershipOpInfo( EventOperationInfo const& opInfo ) {
  for( mace::vector<mace::EventOperationInfo>::iterator iter = ownershipOps.begin(); iter != ownershipOps.end(); iter ++) {
    const mace::EventOperationInfo& op_info = *iter;
    if( op_info.fromContextName == opInfo.fromContextName && op_info.toContextName == opInfo.toContextName ) {
      if( op_info.opType == opInfo.opType ) {
        return;
      } else {
        ownershipOps.erase(iter);
        return;
      }
    }
  }
  ownershipOps.push_back(opInfo);
}

void mace::EventExecutionInfo::serialize(std::string& str) const{
  mace::serialize( str, &fromContextNames );
  mace::serialize( str, &toContextNames );
  mace::serialize( str, &toContextNamesCopy );
  mace::serialize( str, &eventOpInfos );
  mace::serialize( str, &subEvents );
  mace::serialize( str, &deferredMessages );
  mace::serialize( str, &permitContexts );
  mace::serialize( str, &localLockRequests );
  mace::serialize( str, &lockedChildren );
  mace::serialize( str, &ownershipOps );

  mace::serialize( str, &currentTicket );
  mace::serialize( str, &newContextId );
  mace::serialize( str, &already_committed );

  mace::serialize( str, &createContextName );
  mace::serialize( str, &targetContextName );
  mace::serialize( str, &eventOpType );
}

int mace::EventExecutionInfo::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &fromContextNames );
  serializedByteSize += mace::deserialize( is, &toContextNames );
  serializedByteSize += mace::deserialize( is, &toContextNamesCopy );
  serializedByteSize += mace::deserialize( is, &eventOpInfos );
  serializedByteSize += mace::deserialize( is, &subEvents );
  serializedByteSize += mace::deserialize( is, &deferredMessages );
  serializedByteSize += mace::deserialize( is, &permitContexts );
  serializedByteSize += mace::deserialize( is, &localLockRequests );
  serializedByteSize += mace::deserialize( is, &lockedChildren );
  serializedByteSize += mace::deserialize( is, &ownershipOps );

  serializedByteSize += mace::deserialize( is, &currentTicket );
  serializedByteSize += mace::deserialize( is, &newContextId );
  serializedByteSize += mace::deserialize( is, &already_committed );
  
  serializedByteSize += mace::deserialize( is, &createContextName );
  serializedByteSize += mace::deserialize( is, &targetContextName );
  serializedByteSize += mace::deserialize( is, &eventOpType );
  return serializedByteSize;
}


void mace::EventExecutionInfo::addEventPermitContext( const mace::string& ctxName ) {
  if( permitContexts.count(ctxName) == 0 ) {
    permitContexts.insert(ctxName);
  }
}

bool mace::EventExecutionInfo::checkEventExecutePermitCache( const mace::string& ctxName ) {
  if(  permitContexts.count(ctxName) == 0 ) {
    return false;
  } else {
    return true;
  }
}

void mace::EventExecutionInfo::enqueueLocalLockRequest( const mace::EventOperationInfo& eventOpInfo ) {
  ADD_SELECTORS("EventExecutionInfo::enqueueLocalLockRequest");
  bool exist_flag = false;
  for( uint32_t i=0; i<localLockRequests.size(); i++ ) {
    if( localLockRequests[i] == eventOpInfo ){
      maceerr << "EventOp("<< eventOpInfo <<") already exists!" << Log::endl;
      exist_flag = true;
      ASSERT(false);
    }
  }
  if( !exist_flag ){
    localLockRequests.push_back(eventOpInfo);
    if( lockedChildren.count(eventOpInfo.toContextName) == 0 ) {
      lockedChildren.insert(eventOpInfo.toContextName);
    }
  }

  // if( toContextNames.count(eventOpInfo.toContextName) == 0 ) {
  //   toContextNames.insert( eventOpInfo.toContextName );
  // }

  // if( toContextNamesCopy.count(eventOpInfo.toContextName) == 0 ) {
  //   toContextNamesCopy.insert( eventOpInfo.toContextName );
  // }
}

void mace::EventExecutionInfo::addEventToContext( mace::string const& toContext ) {
  if( toContextNames.count(toContext) == 0 ){
    toContextNames.insert(toContext);
    toContextNamesCopy.insert(toContext);
  }

  if( lockedChildren.count(toContext) == 0 ) {
    lockedChildren.insert(toContext);
  }
}

void mace::EventExecutionInfo::addEventToContextCopy( mace::string const& toContext ) {
  if( toContextNamesCopy.count(toContext) == 0 ){
    toContextNamesCopy.insert(toContext);
  }
}

void mace::EventExecutionInfo::addEventFromContext( mace::string const& fromContext ) {
  if( fromContextNames.count(fromContext) == 0 ){
    fromContextNames.insert(fromContext);
  }
}

void mace::EventExecutionInfo::removeEventOp( EventOperationInfo const& opInfo ) {
  mace::vector<EventOperationInfo>::iterator iter = eventOpInfos.begin();
  for(; iter != eventOpInfos.end(); iter ++ ){
    if( *iter == opInfo ) {
      eventOpInfos.erase(iter);
      break;
    }
  }
}

mace::vector<mace::EventOperationInfo> mace::EventExecutionInfo::extractOwnershipOpInfos() {
  mace::vector<mace::EventOperationInfo> oops = ownershipOps;
  ownershipOps.clear();
  return oops;
}

void mace::EventExecutionInfo::enqueueOwnershipOpInfo( EventOperationInfo const& opInfo ){
  for( mace::vector<mace::EventOperationInfo>::iterator iter = ownershipOps.begin(); iter != ownershipOps.end(); iter ++) {
    const mace::EventOperationInfo& op_info = *iter;
    if( op_info.fromContextName == opInfo.fromContextName && op_info.toContextName == opInfo.toContextName ) {
      if( op_info.opType == opInfo.opType ) {
        return;
      } else {
        ownershipOps.erase(iter);
        return;
      }
    }
  }
  ownershipOps.push_back(opInfo);
}

bool mace::EventExecutionInfo::checkParentChildRelation(mace::string const& parentContextName, mace::string const& childContextName ) const {
  for(uint32_t i=0; i<ownershipOps.size(); i++) {
    const mace::EventOperationInfo& opInfo = ownershipOps[i];
    if( opInfo.opType == mace::EventOperationInfo::ADD_OWNERSHIP_OP && opInfo.fromContextName == parentContextName && 
        opInfo.toContextName == childContextName) {
      return true;
    }
  }
  return false;
}

mace::EventOperationInfo mace::EventExecutionInfo::getNewContextOwnershipOp( mace::string const& parentContextName, 
    mace::string const& childContextName ){

  mace::vector< mace::EventOperationInfo >::iterator iter = ownershipOps.begin();
  for(; iter != ownershipOps.end(); iter++) {
    const mace::EventOperationInfo& opInfo = *iter;
    if( opInfo.opType == mace::EventOperationInfo::ADD_OWNERSHIP_OP && opInfo.fromContextName == parentContextName && 
        opInfo.toContextName == childContextName) {
      break;
    }
  }

  if( iter != ownershipOps.end() ){
    mace::EventOperationInfo oop = *iter;
    ownershipOps.erase( iter );
    return oop;
  } else {
    mace::EventOperationInfo oop;
    return oop;
  }

}

void mace::EventExecutionInfo::eraseToContext( const mace::string& toContextName ){
  ADD_SELECTORS("EventExecutionInfo::eraseToContext");
  ASSERT( toContextNames.count(toContextName) > 0 );
  toContextNames.erase(toContextName);
  macedbg(1) << "To erase toContextName("<< toContextName <<"), toContextNames=" << toContextNames << Log::endl;
}

void mace::EventExecutionInfo::addExecutedContextNames( mace::set<mace::string> const& ctxNames ){
  ADD_SELECTORS("EventExecutionInfo::addExecutedContextNames");
  
  mace::set<mace::string>::const_iterator iter;
  for( iter=ctxNames.begin(); iter!=ctxNames.end(); iter++ ){
    if( toContextNamesCopy.count(*iter) == 0 ){
      toContextNamesCopy.insert(*iter);
    }
  }
}

bool mace::EventExecutionInfo::localUnlockContext( mace::EventOperationInfo const& eventOpInfo, 
    mace::vector<mace::EventOperationInfo> const& local_lock_requests, mace::vector<mace::string> const& locked_contexts) {

  ADD_SELECTORS("EventExecutionInfo::localUnlockContext");
  if( localLockRequests.size() == 0 ){
    return false;
  }

  bool exist_flag = false;
  // macedbg(1) << "To local unlock: " << eventOpInfo << Log::endl;

  mace::vector<mace::EventOperationInfo>::iterator rIter = localLockRequests.begin();
  for( ; rIter != localLockRequests.end(); rIter++ ){
    if( *rIter == eventOpInfo ){
      exist_flag = true;
      localLockRequests.erase(rIter);
      break;
    }
  }
  
  if( exist_flag ){
    mace::vector<mace::EventOperationInfo>::const_iterator cr_iter = local_lock_requests.begin();
    for(; cr_iter!=local_lock_requests.end(); cr_iter++ ){
      localLockRequests.push_back(*cr_iter);
      // macedbg(1) << "Add localLockRequest: " << *cr_iter << Log::endl;
    }

    for( uint32_t i=0; i<locked_contexts.size(); i++ ) {
      if( lockedChildren.count(locked_contexts[i]) == 0 ) {
        lockedChildren.insert(locked_contexts[i]);
        // macedbg(1) << "Add locallockedContext: " << locked_contexts[i] << Log::endl;
      }
    }
    return true;
  } else {
    return false;
  }
}

mace::vector<mace::string> mace::EventExecutionInfo::getLockedChildren() const {
  mace::vector<mace::string> locked_contexts;
  for( mace::set<mace::string>::const_iterator iter = lockedChildren.begin(); iter != lockedChildren.end(); iter ++ ) {
    locked_contexts.push_back(*iter);
  }
  return locked_contexts;
}

// void mace::EventExecutionInfo::computeLocalUnlockedContexts() {
//   ADD_SELECTORS("EventExecutionInfo::computeLocalUnlockedContexts");
//   for( uint32_t i=0; i<localLockRequests.size(); i++ ){
//     if( localUnlockedContexts.count(localLockRequests[i].toContextName) > 0 ){
//       localUnlockedContexts.erase(localLockRequests[i].toContextName);
//     }
//   }
//   macedbg(1) << "localUnlockedContexts=" << localUnlockedContexts << Log::endl;
// }