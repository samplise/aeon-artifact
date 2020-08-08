#include "ContextBaseClass.h"
#include "ContextDispatch.h"
#include "ScopedLock.h"
#include "ContextLock.h"
#include "ContextService.h"
#include <map>
using namespace mace;

const uint16_t DominatorRequest::CHECK_PERMISSION;
const uint16_t DominatorRequest::UNLOCK_CONTEXT;
const uint16_t DominatorRequest::RELEASE_CONTEXT;

const uint16_t LockRequest::VWLOCK;
const uint16_t LockRequest::VRLOCK;

template <class T>
  class ObjectPool{
  public:
    ObjectPool(){

    }
    ~ObjectPool(){
      clear();
    }
    void clear(){
      while( !objqueue.empty() ){
        objqueue.pop();
      }
    }
    void put( T* object ){
      ScopedLock sl( lock );
      //ASSERT( objqueue.push( object ) );
      objqueue.push( object );
    }
    T* get(){
      ScopedLock sl( lock );
      if( objqueue.empty() ){
        sl.unlock();
        T* newobj = new T;
        return newobj;
      }else{
        T* obj = objqueue.front();
        objqueue.pop();
        return obj;
      }
    }
  private:
    static pthread_mutex_t lock;
    std::queue< T*, std::list<T*> > objqueue;
  };
  ObjectPool< mace::Event > eventObjectPool;

void ContextEvent::fire() {
  ADD_SELECTORS("ContextEvent::fire");
  switch( type ){
    case TYPE_NULL:
      break;
    case TYPE_ASYNC_EVENT: {
      sv->executeAsyncEvent( static_cast< AsyncEvent_Message* >(param) );
      break;
    }
    case TYPE_BROADCAST_EVENT:
      sv->executeBroadcastEvent( static_cast< AsyncEvent_Message* >(param) ); 
      break;
    case TYPE_START_EVENT:
      sv->executeStartEvent( static_cast< AsyncEvent_Message* >(param) );
      break;
    case TYPE_GRAP_EVENT:
      sv->executeRoutineGrap( static_cast< Routine_Message* >(param), source );
      break;
    case TYPE_ROUTINE_EVENT: {
      ContextService* _service = static_cast< ContextService* >(sv);
      Routine_Message* routineMsg = static_cast< Routine_Message* >(param);
      mace::Event& event = routineMsg->getEvent();

      const mace::string& fromContextName = event.eventOpInfo.fromContextName;

      const mace::ContextMapping& snapshot = _service->getLatestContextMapping();
      mace::MaceAddr src;
      if( mace::ContextMapping::hasContext2(snapshot, fromContextName) == 0 ) {
        macedbg(1) << "Fail to find event("<< event.eventId <<")'s fromContextName " << fromContextName << Log::endl;
        uint32_t i = 0; 
        while(true) {
          _service->getUpdatedContextMapping(0);
          const mace::ContextMapping& newSnapshot = _service->getLatestContextMapping();
          if( mace::ContextMapping::hasContext2(newSnapshot, fromContextName) <= 0 ){
            maceerr << "Fail to find context("<< fromContextName <<") in context mapping!" << Log::endl;
          } else {
            src = mace::ContextMapping::getNodeByContext(newSnapshot, fromContextName);
            break;
          }
          if( i++ > 10 ){
            ASSERT( mace::ContextMapping::hasContext2(newSnapshot, fromContextName) > 0 );
          }
        }
      } else {
        src = mace::ContextMapping::getNodeByContext(snapshot, fromContextName);
      }

      sv->executeRoutine( static_cast< Routine_Message* >(param), src );
      break;
    }
    case TYPE_COMMIT_CONTEXT:{
      sv->executeCommitContext( static_cast< commit_single_context_Message* >(param) );
      delete param;
      break;
    }
    case TYPE_BROADCAST_COMMIT_CONTEXT: {
      sv->executeBroadcastCommitContext( static_cast<commit_single_context_Message*>(param) );
      delete param;
      break;
    }
  }
}


void ContextCommitEvent::serialize(std::string& str) const{
  mace::serialize( str, &eventId );
  mace::serialize( str, &isAsyncEvent );
}

int ContextCommitEvent::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &eventId );
  serializedByteSize += mace::deserialize( is, &isAsyncEvent );
  return serializedByteSize;
}

void ContextCommitEvent::print(std::ostream& out) const {
  out<< "ContextCommitEvent(";
  out<< "EventID="; mace::printItem(out, &eventId ); out<< ", ";
  out<< "isAsyncEvent="; mace::printItem(out, &isAsyncEvent );
  out<< ")";
}

void ContextCommitEvent::printNode(PrintNode& pr, const std::string& name) const {
  mace::PrintNode printer(name, "ContextCommitEvent" );
  mace::printItem( printer, "Event", &eventId );
  mace::printItem( printer, "isAsyncEvent", &isAsyncEvent );
  pr.addChild( printer );
}

void ContextEvent::serialize(std::string& str) const{
  mace::serialize( str, &sid );
  mace::serialize( str, &eventId );
  mace::serialize( str, &executeTicket);
  mace::serialize( str, &type);
  mace::serialize( str, param);
  mace::serialize( str, &source);
}

int ContextEvent::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &sid );
  serializedByteSize += mace::deserialize( is, &eventId );
  serializedByteSize += mace::deserialize( is, &executeTicket);
  serializedByteSize += mace::deserialize( is, &type);

  switch(type) {
    case TYPE_ASYNC_EVENT: {
      serializedByteSize += deserializeEvent(is);
      break;
    }
    case TYPE_ROUTINE_EVENT: {
      serializedByteSize += deserializeEvent(is);
      break;
    }
    case TYPE_BROADCAST_EVENT: {
      serializedByteSize += deserializeEvent(is);
      break;
    }
    case TYPE_GRAP_EVENT: {
      serializedByteSize += deserializeEvent(is);
      break;
    }
    case TYPE_START_EVENT: {
      serializedByteSize += deserializeEvent(is);
      break;
    }
    case TYPE_COMMIT_CONTEXT: {
      param = InternalMessageHelperPtr( new commit_single_context_Message() );
      serializedByteSize += param->deserialize(is);
      break;
    }
    case TYPE_BROADCAST_COMMIT: {
      param = InternalMessageHelperPtr( new BroadcastControl_Message() );
      serializedByteSize += param->deserialize(is);
      break;
    }
    case TYPE_BROADCAST_COMMIT_CONTEXT: {
      param = InternalMessageHelperPtr( new commit_single_context_Message() );
      serializedByteSize += param->deserialize(is);
      break;
    }
    default:{
      ASSERTMSG(false, "Unkown ContextEvent Type!");
    }
  }

  serializedByteSize += mace::deserialize( is, &source);
  return serializedByteSize;
}

int ContextEvent::deserializeEvent( std::istream& in ){
  BaseMaceService* serviceInstance = BaseMaceService::getInstance( sid );
  mace::Message* ptr;
  int count = serviceInstance->deserializeMethod( in, ptr );
  param = InternalMessageHelperPtr( static_cast< InternalMessageHelperPtr >( ptr ) );
  return count;
}

void ContextEvent::print(std::ostream& out) const {
  out<< "ContextEvent(";
  out<< "EventID="; mace::printItem(out, &eventId ); out <<",";
  out<< "EexecuteTicket="; mace::printItem(out, &executeTicket ); out <<",";
  out<< "Type="; mace::printItem(out, &type ); out <<",";
  out<< "Message="; mace::printItem(out, param ); out <<",";
  out<< "Source="; mace::printItem(out, &source ); 
  out<< ")";
}

void ContextEvent::printNode(PrintNode& pr, const std::string& name) const {
  mace::PrintNode printer(name, "ContextEvent" );
  mace::printItem( printer, "EventID", &eventId );
  mace::printItem( printer, "ExecuteTicket", &executeTicket );
  mace::printItem( printer, "Type", &type );
  mace::printItem( printer, "Message", param );
  mace::printItem( printer, "Source", &source );
  pr.addChild( printer );
}

ContextBaseClass::ContextBaseClass(const mace::string& contextName, const uint64_t createTicketNumber, 
      const uint64_t executeTicketNumber, const uint64_t create_now_committing_ticket, const mace::OrderID& execute_now_committing_eventId, 
      const uint64_t execute_now_committing_ticket, const mace::OrderID& now_serving_eventId, 
      const uint64_t now_serving_execute_ticket, const bool execute_serving_flag, const uint64_t lastWrite, 
      const uint8_t serviceId, const uint32_t contextId, const uint8_t contextType): 
    contextName(contextName),
    contextType( contextType ),
    serviceId( serviceId ),
    contextId( contextId ),
    create_now_committing_ticket( create_now_committing_ticket ),
    execute_now_committing_eventId( execute_now_committing_eventId ),
    execute_now_committing_ticket( execute_now_committing_ticket ),
    now_serving_eventId( now_serving_eventId ),
    now_serving_execute_ticket( now_serving_execute_ticket ),
    execute_serving_flag( execute_serving_flag ),
    lastWrite( lastWrite ),
    createTicketNumber( createTicketNumber ),
    executeTicketNumber( executeTicketNumber ),
    executedEventsCommitFlag( ),
    now_serving_create_ticket( 1 ),
    createWaitingFlag(false),
    createWaitingThread( ),
    handlingMessageNumber( 0 ),
    isWaitingForHandlingMessages( false ),
    handlingCreateEventNumber( 0 ),
    isWaitingForHandlingCreateEvents( false ),
    now_max_execute_ticket(0),
    eventExecutionInfos( ),
    migrationEventWaiting( false ),
    skipCreateTickets( ),
    eventCreateStartTime( ),
    committedEventCount( 0 ),
    totalEventTime( 0 ),
    eventExecuteStartTime( ),
    finishedEventCount( 0 ),
    totalEventExecuteTime( 0 ),
    pkey(),
#ifdef __APPLE__
#else
    keyOnce( PTHREAD_ONCE_INIT ),
#endif
    numReaders(0),
    numWriters(0),
    conditionVariables( ),
    commitConditionVariables( ),
    uncommittedEvents(0,-1)    
{
    contextTypeName = contextName;
    if( now_serving_eventId.ticket > 1 ){
        ADD_SELECTORS("ContextBaseClass::(constructor)");
        macedbg(1)<<"context '"<< contextName << "' id: "<< contextId << " is created at event "<< now_serving_eventId << Log::endl;
    }
#ifdef __APPLE__
	pthread_once_t x = PTHREAD_ONCE_INIT;
	keyOnce = x;
#endif

  pthread_mutex_init( &_context_ticketbooth, NULL );

	pthread_mutex_init( &createEventTicketMutex,  NULL );
  pthread_mutex_init( &executeEventTicketMutex, NULL );
  pthread_mutex_init( &executeEventMutex,  NULL );
  pthread_mutex_init( &createEventMutex,  NULL );
  pthread_mutex_init( &commitEventMutex,  NULL );
	
  pthread_mutex_init( &contextMigratingMutex, NULL);
  pthread_cond_init( &contextMigratingCond, NULL);
  
  pthread_mutex_init( &timeMutex, NULL );
  pthread_mutex_init( &executeTimeMutex, NULL );

  pthread_mutex_init( &eventExecutingSyncMutex, NULL );
  pthread_mutex_init( &contextEventOrderMutex, NULL );
  pthread_mutex_init( &releaseLockMutex, NULL );
  pthread_mutex_init( &contextDominatorMutex, NULL );
}
// FIXME: it will not delete context thread structure in other threads.
ContextBaseClass::~ContextBaseClass(){
  /* delete context specific thread pool */
  /*
  if( contextEventDispatcher != NULL ) {
    delete contextEventDispatcher;
  }
  */
    // delete thread specific memories
  pthread_once( & mace::ContextBaseClass::global_keyOnce, mace::ContextBaseClass::createKeyOncePerThread );
  ThreadSpecificMapType* t = (ThreadSpecificMapType *)pthread_getspecific(global_pkey);
  // FIXME: need to free all memories associated with pkey
  // this only releases the memory specific to this thread
  if( t == 0 ){
    //chuangw: this can happen if init() is never called on this context.
    // that is, this thread has never accessed this context.
  }else{
    ContextThreadSpecific* ctxts = (*t)[this];
    t->erase(this);
    delete ctxts;
  }

  pthread_mutex_destroy( &_context_ticketbooth );
  pthread_mutex_destroy( &createEventTicketMutex );
  pthread_mutex_destroy( &executeEventTicketMutex );
  pthread_mutex_destroy( &executeEventMutex );
  pthread_mutex_destroy( &createEventMutex );
  pthread_mutex_destroy( &commitEventMutex );
  
  pthread_mutex_destroy( &contextMigratingMutex );
  pthread_cond_destroy( &contextMigratingCond );
  
  pthread_mutex_destroy( &timeMutex);
  pthread_mutex_destroy( &executeTimeMutex);
  pthread_mutex_destroy( &eventExecutingSyncMutex );

  pthread_mutex_destroy( &contextEventOrderMutex );
  pthread_mutex_destroy( &releaseLockMutex );
  pthread_mutex_destroy( &contextDominatorMutex );

  // CommitEventQueueType::iterator iter = executeCommitEventQueue.begin();
  // for(; iter != executeCommitEventQueue.end(); iter++) {
  //   //putBackEventObject(iter->second->event);
  // } 
  executeCommitEventQueue.clear();
}

ContextThreadSpecific* ContextBaseClass::init(){
  pthread_once( & mace::ContextBaseClass::global_keyOnce, mace::ContextBaseClass::createKeyOncePerThread );
  ThreadSpecificMapType *t = (ThreadSpecificMapType *)pthread_getspecific(mace::ContextBaseClass::global_pkey);
  if (t == 0) {
    t = new mace::hash_map<ContextBaseClass*, ContextThreadSpecific*, SoftState>();
    assert( t != NULL );
    assert(pthread_setspecific(global_pkey, t) == 0);
  }
  ThreadSpecificMapType::iterator ctIterator = t->find(this);
  if( ctIterator == t->end() ){
      ContextThreadSpecific* ctxts = new ContextThreadSpecific();
      assert( ctxts != NULL );
      t->insert( std::pair< ContextBaseClass*, ContextThreadSpecific* >( this, ctxts ) );
      return ctxts;
  }
  return ctIterator->second;
}
void mace::ContextBaseClass::createKeyOncePerThread(){
    assert(pthread_key_create(&global_pkey, NULL) == 0);
}
void mace::ContextBaseClass::print(std::ostream& out) const {
  out<< "ContextBaseClass(";
  out<< "contextName="; mace::printItem(out, &(contextName) ); out<<", ";
  out<< "contextID="; mace::printItem(out, &(contextId) ); out<<", ";
  out<< "Create Ticket="; mace::printItem(out, &(createTicketNumber) ); out<<", ";
  out<< "Execute Ticket="; mace::printItem(out, &(executeTicketNumber) ); out<<", ";
  out<< "now_serving="; mace::printItem(out, &(now_serving_eventId) ); out<<", ";
  out<< "now_create_committing="; mace::printItem(out, &(create_now_committing_ticket) ); out<<", ";
  out<< "Execute Commit EventID="; mace::printItem(out, &(execute_now_committing_eventId) ); out<<", ";
  out<< "now_execute_committing="; mace::printItem(out, &(execute_now_committing_ticket) ); out<<", ";
  //out<< "lastWrite="; mace::printItem(out, &(lastWrite) ); out<<", ";
  out<< "numReaders="; mace::printItem(out, &(numReaders) ); out<<", ";
  out<< "numWriters="; mace::printItem(out, &(numWriters) ); out<<", ";
  out<< "uncommittedEvents="; mace::printItem(out, &(uncommittedEvents) );
  out<< ")";

} // print

void mace::ContextBaseClass::printNode(PrintNode& pr, const std::string& name) const {
  mace::PrintNode printer(name, "ContextBaseClass" );
  
  mace::printItem( printer, "contextName", &contextName );
  mace::printItem( printer, "contextID", &contextId );
  mace::printItem( printer, "Create Ticket", &createTicketNumber );
  mace::printItem( printer, "Execute Ticket", &executeTicketNumber );
  mace::printItem( printer, "now_serving", &now_serving_eventId );
  mace::printItem( printer, "now_create_committing", &create_now_committing_ticket );
  mace::printItem( printer, "Execute Commit EventID", &execute_now_committing_eventId );
  mace::printItem( printer, "now_execute_committing", &execute_now_committing_ticket );
  //mace::printItem( printer, "lastWrite", &lastWrite );
  mace::printItem( printer, "numReaders", &numReaders );
  mace::printItem( printer, "numWriters", &numWriters );
  mace::printItem( printer, "uncommittedEvents", &uncommittedEvents );
  pr.addChild( printer );
}

pthread_once_t mace::ContextBaseClass::global_keyOnce= PTHREAD_ONCE_INIT ;
pthread_key_t mace::ContextBaseClass::global_pkey;
pthread_mutex_t mace::ContextBaseClass::eventCommitMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mace::ContextBaseClass::eventSnapshotMutex = PTHREAD_MUTEX_INITIALIZER;
std::map< mace::OrderID, pthread_cond_t* > mace::ContextBaseClass::eventCommitConds;
std::map< mace::OrderID, pthread_cond_t* > mace::ContextBaseClass::eventSnapshotConds;
mace::snapshotStorageType mace::ContextBaseClass::eventSnapshotStorage;


void mace::ContextBaseClass::enqueueEvent(BaseMaceService* sv, AsyncEvent_Message* const msg) {
    ADD_SELECTORS("ContextBaseClass::enqueueEvent#1");
    mace::Event& event = msg->getEvent();
    int8_t eventType = event.eventType;
    macedbg(1) << "Enqueue an async event("<< event.eventId <<") to " << this->contextName << Log::endl;

    uint8_t ctxEventType = ContextEvent::TYPE_NULL;
    if(eventType == mace::Event::ASYNCEVENT) {
      ctxEventType = ContextEvent::TYPE_ASYNC_EVENT;
    } else if (eventType == mace::Event::BROADCASTEVENT) {
      ctxEventType = ContextEvent::TYPE_BROADCAST_EVENT;
    } 

    if (ctxEventType == ContextEvent::TYPE_NULL) {
      ASSERTMSG(false, "Invalid async event type!");
    }
	
    OrderID const& eventId = event.eventId;
    __EventStorageStruct__ eventStorage(msg, ctxEventType, mace::InternalMessage::ASYNC_EVENT, serviceId, event.eventOpInfo);

    ScopedLock sl(this->executeEventMutex);

    waitingEvents[eventId].push_back(eventStorage);
    eventStorage.msg = NULL;
    sl.unlock();

    ScopedLock order_sl(this->contextEventOrderMutex);
    event.eventOpInfo.requireContextName = this->contextName;
    mace::set<mace::string> permitContexts;
    if( this->checkEventExecutePermission(sv, event.eventOpInfo, true, permitContexts) ){
      macedbg(1) << "Event("<< eventId <<") could execute directly in " << this->contextName << Log::endl;
      event.eventOpInfo.permitContexts.clear();
      sl.lock();
      this->enqueueContextEvent(sv, eventId);
    } else {
      event.eventOpInfo.permitContexts.clear();
    }
}


// For local transition. At this moment, it should obtain its execute ticket and wait for execution
bool mace::ContextBaseClass::enqueueEvent(const BaseMaceService* sv, mace::Event& event) {
    ADD_SELECTORS("ContextBaseClass::enqueueEvent#2");
    
    int8_t eventType = event.eventType;
    uint8_t ctxEventType = ContextEvent::TYPE_NULL;
    if(eventType == mace::Event::STARTEVENT) {
      ctxEventType = ContextEvent::TYPE_FIRST_EVENT;
    } else if(eventType == mace::Event::ENDEVENT) {
      ctxEventType = ContextEvent::TYPE_LAST_EVENT;
    }

    ASSERT( ctxEventType != ContextEvent::TYPE_NULL );
  
    BaseMaceService* nc_sv = const_cast<BaseMaceService*>(sv);
    event.eventOpInfo.requireContextName = this->contextName;
    mace::set<mace::string> permitContexts;
    ASSERTMSG( this->checkEventExecutePermission(nc_sv, event.eventOpInfo, false, permitContexts), "The first event shouldn't wait for any previous event!");
    ScopedLock sl(this->executeEventMutex);
    this->enqueueContextEvent(sv, event);
    return true;
}

void mace::ContextBaseClass::enqueueRoutine(BaseMaceService* sv, Routine_Message* const msg, mace::MaceAddr const& source ) {
    ADD_SELECTORS("ContextBaseClass::enqueueRoutine");
    mace::Event & event = msg->getEvent();
    
    macedbg(1) << "Enqueue an Routine Event("<< event.eventId <<") to " << this->contextName << Log::endl;
    ASSERT( this->contextName == event.eventOpInfo.toContextName );

    uint8_t ctxEventType = ContextEvent::TYPE_ROUTINE_EVENT;
      
    OrderID const& eventId = event.eventId;
    __EventStorageStruct__ eventStorage( msg, ctxEventType, mace::InternalMessage::ROUTINE, serviceId, event.eventOpInfo );

    ScopedLock sl(this->executeEventMutex);
    waitingEvents[eventId].push_back(eventStorage);
    eventStorage.msg = NULL;
    sl.unlock();

    ScopedLock order_sl(this->contextEventOrderMutex);
    event.eventOpInfo.requireContextName = this->contextName;
    mace::set<mace::string> permitContexts;
    if( this->checkEventExecutePermission(sv, event.eventOpInfo, true, permitContexts) ){
      macedbg(1) << "Routine Event("<< event.eventId <<") could directly execute in " << this->contextName << Log::endl;
      sl.lock();
      this->enqueueContextEvent(sv, eventId);
    }
}

void mace::ContextBaseClass::commitContext(BaseMaceService* sv, OrderID const& eventId) {
    ADD_SELECTORS("ContextBaseClass::commitContext#2");
    ScopedLock sl(executeEventMutex);
    

    ContextService* _service = static_cast<ContextService*>(sv);
    //ContextStructure const& cs = _service->contextStructure;
    uint64_t executeTicket = contextEventOrder.getExecuteEventTicket(eventId);

    ASSERT( executeTicket > 0);
    macedbg(1) << "Event(" << eventId << ") is holding context " << contextName <<" executeTicket="<< executeTicket <<" now_serving_execute_ticket=" << now_serving_execute_ticket << Log::endl;
      
    //this->executedEvent(sv, eventId);
    ASSERT( executeTicket == this->now_serving_execute_ticket );
    
    _service->__beginCommitContext(this->contextId);
    _service->__finishCommitContext(this);
}

void mace::ContextBaseClass::initialize(const mace::string& contextName, const OrderID& now_serving_eventId, const uint8_t serviceId, 
      const uint32_t contextId, const ContextEventOrder& contextEventOrder, const uint64_t create_now_committing_ticket, const OrderID& execute_now_committing_eventId,
      const uint64_t execute_now_committing_ticket ){
  ADD_SELECTORS("ContextBaseClass::initialize");
  //macedbg(1) << "Change contextName to " << contextName << Log::endl;

  this->contextName = contextName;
  this->now_serving_eventId = now_serving_eventId;
  this->contextEventOrder = contextEventOrder;
  this->create_now_committing_ticket = create_now_committing_ticket;
  this->execute_now_committing_eventId = execute_now_committing_eventId;
  this->execute_now_committing_ticket = execute_now_committing_ticket;
  this->serviceId = serviceId;
  this->contextId = contextId;
  
  this->contextEventOrder.setContextName(contextName);
  this->runtimeInfo.setContextName(contextName);
}

void mace::ContextBaseClass::initialize2(const mace::string& contextName, const uint8_t serviceId, const uint32_t contextId, 
      const uint64_t create_now_committing_ticket, const uint64_t execute_now_committing_ticket, 
      ContextStructure& ctxStructure ){
  ADD_SELECTORS("ContextBaseClass::initialize2");
  
  this->contextName = contextName;
  this->create_now_committing_ticket = create_now_committing_ticket;
  this->execute_now_committing_ticket = execute_now_committing_ticket;
  this->serviceId = serviceId;
  this->contextId = contextId;
  
  this->contextEventOrder.setContextName(contextName);
  this->runtimeInfo.setContextName(contextName);

  mace::vector<mace::string> dominateContexts = ctxStructure.getDominateContexts(contextName);
  mace::string domContext = ctxStructure.getUpperBoundContextName(contextName);
  uint64_t ver = ctxStructure.getCurrentVersion();
  this->dominator.initialize(contextName, domContext, ver, dominateContexts);
}

void mace::ContextBaseClass::lock(  ){
  mace::ContextLock cl( *this, ThreadStructure::myEventID(), false, mace::ContextLock::WRITE_MODE );
}
void mace::ContextBaseClass::downgrade( int8_t requestedMode ){
  mace::ContextLock cl( *this, ThreadStructure::myEventID(), false, requestedMode );
}

void mace::ContextBaseClass::unlock(  ){
  mace::ContextLock cl( *this, ThreadStructure::myEventID(), false, mace::ContextLock::NONE_MODE );
}
void mace::ContextBaseClass::nullTicket(){
  mace::ContextLock cl( *this, ThreadStructure::myEventID(), false, mace::ContextLock::NONE_MODE );
}

void mace::ContextBaseClass::enqueueCommitEventQueue(BaseMaceService* sv, const mace::OrderID& eventId, bool isAsyncEvent) {
  ADD_SELECTORS("ContextBaseClass::enqueueCommitEventQueue");
  //this->notifyWaitingContexts(eventId);
  ScopedLock sl(commitEventMutex);
  ContextCommitEvent* commitEvent = new ContextCommitEvent(sv, eventId, this, isAsyncEvent);
  uint64_t executeTicket = contextEventOrder.getExecuteEventTicket(eventId);
  ASSERTMSG( executeTicket > 0, "enqueueCommitEventQueue" );
  executeCommitEventQueue[executeTicket] = commitEvent;
  if( executeTicket > execute_now_committing_ticket ){
    macedbg(1) << "Put event(" << eventId << ") into Context("<< contextName <<") execute commit queue. executeTicket=" << executeTicket <<" execute_now_committing_ticket="<< execute_now_committing_ticket <<"("<< contextEventOrder.getExecuteEventOrderID(execute_now_committing_ticket) <<")" << Log::endl;
  }
  sl.unlock();
  enqueueReadyCommitEventQueue();
}

void mace::ContextBaseClass::enqueueCreateEvent(AsyncEventReceiver* sv, HeadEventDispatch::eventfunc func, mace::Message* p, bool useTicket) {
  ADD_SELECTORS("ContextBaseClass::enqueueCreateEvent");
  OrderID myEventId;
    
  if( useTicket ){
    myEventId = newCreateTicket();
  }else{
    myEventId = ThreadStructure::myEventID();
  }
  HeadEventDispatch::HeadEvent thisev (sv, func, p, myEventId, this->contextName);
  ScopedLock sl(createEventMutex);
  createEventQueue.push(thisev);
  enqueueReadyCreateEventQueue();

  thisev.param = NULL;
}  

OrderID mace::ContextBaseClass::newCreateTicket() {
  ADD_SELECTORS("ContextBaseClass::newCreateTicket");

  ScopedLock sl(this->createEventTicketMutex);
  OrderID eventId(this->contextId, this->createTicketNumber);
  macedbg(1) << "Context "<< this->contextId <<" sold create ticket "<< this->createTicketNumber << Log::endl;
  this->createTicketNumber ++;
  return eventId;
}

void mace::ContextBaseClass::tryCreateWakeup() {
  ADD_SELECTORS("ContextBaseClass::tryCreateWakeup");
  //contextEventDispatcher->signalCreateThread();
}

// It should be locked via execute
void mace::ContextBaseClass::enqueueContextEvent(BaseMaceService* sv, const OrderID& eventId ) {
  ADD_SELECTORS("ContextBaseClass::enqueueContextEvent");
  
  EventStorageType::iterator a_iter = waitingEvents.find(eventId);
  if( a_iter == waitingEvents.end() ) {
    maceerr << "Something wrong! Fail to find event(" << eventId << ") in context("<< this->contextName <<")!" << Log::endl;
    //ASSERT(false);
    return;
  }

  mace::vector<__EventStorageStruct__>& eventStorages = a_iter->second;

  mace::vector<__EventStorageStruct__> restEventStorage;
  for(mace::vector<__EventStorageStruct__>::iterator sIter = eventStorages.begin(); sIter != eventStorages.end(); sIter++ ){
    __EventStorageStruct__& eventStorage = *sIter;

    if( eventStorage.eventOpInfo.toContextName != contextName ) {
      restEventStorage.push_back(eventStorage);
      eventStorage.msg = NULL;
      continue;
    }
    if( eventStorage.contextEventType == ContextEvent::TYPE_ASYNC_EVENT ) {
      macedbg(1) << "Event("<< eventId <<") is an async event for "<< contextName << Log::endl;
      AsyncEvent_Message* msg = static_cast<AsyncEvent_Message*>(eventStorage.msg);
      mace::Event& event = msg->getEvent();
            
      ContextEvent ce(event.eventId, sv, eventStorage.contextEventType, eventStorage.msg, serviceId, this);
      enqueueExecuteContextEvent(sv, ce); 
      ce.param = NULL;
    } else if( eventStorage.contextEventType == ContextEvent::TYPE_BROADCAST_EVENT){
      macedbg(1) << "Event("<< eventId <<") is a broadcast event for "<< contextName << Log::endl;
      AsyncEvent_Message* msg = static_cast<AsyncEvent_Message*>(eventStorage.msg);
      mace::Event& event = msg->getEvent();
      uint64_t executeTicket = contextEventOrder.getExecuteEventTicket(event.eventId);
      if( executeTicket != 0 ) {
        ASSERTMSG( executeTicket <= this->now_serving_execute_ticket, "This event must still hold the context!");
        ContextEvent ce(event.eventId, sv, eventStorage.contextEventType, eventStorage.msg, serviceId, this);
        enqueueExecuteContextEventToHead(sv, ce);
        ce.param = NULL;
      } else {
        ContextEvent ce(event.eventId, sv, eventStorage.contextEventType, eventStorage.msg, serviceId, this);
        enqueueExecuteContextEvent(sv, ce);
        ce.param = NULL;
      }
    } else if( eventStorage.contextEventType == ContextEvent::TYPE_ROUTINE_EVENT ){
      Routine_Message* msg = static_cast<Routine_Message*>(eventStorage.msg);
      mace::Event& event = msg->getEvent();
      uint64_t executeTicket = contextEventOrder.getExecuteEventTicket(event.eventId);
      if( executeTicket != 0 ) {
        //macedbg(1) << "Broadcast Event("<< event.eventId <<")'s execute ticket in Context("<< contextName <<") is " << executeTicket << Log::endl;
        ASSERTMSG( executeTicket <= now_serving_execute_ticket, "This event must still hold the context!");
        ContextEvent ce(event.eventId, sv, eventStorage.contextEventType, eventStorage.msg, serviceId, this);
        enqueueExecuteContextEventToHead(sv, ce);
        ce.param = NULL;
      } else {
        ContextEvent ce(event.eventId, sv, eventStorage.contextEventType, eventStorage.msg, serviceId, this);
        enqueueExecuteContextEvent(sv, ce);
        ce.param = NULL;
      }
    }
  
    eventStorage.msg = NULL;
  }
  waitingEvents.erase(a_iter);

  if( restEventStorage.size() > 0 ){
    waitingEvents[eventId] = restEventStorage;
  }
}


void mace::ContextBaseClass::enqueueContextEvent( const BaseMaceService* sv, mace::Event& event ) {
  ADD_SELECTORS("ContextBaseClass::enqueueContextEvent#2");
  contextEventOrder.addExecuteEvent(event.eventId);
}

void mace::ContextBaseClass::forwardWaitingBroadcastRequset(BaseMaceService* sv, const EventOperationInfo& eventOpInfo ) {
  ADD_SELECTORS("ContextBaseClass::forwardWaitingBroadcastRequset");
  
  const mace::OrderID& eventId = eventOpInfo.eventId;
  EventStorageType::iterator a_iter = waitingEvents.find(eventId);
  if( a_iter == waitingEvents.end() ) {
    maceerr << "Something wrong! Fail to find event(" << eventOpInfo << ")'s broadcast event!" << Log::endl;
    ASSERT(false);
  }

  ContextService* _service = static_cast<ContextService*>(sv);
  mace::vector<__EventStorageStruct__>& eventStorages = a_iter->second;

  mace::vector<__EventStorageStruct__> restEventStorage;
  for(mace::vector<__EventStorageStruct__>::iterator sIter = eventStorages.begin(); sIter != eventStorages.end(); sIter++ ){
    __EventStorageStruct__& eventStorage = *sIter;

    if( eventStorage.eventOpInfo == eventOpInfo ) {
      // this->addChildEventOp(eventOpInfo, _service->contextStructure);
      _service->broadcastHead(eventStorage.msg);
      eventStorage.msg = NULL;
      eventStorages.erase(sIter);
      break;
    }
  }
  if( eventStorages.size() == 0 ){
    waitingEvents.erase(a_iter);
  }
}

void mace::ContextBaseClass::setCurrentEventOp( const mace::EventOperationInfo& eop ) {
  ScopedLock sl( this->eventExecutingSyncMutex );
  this->currOwnershipEventOp = eop;
}

bool mace::ContextBaseClass::checkBroadcastRequestExecutePermission( BaseMaceService* sv, const mace::EventOperationInfo& eventOpInfo, mace::AsyncEvent_Message* reqObj) {
  ADD_SELECTORS("ContextBaseClass::checkBroadcastRequestExecutePermission");
  ContextService* _service = static_cast<ContextService*>(sv);
  ContextStructure& contextStructure = _service->contextStructure;
  const mace::string dominator = contextStructure.getUpperBoundContextName( this->contextName );
  
  ScopedLock order_sl(this->contextEventOrderMutex);
  mace::set<mace::string> permitContexts;
  if( this->checkEventExecutePermission(sv, eventOpInfo, false, permitContexts) ){
    macedbg(1) << "In context("<< this->contextName <<"): " << eventOpInfo << " could be forwarded directly!" << Log::endl;
    return true;
  }
  
  ScopedLock sl(this->executeEventMutex);
  macedbg(1) << "In dominator("<< this->contextName <<"): " << eventOpInfo << " should be enqueue until getting the permission!" << Log::endl;
  uint8_t ctxEventType = ContextEvent::TYPE_BROADCAST_EVENT;
  OrderID const& eventId = eventOpInfo.eventId;
  __EventStorageStruct__ eventStorage(reqObj, ctxEventType, mace::InternalMessage::ASYNC_EVENT, this->serviceId, eventOpInfo);
 
  waitingEvents[eventId].push_back(eventStorage);
  eventStorage.msg = NULL;
  return false;
}

void mace::ContextBaseClass::initializeDominator( const mace::string& contextName, const mace::string& domCtx, const uint64_t& ver,
    const mace::vector<mace::string>& dominateCtxs ) {
  dominator.initialize(contextName, domCtx, ver, dominateCtxs );
}

// void mace::ContextBaseClass::handleWaitingDominatorRequests(BaseMaceService* sv, mace::vector<DominatorRequest> const& dominatorRequests) {
//   ADD_SELECTORS("ContextBaseClass::handleWaitingDominatorRequests");

//   mace::vector<mace::EventOperationInfo> local_lock_requests;
//   mace::vector<mace::string> local_require_contexts;

//   ContextService* _service = static_cast<ContextService*>(sv);
//   for( uint32_t i=0; i<dominatorRequests.size(); i++ ){
//     const DominatorRequest& request = dominatorRequests[i];
//     if( request.lockType == mace::DominatorRequest::CHECK_PERMISSION ){
//       mace::set<mace::string> p_ctx_names;
//       if( this->checkEventExecutePermission(sv, request.eventOpInfo, false, p_ctx_names ) ){
//         mace::vector<mace::EventOperationInfo> eventOpInfos;
//         mace::vector<mace::string> contextNames;

//         eventOpInfos.push_back(request.eventOpInfo);
//         for( mace::set<mace::string>::iterator iter = p_ctx_names.begin(); iter != p_ctx_names.end(); iter++ ){
//           contextNames.push_back(*iter);
//         }
//         _service->send__event_replyEventExecutePermission( request.ctxName, this->contextName, request.eventOpInfo.eventId, 
//           contextNames, eventOpInfos );
//       }
//     } else if( request.lockType == mace::DominatorRequest::UNLOCK_CONTEXT ) {
//       this->unlockContext(sv, request.eventOpInfo, request.ctxName, local_lock_requests, local_require_contexts );
//     } else if( request.lockType == mace::DominatorRequest::RELEASE_CONTEXT ){
//       this->releaseContext(sv, request.eventId, request.ctxName, local_lock_requests, local_require_contexts );
//     } else {
//       ASSERT(false);
//     }
//   }
// }

void ContextBaseClass::createEvent(BaseMaceService* sv, mace::OrderID& myEventId, mace::Event& event, 
    const mace::string& targetContextName, const mace::string& methodType, const int8_t eventType, const uint8_t& eventOpType) {
  ADD_SELECTORS("ContextBaseClass::createEvent");
  //macedbg(1) << "Context("<< contextName <<") creates event which target context " << targetContextName << Log::endl;
  //OrderID myEventId = newCreateTicket();

  ScopedLock sl(createEventMutex);
  macedbg(1) << "now_serving_create_ticket=" << now_serving_create_ticket << " create_ticket=" << myEventId.ticket << Log::endl;
  if( myEventId.ticket != now_serving_create_ticket ) {
    pthread_cond_t cond;
    pthread_cond_init( &cond, NULL );
    createWaitingThread[ myEventId.ticket ] = &cond;
    createWaitingFlag = true;
    pthread_cond_wait( &cond, &createEventMutex );

    createWaitingThread[ myEventId.ticket ] = NULL;
    createWaitingThread.erase(myEventId.ticket);
    pthread_cond_destroy( &cond );

    if( createWaitingThread.size() == 0 ) {
      createWaitingFlag = false;
    }
  }

  ASSERTMSG( myEventId.ticket == now_serving_create_ticket, "ContextBaseClass::createEvent");
  //macedbg(1) << "Now create event for ticket " << myEventId << Log::endl;
  sl.unlock();
  
  uint64_t ver = this->getContextMappingVersion();
  uint64_t cver = this->getContextStructureVersion();
  //if( !isTestFlag ) {
  event.initialize2(myEventId, eventOpType, this->contextName, targetContextName, methodType, eventType, ver, cver);
  
  //}
  event.addServiceID(serviceId);
  mace::EventOperationInfo opInfo(myEventId, mace::EventOperationInfo::ASYNC_OP, targetContextName, targetContextName, 1, 
    event.eventOpType);
  event.eventOpInfo = opInfo;
  //macedbg(1) << "EventId = " << myEventId <<" targetContext=" << targetContextName << " startContext=" << start_ctx_name << " accessContexts="<< accessCtxs<<" preEventIds=" << preCreateEventIds << Log::endl;
  
  sl.lock();
  now_serving_create_ticket ++;
  macedbg(1) << "Increasing Context("<< contextName<<")'s now_serving_create_ticket to be " << now_serving_create_ticket << Log::endl;
  enqueueReadyCreateEventQueue();
  sl.unlock();
  signalCreateThread();
}

bool ContextBaseClass::waitingForExecution(const BaseMaceService* sv, mace::Event& event) {
  ADD_SELECTORS("ContextBaseClass::waitingForExecution");
  macedbg(1) << "Enter ContextBaseClass::waitingForExecution!" << Log::endl;
  bool execute_flag = enqueueEvent(sv, event);
  return execute_flag;
}

void ContextBaseClass::commitEvent(const mace::OrderID& eventId) {
  ADD_SELECTORS("ContextBaseClass::commitEvent");
  ScopedLock sl_commit(_context_ticketbooth);
  if( readerEvents.count(eventId) > 0 || writerEvents.count(eventId) > 0 ){
    maceerr << "Event("<< eventId <<") still hold the lock on " << this->contextName << Log::endl;
    ASSERT(false);
  }
  sl_commit.unlock();

  ScopedLock sl(commitEventMutex);
  
  const uint64_t execute_ticket = contextEventOrder.getExecuteEventTicket( eventId );
  if( execute_ticket == 0 ){
    maceerr << "Event("<< eventId <<") is not found in Context("<< this->contextName <<")!" << Log::endl;
    return;
  }

  macedbg(1) << "To commit event("<< eventId <<") in " << this->contextName << Log::endl;
 
  markExecuteEventCommitted(eventId);
  contextEventOrder.removeEventExecuteTicket(eventId);
  // committedEventIds.push_back(eventId);
  sl.unlock();
  
  this->deleteEventExecutionInfo(eventId);
  (this->runtimeInfo).commitEvent(eventId);
}

void ContextBaseClass::putBackEventObject(mace::Event* event) {
  //eventObjectPool.put(event);
  delete event;
}

void ContextBaseClass::enqueueSubEvents(mace::OrderID const& eventId) {
  ADD_SELECTORS("ContextBaseClass::enqueueSubEvents");
  mace::vector<mace::EventRequestWrapper> subevents = getSubEvents(eventId);
  if( !subevents.empty() ) {
    AsyncEventReceiver* sv = BaseMaceService::getInstance(this->serviceId);
    for( uint32_t i=0; i<subevents.size(); i++ ) {
      this->enqueueCreateEvent(sv, (HeadEventDispatch::eventfunc)&BaseMaceService::createEvent, subevents[i].request, true ); 
    }
    macedbg(1) << "Enqueue Event(" << eventId <<")'s subevents("<< subevents.size() <<") into waiting queue!" << Log::endl;
  } else {
    macedbg(1) << "Event(" << eventId <<") has no subevents!" << Log::endl;
  }
}


void ContextBaseClass::enqueueDeferredMessages( mace::OrderID const& eventId ) {
  ADD_SELECTORS("ContextBaseClass::enqueueDeferredMessages");
  mace::vector<mace::EventMessageRecord> messages = getExternalMessages(eventId);
  
  if( !messages.empty() ){
    ThreadStructure::ScopedContextID sc( ContextMapping::getHeadContextID() );
    for( mace::vector<mace::EventMessageRecord>::const_iterator msgIt = messages.begin(); msgIt != messages.end(); msgIt++ ){
      BaseMaceService* serviceInstance = BaseMaceService::getInstance( msgIt->sid );
      macedbg(1) << "In context("<< this->contextName <<"): Dispatch a message!" << Log::endl;
      serviceInstance->dispatchDeferredMessages( msgIt->dest, msgIt->message, msgIt->rid );
    }
    //macedbg(1) << "Dispatch event(" << eventId <<")'s messages("<< messages.size() <<")!" << Log::endl;
  } else {
    //macedbg(1) << "Event(" << eventId <<") has no messages!" << Log::endl;
  }
}

void ContextBaseClass::markExecuteEventCommitted(const mace::OrderID& eventId) {
  ADD_SELECTORS("ContextBaseClass::markExecuteEventCommitted");
  macedbg(1) << "Mark event("<< eventId <<") as committed in " << this->contextName;
  uint64_t executeTicket = getExecuteEventTicket(eventId);
  ASSERT( executeTicket != 0 );
  executedEventsCommitFlag[executeTicket] = true;
  macedbg(1) << "Mark event(" << eventId <<") in context "<< contextName <<" as committed. executeTicket="<< executeTicket<< " execute_now_committing_ticket=" 
    << execute_now_committing_ticket <<" executedEventsCommitFlag="<< executedEventsCommitFlag << Log::endl;
  while(true) {
    mace::map< uint64_t, bool >::iterator iter = executedEventsCommitFlag.find(execute_now_committing_ticket);
    if( iter == executedEventsCommitFlag.end() || !iter->second ) {
      break;
    } else {
      execute_now_committing_ticket ++;
      macedbg(1) << "Context("<< contextName <<") increases execute_now_committing_ticket to " << execute_now_committing_ticket << Log::endl;
      executedEventsCommitFlag.erase(iter);
    }
  }

  
  if( migrationEventWaiting ) {
    ContextLock::notifyMigrationEvent(*this);
  }
}

void ContextBaseClass::sendCommitDoneMsg(const mace::Event& event) {
  AsyncEventReceiver* sv = BaseMaceService::getInstance(this->serviceId);
  ContextService* _service = static_cast<ContextService*>(sv);

  mace::set<mace::string> accessed_contexts;
  _service->send__event_CommitDoneMsg(event.create_ctx_name, event.target_ctx_name, event.eventId, accessed_contexts);
}

void ContextBaseClass::signalCreateThread() {
  ADD_SELECTORS("ContextBaseClass::signalCreateThread");
  ScopedLock sl(createEventMutex);
  if( createWaitingFlag ) {
    std::map<uint64_t, pthread_cond_t*>::iterator iter = createWaitingThread.find(now_serving_create_ticket);
    if( iter != createWaitingThread.end() ) {
      pthread_cond_signal(iter->second);
      macedbg(1) << "Context("<< contextName <<") signals waiting create ticket " << now_serving_create_ticket << Log::endl;
    }
  }
}

uint64_t ContextBaseClass::getContextMappingVersion() const {
  BaseMaceService* sv = BaseMaceService::getInstance(this->serviceId);
  ContextService* _service = static_cast<ContextService*>(sv);

  uint64_t ver = _service->getContextMapping().getCurrentVersion();
  return ver; 
}

uint64_t ContextBaseClass::getContextStructureVersion() const {
  BaseMaceService* sv = BaseMaceService::getInstance(this->serviceId);
  ContextService* _service = static_cast<ContextService*>(sv);

  uint64_t ver = _service->contextStructure.getCurrentVersion();
  return ver; 
}

uint64_t ContextBaseClass::requireExecuteTicket( mace::OrderID const& eventId) {
  ScopedLock sl(executeEventMutex);
  uint64_t executeTicket = contextEventOrder.addExecuteEvent(eventId);
  return executeTicket; 
}

void ContextBaseClass::waitForEventExecutePermission(BaseMaceService *sv, mace::EventOperationInfo const& eventOpInfo) {
  ADD_SELECTORS("ContextBaseClass::waitForEventExecutePermission");
  macedbg(1) << "Check event("<< eventOpInfo.eventId <<") permit to access context("<< eventOpInfo.toContextName <<") from context("<< eventOpInfo.fromContextName <<")!" << Log::endl;
    
  ScopedLock order_sl(this->contextEventOrderMutex);
  mace::set<mace::string> permitContexts;
  if( this->checkEventExecutePermission(sv, eventOpInfo, false, permitContexts ) ) {
    macedbg(1) << "Event("<< eventOpInfo.eventId <<") could access context("<< eventOpInfo.toContextName <<") from context("<< eventOpInfo.fromContextName <<")!" << Log::endl;
    return;
  }

  ScopedLock sl(eventExecutingSyncMutex);

  order_sl.unlock();

  pthread_cond_t cond;
  pthread_cond_init( &cond, NULL );
  eventExecutingSyncConds[eventOpInfo] = &cond;
  pthread_cond_wait( &cond, &eventExecutingSyncMutex );
  eventExecutingSyncConds[eventOpInfo] = NULL;
  eventExecutingSyncConds.erase(eventOpInfo);
  pthread_cond_destroy(&cond);

}

void ContextBaseClass::prepareHalt() {
  ADD_SELECTORS("ContextBaseClass::prepareHalt");
  this->waitingForMessagesDone();
  macedbg(1) << "After waitingForMessagesDone!!" << Log::endl;
  this->waitingForCreateEventsDone();
}

void ContextBaseClass::resumeExecution() {
  ADD_SELECTORS("ContextBaseClass::resumeExecution");
  macedbg(1) << "context("<< this->contextName <<") executeEventQueue=" << executeEventQueue.size() << " createEventQueue=" << createEventQueue.size() << Log::endl;

  enqueueReadyExecuteEventQueueWithLock();
  enqueueReadyCreateEventQueueWithLock();
}

void ContextBaseClass::snapshot(std::string& str) const{
  mace::serialize(str, &contextName); ///< The canonical name of the context
  mace::serialize(str, &contextTypeName);
  
  mace::serialize(str, &serviceId); ///< The service in which the context belongs to
  mace::serialize(str, &contextId);
  
  mace::serialize(str, &waitingEvents);

  mace::serialize(str, &executeEventQueue);

  mace::serialize(str, &create_now_committing_ticket);
  mace::serialize(str, &execute_now_committing_eventId);
  mace::serialize(str, &execute_now_committing_ticket);

  mace::serialize(str, &now_serving_eventId);
  mace::serialize(str, &now_serving_execute_ticket);
  
  mace::serialize(str, &lastWrite);

  mace::serialize(str, &contextEventOrder);
  mace::serialize(str, &dominator);

  mace::serialize(str, &runtimeInfo);

  mace::serialize(str, &createTicketNumber);
  mace::serialize(str, &executeTicketNumber);

  mace::serialize(str, &executedEventsCommitFlag);
  mace::serialize(str, &now_serving_create_ticket);
  mace::serialize(str, &now_max_execute_ticket);
  
  mace::serialize(str, &eventExecutionInfos);

  mace::serialize(str, &readerEvents);
  mace::serialize(str, &writerEvents);
  mace::serialize(str, &migrationEventWaiting);
  mace::serialize(str, &skipCreateTickets );
}

void ContextBaseClass::resumeParams( BaseMaceService* sv, const ContextBaseClassParams* params ) {
  ADD_SELECTORS("ContextBaseClass::resumeParams");
  macedbg(1) << "context=" << params->contextName << Log::endl;

  this->contextName = params->contextName; ///< The canonical name of the context
  this->contextTypeName = params->contextTypeName;
  this->serviceId = params->serviceId; ///< The service in which the context belongs to
  this->contextId = params->contextId;

  ContextBaseClassParams* nonContextParams = const_cast<ContextBaseClassParams*>(params);
  mace::vector< mace::__CreateEventStorage__>::iterator createIter = nonContextParams->createEventQueue.begin();
  for(; createIter != nonContextParams->createEventQueue.end(); createIter++ ) {
    mace::__CreateEventStorage__& mcreateEvent = *createIter;
    HeadEventDispatch::HeadEvent thisev (sv, (HeadEventDispatch::eventfunc)&BaseMaceService::createEvent, mcreateEvent.msg, mcreateEvent.eventId, this->contextName);
    this->createEventQueue.push(thisev);
    mcreateEvent.msg = NULL;
    thisev.param = NULL;
  }
  macedbg(1) << "createEventQueue size=" << createEventQueue.size() << Log::endl;
  
  mace::ContextBaseClass::EventStorageType::iterator wIter = nonContextParams->waitingEvents.begin();
  for(; wIter != nonContextParams->waitingEvents.end(); wIter++ ) {
    mace::vector<mace::__EventStorageStruct__>& wevents = wIter->second;
    for( uint32_t i=0; i<wevents.size(); i++ ){
      mace::__EventStorageStruct__& we = wevents[i];
      this->waitingEvents[ wIter->first ].push_back(we);
      we.msg = NULL;
    }
  }
  macedbg(1) << "waitingEvents size="<< this->waitingEvents.size() << Log::endl;
  
  mace::map<uint64_t, mace::Event>::const_iterator iter = params->executeCommitEventQueue.begin();
  for(; iter != params->executeCommitEventQueue.end(); iter++) {
    Event* event = getEventObject();
    *event = iter->second;
    ContextCommitEvent* commitEvent = new ContextCommitEvent(sv, event->eventId, this, true);
    this->executeCommitEventQueue[iter->first] = commitEvent;
  }
  
  macedbg(1) << "executeCommitEventQueue size=" << this->executeCommitEventQueue.size() << Log::endl;
    
  macedbg(1) << "ContextBaseClassParams: executeEventQueue=" << nonContextParams->executeEventQueue.size() << Log::endl;
  while( !nonContextParams->executeEventQueue.empty() ) {
    ContextEvent& e = nonContextParams->executeEventQueue.front();
    ContextEvent ce(e.eventId, sv, e.type, e.param, e.source, e.sid, this);
    this->executeEventQueue.push_back(ce);
    e.param = NULL;
    nonContextParams->executeEventQueue.pop_front();
    ce.param = NULL;
  }
  macedbg(1) << "ContextBaseClass: executeEventQueue=" << this->executeEventQueue.size() << Log::endl;
    
  this->create_now_committing_ticket = params->create_now_committing_ticket;
  this->execute_now_committing_eventId = params->execute_now_committing_eventId;
  this->execute_now_committing_ticket = params->execute_now_committing_ticket;

  this->now_serving_eventId = params->now_serving_eventId;
  this->now_serving_execute_ticket = params->now_serving_execute_ticket;
  
  this->lastWrite = params->lastWrite;

  this->contextEventOrder = params->contextEventOrder;
  this->dominator = params->dominator;

  this->runtimeInfo = params->runtimeInfo;

  this->createTicketNumber = params->createTicketNumber;
  this->executeTicketNumber = params->executeTicketNumber;

  this->executedEventsCommitFlag = params->executedEventsCommitFlag;
  this->now_serving_create_ticket = params->now_serving_create_ticket;
  this->now_max_execute_ticket = params->now_max_execute_ticket;
  
  this->eventExecutionInfos = params->eventExecutionInfos; 
  this->readerEvents = params->readerEvents;
  this->writerEvents = params->writerEvents;
  this->migrationEventWaiting = params->migrationEventWaiting;
  this->skipCreateTickets = params->skipCreateTickets;

  this->numWriters = this->writerEvents.size();
  this->numReaders = this->readerEvents.size();

  macedbg(1) << "numReaders=" << this->readerEvents.size() << " numWriters=" << this->writerEvents << Log::endl; 
}

void mace::ContextBaseClass::increaseHandlingMessageNumber() {
  ADD_SELECTORS("ContextBaseClass::increaseHandlingMessageNumber");
  ScopedLock sl(contextMigratingMutex);
  handlingMessageNumber ++;
  // macedbg(1) << "Increasing context("<< this->contextName <<")'s handlingMessageNumber to " << handlingMessageNumber << Log::endl;
}

void mace::ContextBaseClass::decreaseHandlingMessageNumber() {
  ADD_SELECTORS("ContextBaseClass::decreaseHandlingMessageNumber");
  ScopedLock sl(contextMigratingMutex);
  handlingMessageNumber --;

  if( isWaitingForHandlingMessages ){
    // macedbg(1) << "decreasing context("<< this->contextName <<")'s handlingMessageNumber to " << handlingMessageNumber << Log::endl;
  }
  if( isWaitingForHandlingMessages && handlingMessageNumber == 0) {
    // macedbg(1) << "context("<< this->contextName <<") release waiting migration event!" << Log::endl;
    pthread_cond_signal(&contextMigratingCond);
  }
}

void mace::ContextBaseClass::waitingForMessagesDone() {
  ADD_SELECTORS("ContextBaseClass::waitingForMessagesDone");
  ScopedLock sl(contextMigratingMutex);
  macedbg(1) << "Current handling messages for Context("<< contextName<<") is " << handlingMessageNumber << Log::endl;
  if( handlingMessageNumber > 0){
    isWaitingForHandlingMessages = true;
    pthread_cond_wait( &contextMigratingCond, &contextMigratingMutex);
    isWaitingForHandlingMessages = false;
  }
  ASSERT( handlingMessageNumber == 0 );
  macedbg(1) << "Now handling messages for Context("<< contextName<<") is " << handlingMessageNumber << Log::endl; 
}

void mace::ContextBaseClass::waitingForCreateEventsDone() {
  ADD_SELECTORS("ContextBaseClass::waitingForCreateEventsDone");
  ScopedLock sl(createEventMutex);
  macedbg(1) << "Current handling create events for Context("<< contextName<<") is " << handlingCreateEventNumber << Log::endl;
  if( handlingCreateEventNumber > 0){
    isWaitingForHandlingCreateEvents = true;
    pthread_cond_wait( &contextMigratingCond, &createEventMutex);
    isWaitingForHandlingCreateEvents = false;
  }
  ASSERT( handlingCreateEventNumber == 0 );
  macedbg(1) << "Now handling create event for context("<< contextName<<") is " << handlingCreateEventNumber << Log::endl; 
}

void mace::ContextBaseClass::setMigratingFlag( const bool flag ) {
  ScopedLock sl(contextMigratingMutex);
  migrationEventWaiting = flag;
}

bool ContextBaseClass::checkEventExecutePermission(BaseMaceService* sv, mace::EventOperationInfo const& eventOpInfo, 
    const bool& add_permit_contexts, mace::set<mace::string>& permittedContextNames ) {
  ADD_SELECTORS("ContextBaseClass::checkEventExecutePermission");

  ScopedLock dsl( this->contextDominatorMutex );
  macedbg(1) << "Check permission in context("<< this->contextName <<") for "<< eventOpInfo << Log::endl;
  ASSERT( eventOpInfo.requireContextName != "" );
  
  ContextService* _service = static_cast<ContextService*>(sv);
  ContextStructure& contextStructure = _service->contextStructure;
  contextStructure.getLock(ContextStructure::READER);

  const mace::string dominatorContext = contextStructure.getUpperBoundContextName(eventOpInfo.requireContextName);
  ASSERT( dominatorContext != "" );
  // macedbg(1) << "context("<< eventOpInfo.requireContextName <<")'s dominator is " << dominatorContext << Log::endl;
  if( dominatorContext != this->contextName && add_permit_contexts ) {
    this->addEventPermitContexts( eventOpInfo.eventId, eventOpInfo.permitContexts );
  }
  
  if( eventOpInfo.requireContextName == this->contextName && 
      this->checkEventExecutePermitCache(eventOpInfo.eventId, eventOpInfo.toContextName ) ){
    macedbg(1) << "Cached permited context: " << eventOpInfo.toContextName << " in " << this->contextName << Log::endl;
    permittedContextNames.insert( eventOpInfo.toContextName );
    if( eventOpInfo.fromContextName == this->contextName ) {
      this->addChildEventOp( eventOpInfo );
    }
  } else {
    if( dominatorContext == this->contextName ) { // local dominator
      permittedContextNames = dominator.checkEventExecutePermission(sv, eventOpInfo);
      if( this->contextName == eventOpInfo.requireContextName ){
        this->addEventPermitContexts( eventOpInfo.eventId, permittedContextNames );
      }
    } else {
      // Require permission from the dominator
      contextStructure.releaseLock(ContextStructure::READER);
      dsl.unlock();
      permittedContextNames = _service->send__event_requireEventExecutePermission( dominatorContext, eventOpInfo );
      if( permittedContextNames.count(eventOpInfo.toContextName) > 0 ) {
        return true;
      } else {
        return false;
      }
    }
  }
  
  contextStructure.releaseLock(ContextStructure::READER);
  if( permittedContextNames.count(eventOpInfo.toContextName) > 0 ) {
    return true;
  } else {
    if( dominatorContext == this->contextName ) {
    // macedbg(1) << "Event("<< eventOpInfo.eventId <<") could not lock context("<< eventOpInfo.toContextName <<") in dominator("<< this->contextName <<")!" << Log::endl;
      dominator.printWaitingEventID(eventOpInfo);
    }
    return false;
  }
  
}

void ContextBaseClass::addEventPermitContexts( mace::OrderID const& eventId, mace::vector<mace::string> const& permitContexts) {
  ADD_SELECTORS("ContextBaseClass::addEventPermitContexts");
  if( permitContexts.size() > 0 ){
    // macedbg(1) << "Add permit context("<< permitContexts <<") for event("<< eventId <<") in context("<< this->contextName <<")!" << Log::endl;
  }
  for( uint32_t i=0; i<permitContexts.size(); i++ ){
    const mace::string& ctxName = permitContexts[i];
    this->addEventPermitContext(eventId, ctxName);
  }
}

void ContextBaseClass::addEventPermitContexts( mace::OrderID const& eventId, mace::set<mace::string> const& permitContexts) {
  ADD_SELECTORS("ContextBaseClass::addEventPermitContexts");
  if( permitContexts.size() > 0 ) {
    // macedbg(1) << "Add permit context("<< permitContexts <<") for event("<< eventId <<") in context("<< this->contextName <<")!" << Log::endl;
  }
  for( mace::set<mace::string>::iterator iter=permitContexts.begin(); iter!=permitContexts.end(); iter++ ){
    this->addEventPermitContext(eventId, *iter);
  }
}

void ContextBaseClass::addEventPermitContext( mace::OrderID const& eventId, mace::string const& permitContext) {
  ADD_SELECTORS("ContextBaseClass::addEventPermissionContext");
  
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<OrderID, EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  if( iter == eventExecutionInfos.end() ){
    EventExecutionInfo info;
    info.addEventPermitContext(permitContext);
    eventExecutionInfos[eventId] = info;
  } else{
    (iter->second).addEventPermitContext(permitContext);
  }
}

mace::vector<mace::string> ContextBaseClass::getEventPermitContexts( const mace::OrderID& eventId ) {
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<OrderID, EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  mace::vector<mace::string> permit_contexts;
  if( iter != eventExecutionInfos.end() ){
    mace::set<mace::string> p_contexts = (iter->second).getEventPermitContexts();

    for( mace::set<mace::string>::iterator iter = p_contexts.begin(); iter != p_contexts.end(); iter ++ ){
      permit_contexts.push_back(*iter);
    }
  } 
  return permit_contexts;
}

bool ContextBaseClass::checkEventExecutePermitCache( const mace::OrderID& eventId, const mace::string& ctxName) {
  ADD_SELECTORS("ContextBaseClass::checkEventExecutePermitCache");
  
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<OrderID, EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  if( iter == eventExecutionInfos.end() ){
    return false;
  } else{
    return (iter->second).checkEventExecutePermitCache(ctxName);
  }
}

void ContextBaseClass::clearEventPermitCache() {
  ADD_SELECTORS("ContextBaseClass::clearEventPermitCache");
  ScopedLock sl(eventExecutingSyncMutex);
  
  mace::map<OrderID, EventExecutionInfo>::iterator iter;
  for( iter=eventExecutionInfos.begin(); iter!=eventExecutionInfos.end(); iter++ ){
    (iter->second).clearEventPermitCache();
  }
}

void ContextBaseClass::clearEventPermitCacheNoLock() {
  mace::map<OrderID, EventExecutionInfo>::iterator iter;
  for( iter=eventExecutionInfos.begin(); iter!=eventExecutionInfos.end(); iter++ ){
    (iter->second).clearEventPermitCache();
  }
}

void ContextBaseClass::notifyReleasedContexts(BaseMaceService* sv, mace::OrderID const& eventId, 
    mace::vector<mace::string> const& releaseContexts) {
  ADD_SELECTORS("ContextBaseClass::notifyReleasedContexts");
  if( releaseContexts.size() > 0 ){
    macedbg(1) << "Event("<< eventId <<") releases contexts: " << releaseContexts << Log::endl;
  }
  ContextService* _service = static_cast<ContextService*>(sv);
  ContextStructure& contextStructure = _service->contextStructure;
  // ASSERT( contextStructure.getUpperBoundContextName(this->contextName) == this->contextName );

  for( mace::vector<mace::string>::const_iterator iter=releaseContexts.begin(); iter!=releaseContexts.end(); iter++ ) { 
    const mace::string& releaseContext = *iter; 
    if( releaseContext == this->contextName ){
      _service->send__event_releaseLockOnContext( releaseContext, this->contextName, eventId );
    } else if( contextStructure.isUpperBoundContext(releaseContext) ) {
      mace::vector<mace::EventOperationInfo> local_lock_requests;
      mace::vector<mace::string> locked_contexts;
      _service->send__event_releaseContext(releaseContext, eventId, releaseContext, local_lock_requests, locked_contexts, this->contextName );
    } else {
      _service->send__event_releaseLockOnContext( releaseContext, this->contextName, eventId );
    }
  } 
}

void ContextBaseClass::notifyNextExecutionEvents(BaseMaceService* sv, 
    const mace::map< mace::string, mace::vector<mace::EventOperationInfo> >& permittedEventOps, 
    const mace::map< mace::OrderID, mace::vector<mace::string> >& permittedContextNames ) {

  ADD_SELECTORS("ContextBaseClass::notifyNextExecutionEvents");
  if( permittedContextNames.size() > 0 ){
    macedbg(1) << "dominator("<< this->contextName <<") notify next events: " << permittedContextNames << Log::endl;
  }
  ContextService* _service = static_cast<ContextService*>(sv);

  mace::vector<mace::EventOperationInfo>::const_iterator sIter;
  mace::map< mace::OrderID, mace::vector<mace::string> >::const_iterator mIter;

  for( mace::map<mace::string, mace::vector<mace::EventOperationInfo> >::const_iterator iter = permittedEventOps.begin();
      iter != permittedEventOps.end(); iter ++ ) {
    const mace::vector<mace::EventOperationInfo>& eops = iter->second;
    mace::map< mace::OrderID, mace::vector<mace::EventOperationInfo> > ops;
    for( sIter = eops.begin(); sIter != eops.end(); sIter ++ ){
      ops[ (*sIter).eventId ].push_back(*sIter);
    }

    for( mace::map< mace::OrderID, mace::vector<mace::EventOperationInfo> >::iterator iter2 = ops.begin(); iter2 != ops.end();
        iter2 ++ ){
      mIter = permittedContextNames.find( iter2->first );
      ASSERT( mIter != permittedContextNames.end() );
      if( iter->first == this->contextName ){
        this->handleEventExecutePermission( sv, iter2->second, mIter->second );
      } else {
        _service->send__event_replyEventExecutePermission( iter->first, this->contextName, iter2->first, mIter->second,  
          iter2->second );
      }
    }
  }
}

bool ContextBaseClass::localUnlockContext( mace::EventOperationInfo const& eventOpInfo, 
    mace::vector<mace::EventOperationInfo> const& local_lock_requests, mace::vector<mace::string> const& locked_contexts) {
  ADD_SELECTORS("ContextBaseClass::localUnlockContext");
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventOpInfo.eventId);
  ASSERT( iter != eventExecutionInfos.end() );
  bool succ = (iter->second).localUnlockContext( eventOpInfo, local_lock_requests, locked_contexts);
  if( succ ){
    macedbg(1) << "To remove localLockRequest("<< eventOpInfo <<") in context("<< this->contextName <<")!" << Log::endl;
  } else {
    macedbg(1) << "There is no localLockRequest("<< eventOpInfo <<") in context("<< this->contextName <<")!" << Log::endl;
  }
  return succ;
}

void ContextBaseClass::setEventExecutionInfo( mace::OrderID const& eventId, mace::string const& createContextName,  
      mace::string const& targetContextName, const uint8_t eventOpType){

  mace::map<OrderID, EventExecutionInfo>::iterator eeinfo_iter = getEventExecutionInfo(eventId);
  if( eeinfo_iter == eventExecutionInfos.end() ) {
    EventExecutionInfo ee_info;
    ee_info.createContextName = createContextName;
    ee_info.targetContextName = targetContextName;
    ee_info.eventOpType = eventOpType;
    eventExecutionInfos[eventId] = ee_info;
  } else {
    (eeinfo_iter->second).createContextName = createContextName;
    (eeinfo_iter->second).targetContextName = targetContextName;
    (eeinfo_iter->second).eventOpType = eventOpType;
  }
}

void ContextBaseClass::enqueueSubEvent( OrderID const& eventId, mace::EventRequestWrapper const& eventRequest ) {
  ADD_SELECTORS("ContextBaseClass::enqueueSubEvent");
  ScopedLock sl(eventExecutingSyncMutex);
  macedbg(1) << "Enqueue a subevent to event("<< eventId <<") in context("<< contextName <<")" << Log::endl;
  mace::map<OrderID, EventExecutionInfo>::iterator eeinfo_iter = getEventExecutionInfo(eventId);
  if( eeinfo_iter == eventExecutionInfos.end() ) {
    EventExecutionInfo ee_info;
    ee_info.enqueueSubEvent(eventRequest);
    eventExecutionInfos[eventId] = ee_info;
  } else {
    (eeinfo_iter->second).enqueueSubEvent(eventRequest);
  }
}

void ContextBaseClass::enqueueExternalMessage( OrderID const& eventId, mace::EventMessageRecord const& msg ) {
  ADD_SELECTORS("ContextBaseClass::enqueueExternalMessage");
  ScopedLock sl(eventExecutingSyncMutex);
  macedbg(1) << "Enqueue a externalmessage to event("<< eventId <<") in context("<< this->contextName <<")" << Log::endl;
  mace::map<OrderID, EventExecutionInfo>::iterator eeinfo_iter = getEventExecutionInfo(eventId);
  if( eeinfo_iter == eventExecutionInfos.end() ) {
    EventExecutionInfo ee_info;
    ee_info.enqueueExternalMessage(msg);
    eventExecutionInfos[eventId] = ee_info;
  } else {
    (eeinfo_iter->second).enqueueExternalMessage(msg);
  }
}

mace::vector< mace::EventRequestWrapper > ContextBaseClass::getSubEvents(mace::OrderID const& eventId) {
  ADD_SELECTORS("ContextBaseClass::getSubEvents");
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<OrderID, EventExecutionInfo>::iterator eeinfo_iter = getEventExecutionInfo(eventId);
  ASSERT( eeinfo_iter != eventExecutionInfos.end() );
  return (eeinfo_iter->second).getSubEvents();
}

mace::vector< mace::EventMessageRecord > ContextBaseClass::getExternalMessages(mace::OrderID const& eventId) {
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<OrderID, EventExecutionInfo>::iterator eeinfo_iter = getEventExecutionInfo(eventId);
  ASSERT( eeinfo_iter != eventExecutionInfos.end() );
  return (eeinfo_iter->second).getExternalMessages();
}

uint64_t ContextBaseClass::getNextOperationTicket(OrderID const& eventId ){
  //ASSERTMSG( this->now_serving_eventId == eventId, "This context is locked by a different event");
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<OrderID, EventExecutionInfo>::iterator eeinfo_iter = getEventExecutionInfo(eventId);
  if( eeinfo_iter == eventExecutionInfos.end() ) {
    EventExecutionInfo ee_info;
    eventExecutionInfos[eventId] = ee_info;
  } 

  return eventExecutionInfos[eventId].getNextTicket();
}

void ContextBaseClass::enqueueEventOperation( OrderID const& eventId, EventOperationInfo const& opInfo ){
  ADD_SELECTORS("ContextBaseClass::enqueueEventOperation");
  macedbg(1) << "enqueue an ownershipOp("<< opInfo<<") of Event("<< eventId <<") to context("<< contextName<<")." << Log::endl; 
  ASSERTMSG( this->now_serving_eventId == eventId, "This context is locked by a different event" );
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<OrderID, EventExecutionInfo>::iterator eeinfo_iter = getEventExecutionInfo(eventId);
  if( eeinfo_iter == eventExecutionInfos.end() ) {
    EventExecutionInfo ee_info;
    ee_info.addEventOpInfo( opInfo );
    eventExecutionInfos[eventId] = ee_info;
  } else {
    (eeinfo_iter->second).addEventOpInfo( opInfo );
  }
}

void ContextBaseClass::enqueueOwnershipOpInfo( mace::OrderID const& eventId, mace::EventOperationInfo const& op ) {
  ADD_SELECTORS("ContextBaseClass::enqueueOwnershipOpInfo");
  ScopedLock sl(eventExecutingSyncMutex);
  macedbg(1) << "Enqueue a ownershipOp("<< op <<") to event("<< eventId <<") in context("<< this->contextName <<")" << Log::endl;
  mace::map<OrderID, EventExecutionInfo>::iterator eeinfo_iter = getEventExecutionInfo(eventId);
  if( eeinfo_iter == eventExecutionInfos.end() ) {
    EventExecutionInfo ee_info;
    ee_info.enqueueOwnershipOpInfo(op);
    eventExecutionInfos[eventId] = ee_info;
  } else {
    (eeinfo_iter->second).enqueueOwnershipOpInfo(op);
  }

  // mace::ElasticityCondition cond( mace::ElasticityCondition::REFERENCE, Util::extractContextType(op.fromContextName), 
  //   Util::extractContextType(op.toContextName) );
  // mace::vector< mace::ElasticityCondition > conds;
  // conds.push_back( cond );

  // mace::ElasticityBehavior behavior = eConfig.getElasticityBehavior( conds );
  // if( behavior.behaviorType != mace::ElasticityBehavior::INVALID ){
  //   behavior.contextNames.insert( op.fromContextName );
  //   behavior.contextNames.insert( op.toContextName );
  // }
  // mace::eMonitor::applyElasticityRule(behavior);
}

mace::vector<mace::EventOperationInfo> ContextBaseClass::extractOwnershipOpInfos( OrderID const& eventId ) {
  //ASSERT( eventId == this->now_serving_eventId );
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<OrderID, EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  if( iter != eventExecutionInfos.end() ){
    return (iter->second).extractOwnershipOpInfos();
  } else {
    mace::vector<EventOperationInfo> empty_vector;
    return empty_vector;
  }
}

mace::EventOperationInfo ContextBaseClass::getNewContextOwnershipOp( mace::OrderID const& eventId, mace::string const& parentContextName,
    mace::string const& childContextName ) {
  //ASSERT( eventId == this->now_serving_eventId );
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<OrderID, EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  if( iter != eventExecutionInfos.end() ){
    return (iter->second).getNewContextOwnershipOp( parentContextName, childContextName);
  } else {
    mace::EventOperationInfo oop;
    return oop;
  }
}

bool ContextBaseClass::checkParentChildRelation(mace::OrderID& eventId, mace::string const& parentContextName, 
    mace::string const& childContextName ) {

  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<OrderID, EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  if( iter != eventExecutionInfos.end() ){
    return (iter->second).checkParentChildRelation(parentContextName, childContextName);
  } else {
    return false;
  }
}

void ContextBaseClass::applyOwnershipOperations( const BaseMaceService* sv, mace::EventOperationInfo const& eop, 
    mace::vector<mace::EventOperationInfo> const& ownershipOpInfos ) {
  ADD_SELECTORS("ContextBaseClass::applyOwnershipOperations");
  // enqueue cached lock requests
  const ContextService* _service = static_cast<const ContextService*>(sv);
  const ContextStructure& ctxStructure = _service->contextStructure;

  mace::string dominator_context = this->contextName;
  bool is_ancestor = true;
  for( uint32_t i=0; i<ownershipOpInfos.size(); i++ ){
    const mace::EventOperationInfo& o = ownershipOpInfos[i];
    ASSERT( o.fromContextName == this->contextName );
    if( !ctxStructure.isElderContext(o.toContextName, this->contextName) && ctxStructure.connectToRootNode(o.toContextName) ){
      macedbg(1) << "context("<< this->contextName <<") is not ancestor of context("<< o.toContextName <<")!" << Log::endl;
      is_ancestor = false;
      break;
    }
  }

  bool is_dominator = false;
  if( ctxStructure.isUpperBoundContext(this->contextName) ){
    is_dominator = true;
  } else {
    macedbg(1) << "context("<< this->contextName <<") is not a dominator!" << Log::endl;
  }
  
  if( is_ancestor && is_dominator ){

  } else {
    ScopedLock sl( this->eventExecutingSyncMutex );
    if( currOwnershipEventOp.eventId == eop.eventId && currOwnershipEventOp.opType == mace::EventOperationInfo::ROUTINE_OP ){
      dominator_context = eop.getPreAccessContextName(dominator_context);
      ASSERT(dominator_context != "");
    } else if(ctxStructure.isUpperBoundContext(this->contextName)) {
      dominator_context = this->contextName;
    } else {
      dominator_context = ctxStructure.getUpperBoundContextName(this->contextName);
      ASSERT( dominator_context != "" );
    }
    sl.unlock();
  }
      
  macedbg(1) << "To apply ownership operations of "<< eop <<" on context("<< dominator_context <<")!" << Log::endl;
  ScopedLock sl(eventExecutingSyncMutex);
  _service->send__ownership_ownershipOperations(dominator_context, eop, this->contextName, ownershipOpInfos);
	pthread_cond_t cond;
	pthread_cond_init(&cond,  NULL);
	eventExecutingSyncConds[eop] = &cond;
  pthread_cond_wait( &cond, &eventExecutingSyncMutex );
	eventExecutingSyncConds[eop] = NULL;
	eventExecutingSyncConds.erase(eop);
  pthread_cond_destroy( &cond );
  macedbg(1) << "Ownership("<< ownershipOpInfos << ") are done!" << Log::endl;
}

void ContextBaseClass::addOwnershipOperations( const BaseMaceService* sv, mace::EventOperationInfo const& eop, 
    mace::vector<mace::EventOperationInfo> const& ownershipOpInfos ) {
  ADD_SELECTORS("ContextBaseClass::addOwnershipOperations");
  // enqueue cached lock requests
  const ContextService* _cservice = static_cast<const ContextService*>(sv);
  ContextService* _service = const_cast<ContextService*>(_cservice);
  
  mace::ContextBaseClass* ctxObj = _service->getContextObjByName(eop.fromContextName);
  if( ctxObj != NULL ) {
    for( uint32_t i=0; i<ownershipOpInfos.size(); i++ ){
      ctxObj->enqueueOwnershipOpInfo(eop.eventId, ownershipOpInfos[i] );
    }
  } else {
    macedbg(1) << "Try to send ownership ops of Event("<< eop.eventId <<") to " << eop.fromContextName << " from " << this->contextName << Log::endl;
    
    ScopedLock sl(eventExecutingSyncMutex);
    _service->send__event_enqueueOwnershipOps(eop, eop.fromContextName, this->contextName, ownershipOpInfos);
    
    pthread_cond_t cond;
    pthread_cond_init( &cond, NULL );
    eventExecutingSyncConds[ eop ] = &cond;
    pthread_cond_wait( &cond, &eventExecutingSyncMutex );
    pthread_cond_destroy( &cond );
    eventExecutingSyncConds[eop] = NULL;
    eventExecutingSyncConds.erase(eop);
    macedbg(1) << "Done send ownership ops of Event("<< eop.eventId <<") to " << eop.fromContextName << " from " << this->contextName << Log::endl;
  }
}

void ContextBaseClass::handleOwnershipOperations( BaseMaceService* sv, const mace::EventOperationInfo& eop, 
    const mace::string& src_ctxName, const mace::vector<mace::EventOperationInfo>& ownershipOpInfos ) {
  ADD_SELECTORS("ContextBaseClass::handleOwnershipOperations");
  
  ContextService* _service = static_cast<ContextService*>(sv);
  ContextStructure& contextStructure = _service->contextStructure;

  mace::string dominator_context = this->contextName;
  bool is_ancestor = true;
  for( uint32_t i=0; i<ownershipOpInfos.size(); i++ ){
    const mace::EventOperationInfo& o = ownershipOpInfos[i];
    if( !contextStructure.isElderContext(o.toContextName, this->contextName) && contextStructure.connectToRootNode(o.toContextName) ){
      macedbg(1) << "context("<< this->contextName <<") is not ancestor of context("<< o.toContextName <<")!" << Log::endl;
      is_ancestor = false;
      break;
    }
  }

  bool is_dominator = false;
  if( contextStructure.isUpperBoundContext(this->contextName) ){
    is_dominator = true;
  } else {
    macedbg(1) << "context("<< this->contextName <<") is not a dominator!" << Log::endl;
  }
  
  if( is_ancestor && is_dominator ){

  } else {
    ScopedLock sl( this->eventExecutingSyncMutex );
    if( currOwnershipEventOp.eventId == eop.eventId && currOwnershipEventOp.opType == mace::EventOperationInfo::ROUTINE_OP ){
      dominator_context = eop.getPreAccessContextName(dominator_context);
      ASSERT(dominator_context != "");
      _service->send__ownership_ownershipOperations( dominator_context, eop, src_ctxName, ownershipOpInfos );
      return;
    } else if(contextStructure.isUpperBoundContext(this->contextName)) {
      dominator_context = this->contextName;
    } else {
      dominator_context = contextStructure.getUpperBoundContextName(this->contextName);
      ASSERT( dominator_context != "" );
      _service->send__ownership_ownershipOperations( dominator_context, eop, src_ctxName, ownershipOpInfos );
      return;
    }
    sl.unlock();
  }

  const mace::ContextMapping& snapshot = _service->getLatestContextMapping();
  
  mace::map< MaceAddr, mace::set<mace::string> > toUpdateDAGContexts; 
  const mace::map<mace::string, uint64_t>& ctx_dag_vers = eop.contextDAGVersions;

  ScopedLock dom_sl( this->contextDominatorMutex );
  for( mace::map<mace::string, uint64_t>::const_iterator iter = ctx_dag_vers.begin(); iter != ctx_dag_vers.end(); iter ++ ) {
    uint64_t ver = contextStructure.getDAGNodeVersion(iter->first);
    if( ver == 0 || ver < iter->second ){
      macedbg(1) << "context("<< iter->first <<") ver1=" << ver << ", ver2=" << iter->second << Log::endl;
      mace::MaceAddr addr = mace::ContextMapping::getNodeByContext( snapshot, iter->first );
      ASSERT( addr != Util::getMaceAddr() );
      toUpdateDAGContexts[addr].insert(iter->first);
      dominator.addUpdateWaitingContext(iter->first);
    }
  }  

  if( toUpdateDAGContexts.size() > 0 ){
    for( mace::map< MaceAddr, mace::set<mace::string> >::iterator iter = toUpdateDAGContexts.begin(); iter != toUpdateDAGContexts.end();
        iter ++ ) {
      _service->send__ownership_contextDAGRequest( this->contextName, iter->first, iter->second );
    }
    mace::vector<mace::EventOperationInfo> eops;
    eops.push_back(eop);
    for( uint32_t i=0; i<ownershipOpInfos.size(); i++ ) {
      eops.push_back(ownershipOpInfos[i]);
    }

    dominator.setWaitingUnlockRequests( eops );
    dominator.addUpdateSourceContext( src_ctxName );
  } else {
    this->executeOwnershipOperations( sv, eop, src_ctxName, ownershipOpInfos );
  }
}

void ContextBaseClass::executeOwnershipOperations( BaseMaceService* sv, const mace::EventOperationInfo& eop, 
    const mace::string& src_ctxName, const mace::vector<mace::EventOperationInfo>& ownershipOpInfos ) {
  ADD_SELECTORS("ContextBaseClass::executeOwnershipOperations");
  macedbg(1) << "To apply ownerships of "<< eop <<" in context("<< this->contextName <<")!" << Log::endl;

  ContextService* _service = static_cast<ContextService*>(sv);
  ContextStructure& contextStructure = _service->contextStructure;

  const mace::ContextMapping& snapshot = _service->getLatestContextMapping();

  contextStructure.getLock(ContextStructure::WRITER);
  ASSERT( contextStructure.getUpperBoundContextName(this->contextName) == this->contextName );
  mace::set<mace::string> affected_contexts_set = contextStructure.modifyOwnerships( this->contextName, ownershipOpInfos);
  ASSERT( contextStructure.getUpperBoundContextName(this->contextName) == this->contextName );
  contextStructure.releaseLock(ContextStructure::WRITER);

  macedbg(1) << "AffectedContextNames: " << affected_contexts_set << Log::endl;
  
  mace::vector< mace::pair<mace::string, mace::string> > ownershipPairs = contextStructure.getOwnershipOfContexts( affected_contexts_set );
  mace::map< mace::string, uint64_t > ownershipVers;

  for( uint32_t i=0; i<ownershipPairs.size(); i++ ){
    const mace::pair<mace::string, mace::string>& opair = ownershipPairs[i];
    uint64_t ver = contextStructure.getDAGNodeVersion(opair.first);
    ownershipVers[opair.first] = ver;

    ver = contextStructure.getDAGNodeVersion(opair.second);
    ownershipVers[opair.second] = ver;    
  }

  for(mace::set<mace::string>::const_iterator c_iter = affected_contexts_set.begin(); c_iter != affected_contexts_set.end(); 
      c_iter ++ ) {
    if( ContextMapping::hasContext2( snapshot, *c_iter) == 0 ) {
      macedbg(1) << "Try to create context: " << *c_iter << Log::endl;
      _service->asyncHead( eop.eventId, *c_iter, ownershipPairs, ownershipVers); 
    }
  }

  const mace::ContextMapping& update_snapshot = _service->getLatestContextMapping();

  dominator.addUpdateSourceContext(src_ctxName);
  dominator.addReplyEventOp(eop);

  mace::vector<mace::EventOperationInfo> forward_eops;
  dominator.updateDominator( contextStructure, forward_eops );
  macedbg(1) << "forward_eops: " << forward_eops << Log::endl;
  
  mace::map< mace::MaceAddr, mace::set<mace::string> > dom_update_addrs;
  affected_contexts_set.erase( this->contextName );

  for( mace::set<mace::string>::iterator iter = affected_contexts_set.begin(); iter != affected_contexts_set.end(); 
      iter ++ ){
    const mace::MaceAddr& destAddr = ContextMapping::getNodeByContext(update_snapshot, *iter );
    dom_update_addrs[destAddr].insert(*iter);
    dominator.addUpdateWaitingContext(*iter);
  }    

  for(mace::map< mace::MaceAddr, mace::set<mace::string> >::const_iterator addrIter = dom_update_addrs.begin(); 
      addrIter != dom_update_addrs.end(); addrIter ++ ){
    _service->send__ownership_updateOwnershipAndDominators( addrIter->first, this->contextName, addrIter->second, forward_eops, 
      ownershipPairs, ownershipVers );
  }
}

void ContextBaseClass::handleOwnershipOperationsReply( mace::EventOperationInfo const& eop ) {
  ADD_SELECTORS("ContextBaseClass::handleOwnershipOperationsReply");
  ScopedLock sl(eventExecutingSyncMutex);

  ASSERT( eventExecutingSyncConds.find(eop) != eventExecutingSyncConds.end() );
  pthread_cond_signal( eventExecutingSyncConds[eop] );
}

void ContextBaseClass::handleContextDAGReply( BaseMaceService* sv, const mace::set<mace::string>& contexts, 
    const mace::vector<mace::pair<mace::string, mace::string > >& ownershipPairs, const mace::map<mace::string, uint64_t>& vers ) {
  ADD_SELECTORS("ContextBaseClass::handleContextDAGReply");

  ContextService* _service = static_cast<ContextService*>(sv);
  ContextStructure& contextStructure = _service->contextStructure;

  contextStructure.getLock(ContextStructure::WRITER);
  contextStructure.updateOwnerships( ownershipPairs, vers );
  contextStructure.releaseLock(ContextStructure::WRITER);

  ScopedLock sl(this->contextDominatorMutex);
  for( mace::set<mace::string>::const_iterator iter = contexts.begin(); iter != contexts.end(); iter ++ ){
    dominator.removeUpdateWaitingContext(*iter);
  }

  if( !dominator.isUpdateWaitingForReply() ) {
    mace::vector<mace::EventOperationInfo> eops = dominator.getUpdateReplyEventOps();
    dominator.clearUpdateReplyEventOps();

    ASSERT( eops.size() > 1 );
    mace::EventOperationInfo eop = eops[0];
    mace::vector<mace::EventOperationInfo> ownershipOpInfos;
    for( uint32_t i=1; i<eops.size(); i++ ){
      ownershipOpInfos.push_back(eops[i]);
    }

    mace::set<mace::string> src_contexts = dominator.getUpdateSourceContexts();
    ASSERT(src_contexts.size() == 1);
    dominator.clearUpdateSourceContexts();
    mace::string src_contextName = *(src_contexts.begin());

    this->executeOwnershipOperations(sv, eop, src_contextName, ownershipOpInfos );
  }
}

void ContextBaseClass::enqueueExecuteContextEvent( BaseMaceService* sv, ContextEvent& ce) {
  ADD_SELECTORS("ContextBaseClass::enqueueExecuteContextEvent");
  macedbg(1) << "Enqueue an ContextEvent("<< ce.eventId <<") to context " << this->contextName << Log::endl;
  bool push_back_flag = true;
  mace::deque<mace::ContextEvent>::iterator iter = executeEventQueue.begin();
  for(; iter != executeEventQueue.end(); iter ++ ) {
    const mace::ContextEvent& ce2 = *iter;
    if( ce2.eventId == ce.eventId ) {
      iter ++;
      if( iter != executeEventQueue.end() ) {
        executeEventQueue.insert(iter, ce);
        push_back_flag = false;
      }
      break;
    }
  }
  if( push_back_flag ) {
    executeEventQueue.push_back( ce );
  }
  ce.param = NULL;
  enqueueReadyExecuteEventQueue();
}

void ContextBaseClass::enqueueExecuteContextEventToHead( BaseMaceService* sv, ContextEvent& ce) {
  executeEventQueue.push_front( ce );
  ce.param = NULL;
  enqueueReadyExecuteEventQueue();
}

void ContextBaseClass::enqueueReadyExecuteEventQueueWithLock() {
  ScopedLock sl(executeEventMutex);
  enqueueReadyExecuteEventQueue();
}

void ContextBaseClass::enqueueReadyCreateEventQueueWithLock() {
  ScopedLock sl( createEventMutex );
  enqueueReadyCreateEventQueue();
}

// executeEventQueue should be locked before and during this method
bool ContextBaseClass::enqueueReadyExecuteEventQueue() {
  ADD_SELECTORS("ContextBaseClass::enqueueReadyExecuteEventQueue");
  if( executeEventQueue.empty() ){
    return false;
  }
  
  ContextEvent& e = executeEventQueue.front();
  ContextService* _service = static_cast<ContextService*>(e.sv);

  uint64_t execute_ticket = contextEventOrder.getExecuteEventTicket(e.eventId);
  mace::EventOperationInfo eop;
  // this event already lock this context
  if( execute_ticket != 0 && execute_ticket <= this->now_serving_execute_ticket ) {
    macedbg(1) << "Event("<< e.eventId <<") executed in context("<< this->contextName <<") before!" << Log::endl;
    
    if( e.type == ContextEvent::TYPE_BROADCAST_EVENT ) {
      AsyncEvent_Message* msg = static_cast<AsyncEvent_Message*>(e.param);
      mace::Event& event = msg->getEvent();
      eop = event.eventOpInfo;
    } else if( e.type == ContextEvent::TYPE_ROUTINE_EVENT ) {
      Routine_Message* msg = static_cast<Routine_Message*>(e.param);
      mace::Event& event = msg->getEvent();
      eop = event.eventOpInfo;
    }
    this->addEventFromContext( e.eventId, eop.fromContextName );
    _service->enqueueReadyExecuteEventQueue(e);
    e.param = NULL;
    executeEventQueue.pop_front();
    return true;
  } else {
    
    macedbg(1) << "context("<< this->contextName<<") executeTicket=" << execute_ticket << " nextExecuteTicketNumber=" << 
      contextEventOrder.getExecuteTicketNumber() << " now_serving_execute_ticket="<< this->now_serving_execute_ticket << Log::endl;
    
    ASSERT( contextEventOrder.getExecuteTicketNumber() >= this->now_serving_execute_ticket );

    ContextStructure& contextStructure = _service->contextStructure;
    if( execute_ticket == 0 && contextEventOrder.getExecuteTicketNumber() == this->now_serving_execute_ticket ){ // context allow next event to execute
      mace::EventExecutionInfo eventInfo;
      
      if( e.type == ContextEvent::TYPE_ASYNC_EVENT ){
        AsyncEvent_Message* msg = static_cast<AsyncEvent_Message*>(e.param);
        mace::Event& event = msg->getEvent();
        eventInfo.targetContextName = event.target_ctx_name;
        eventInfo.createContextName = event.create_ctx_name;
        eventInfo.eventOpType = event.eventOpType;
        eop = event.eventOpInfo;

        event.eventContextStructureVersion = contextStructure.getCurrentVersion();
        contextEventOrder.addExecuteEvent(event.eventId);
      } else if( e.type == ContextEvent::TYPE_BROADCAST_EVENT ) {
        AsyncEvent_Message* msg = static_cast<AsyncEvent_Message*>(e.param);
        mace::Event& event = msg->getEvent();
        //macedbg(1) << "Event("<< event.eventId <<") is a broadcast event!" << Log::endl;
        eventInfo.targetContextName = event.target_ctx_name;
        eventInfo.createContextName = event.create_ctx_name;
        eventInfo.eventOpType = event.eventOpType;
        eop = event.eventOpInfo;
        
        this->contextEventOrder.addExecuteEvent(e.eventId);
      } else if( e.type == ContextEvent::TYPE_ROUTINE_EVENT ) {
        Routine_Message* msg = static_cast<Routine_Message*>(e.param);
        mace::Event& event = msg->getEvent();
        eventInfo.targetContextName = event.target_ctx_name;
        eventInfo.createContextName = event.create_ctx_name;
        eventInfo.eventOpType = event.eventOpType;
        eop = event.eventOpInfo;
        
        this->contextEventOrder.addExecuteEvent(e.eventId);
        
      } else {
        ASSERTMSG(false, "Invalid event type!");
        return false;
      }

      const uint64_t executeTicket = contextEventOrder.getExecuteEventTicket(e.eventId);
      if( executeTicket == 0 || executeTicket != this->now_serving_execute_ticket ) {
        maceerr << "context("<< this->contextName <<") executeTicket=" << executeTicket << ", now_serving_execute_ticket=" << this->now_serving_execute_ticket << Log::endl;
        ASSERTMSG(false, "Wrong event ticket assignment!");
      }

      ScopedLock execute_sl( eventExecutingSyncMutex );
      mace::map<mace::OrderID, EventExecutionInfo>::iterator einfo_iter = getEventExecutionInfo(e.eventId);
      if( einfo_iter == eventExecutionInfos.end() ) {
        eventExecutionInfos[e.eventId] = eventInfo;
      } else {
        mace::EventExecutionInfo& eInfo = einfo_iter->second;
        eInfo.targetContextName = eventInfo.targetContextName;
        eInfo.createContextName = eventInfo.createContextName;
        eInfo.eventOpType = eventInfo.eventOpType;
      }
      execute_sl.unlock();

      this->addEventFromContext( e.eventId, eop.fromContextName );
      _service->enqueueReadyExecuteEventQueue(e);
      e.param = NULL;
      this->executeEventQueue.pop_front();
      return true;
    } 
  }
  return false;
}

bool mace::ContextBaseClass::enqueueReadyCreateEventQueue(){
  ADD_SELECTORS("ContextBaseClass::enqueueReadyCreateEventQueue");
  if( this->migrationEventWaiting ) {
    macedbg(1) << "context("<< this->contextName <<") is migrating!" << Log::endl;
    return false;
  }

  if( createEventQueue.empty() ) {
    return false;
  }

  while( skipCreateTickets.size() > 0 ) {
    if( skipCreateTickets.count(this->now_serving_create_ticket) > 0 ){
      skipCreateTickets.erase( this->now_serving_create_ticket);
      this->now_serving_create_ticket ++;
    } else {
      break;
    }
  }

  HeadEventDispatch::HeadEvent& top = createEventQueue.front();
  ContextService* _service = static_cast<ContextService*>(top.cl);
  if( top.eventId.ticket == this->now_serving_create_ticket ) {
    macedbg(1) << "Try to create event with ticket("<< top.eventId.ticket <<") in context("<< this->contextName <<")." << Log::endl;
    _service->enqueueReadyCreateEventQueue(top);
    top.param = NULL;
    createEventQueue.pop();
    this->handlingCreateEventNumber ++;
    return true;    
  } 

  macedbg(1) << "In context("<< this->contextName <<") top event's ticket=" << top.eventId.ticket << " while now_serving_create_ticket=" << this->now_serving_create_ticket << Log::endl;
  return false;
}

bool mace::ContextBaseClass::enqueueReadyCommitEventQueue() {
  ADD_SELECTORS("ContextBaseClass::enqueueCommitExecuteEventQueue");
  ScopedLock sl(commitEventMutex);
  mace::ContextCommitEvent* committingEvent = NULL;
  if( executeCommitEventQueue.find(execute_now_committing_ticket) != executeCommitEventQueue.end() ) {
    committingEvent = executeCommitEventQueue[execute_now_committing_ticket];
  } else {
    macedbg(1) << "There is no available commit events! execute_now_committing_ticket="<< execute_now_committing_ticket << Log::endl;
    return false;
  }
  executeCommitEventQueue[execute_now_committing_ticket] = NULL;
  executeCommitEventQueue.erase(execute_now_committing_ticket);
  sl.unlock();
  ContextService* _service = static_cast<ContextService*>(committingEvent->sv);
  _service->enqueueReadyCommitEventQueue(*committingEvent);
  return true;
}

uint32_t mace::ContextBaseClass::createNewContext(BaseMaceService* sv, mace::string const& contextTypeName, 
    mace::EventOperationInfo& eop ) {
  ADD_SELECTORS("ContextBaseClass::createNewContext");
  ContextService* _service = static_cast<ContextService*>(sv);

  const mace::OrderID& eventId = ThreadStructure::myEventID();
  macedbg(1) << "Event("<< eventId <<") try to create a new context of "<< contextTypeName << "!" << Log::endl;
  const mace::EventOperationInfo& eventOpInfo = ThreadStructure::myEvent().eventOpInfo;
  
  ScopedLock sl(eventExecutingSyncMutex);
  _service->send__event_createNewContext(this->contextName, contextTypeName, eventOpInfo);
  pthread_cond_t cond;
  pthread_cond_init( &cond, NULL );
  eventExecutingSyncConds[ eventOpInfo ] = &cond;
  pthread_cond_wait( &cond, &eventExecutingSyncMutex );
  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  ASSERT( iter != eventExecutionInfos.end() );
  pthread_cond_destroy( &cond );
  eventExecutingSyncConds[eventOpInfo] = NULL;
  eventExecutingSyncConds.erase(eventOpInfo);
  uint32_t new_ctx_id = (iter->second).getNewContextID();

  mace::string new_ctx_name = Util::generateContextName( contextTypeName, new_ctx_id );
  eop.newCreateContexts.insert(new_ctx_name);
  macedbg(1) << "Event("<< eop.eventId <<") creates new context("<< new_ctx_name <<") in " << this->contextName << Log::endl;
  return new_ctx_id;
}

void mace::ContextBaseClass::handleCreateNewContextReply(mace::EventOperationInfo const& eventOpInfo, const uint32_t& newContextId) {
  ADD_SELECTORS("ContextBaseClass::handleCreateNewContextReply");
  const mace::OrderID& eventId = eventOpInfo.eventId;
  macedbg(1) << "Event("<< eventId <<") create a new contextId=" << newContextId << Log::endl;
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  if( iter == eventExecutionInfos.end() ) {
    EventExecutionInfo info;
    eventExecutionInfos[eventId] = info;
  }
  eventExecutionInfos[eventId].setNewContextID(newContextId);
  std::map<mace::EventOperationInfo, pthread_cond_t*>::iterator cond_iter = eventExecutingSyncConds.find(eventOpInfo);
  if( cond_iter != eventExecutingSyncConds.end() ) {
    pthread_cond_signal(cond_iter->second);
  }
}

void mace::ContextBaseClass::enqueueSubEvent(BaseMaceService* sv, mace::EventOperationInfo const& eventOpInfo, mace::string const& targetContextName, mace::EventRequestWrapper const& reqObject) {
  ADD_SELECTORS("ContextBaseClass::enqueueSubEvent");
  ContextService* _service = static_cast<ContextService*>(sv);
  mace::ContextBaseClass* ctxObj = _service->getContextObjByName(targetContextName);
  if( ctxObj != NULL ) {
    ctxObj->enqueueSubEvent(eventOpInfo.eventId, reqObject);
  } else {
    macedbg(1) << "Try to send a subevent of Event("<< eventOpInfo.eventId <<") to " << targetContextName << " from " << this->contextName << Log::endl;
    
    ScopedLock sl(eventExecutingSyncMutex);
    _service->send__event_enqueueSubEvent(eventOpInfo, targetContextName, this->contextName, reqObject);
    
    pthread_cond_t cond;
    pthread_cond_init( &cond, NULL );
    eventExecutingSyncConds[ eventOpInfo ] = &cond;
    pthread_cond_wait( &cond, &eventExecutingSyncMutex );
    pthread_cond_destroy( &cond );
    eventExecutingSyncConds[eventOpInfo] = NULL;
    eventExecutingSyncConds.erase(eventOpInfo);
    macedbg(1) << "Done send a subevent of Event("<< eventOpInfo.eventId <<") to " << targetContextName << " from " << this->contextName << Log::endl;
  }
}

bool ContextBaseClass::deferExternalMessage( mace::EventOperationInfo const& eventOpInfo, mace::string const& targetContextName, uint8_t instanceUniqueID, 
    MaceKey const& dest,  std::string const&  message, registration_uid_t const rid ) {
  ADD_SELECTORS("ContextBaseClass::deferExternalMessage");
  AsyncEventReceiver* sv = BaseMaceService::getInstance(this->serviceId);
  ContextService* _service = static_cast<ContextService*>(sv);

  macedbg(1)<<"defer an external message sid="<<(uint16_t)instanceUniqueID<<", dest="<<dest<<", rid="<<rid<<Log::endl;
  EventMessageRecord emr(instanceUniqueID, dest, message, rid );

  mace::ContextBaseClass* ctxObj = _service->getContextObjByName(targetContextName);
  if( ctxObj != NULL ) {
    ctxObj->enqueueExternalMessage(eventOpInfo.eventId, emr);
  } else {
    macedbg(1) << "Try to send a externalmessage of Event("<< eventOpInfo.eventId <<") to " << targetContextName << " from " << this->contextName << Log::endl;
    
    ScopedLock sl(eventExecutingSyncMutex);
    _service->send__event_enqueueExternalMessage(eventOpInfo, targetContextName, this->contextName, emr);
    
    pthread_cond_t cond;
    pthread_cond_init( &cond, NULL );
    eventExecutingSyncConds[ eventOpInfo ] = &cond;
    pthread_cond_wait( &cond, &eventExecutingSyncMutex );
    pthread_cond_destroy( &cond );
    eventExecutingSyncConds[eventOpInfo] = NULL;
    eventExecutingSyncConds.erase(eventOpInfo);
    macedbg(1) << "Done send a externalmessage of Event("<< eventOpInfo.eventId <<") to " << targetContextName << " from " << this->contextName << Log::endl;
  }
  return true;
}

void mace::ContextBaseClass::handleEnqueueSubEventReply(mace::EventOperationInfo const& eventOpInfo) {
  ADD_SELECTORS("ContextBaseClass::handleEnqueueSubEventReply");
  const mace::OrderID& eventId = eventOpInfo.eventId;
  macedbg(1) << "Recv enqueue subevent reply for Event("<< eventId <<")!" << Log::endl;
  ScopedLock sl(eventExecutingSyncMutex);
  std::map<mace::EventOperationInfo, pthread_cond_t*>::iterator cond_iter = eventExecutingSyncConds.find(eventOpInfo);
  if( cond_iter != eventExecutingSyncConds.end()  ) {
    pthread_cond_signal(cond_iter->second);
    macedbg(1) << "Signal Event("<< eventId <<") in " << this->contextName << Log::endl;
  }
}

void mace::ContextBaseClass::handleEnqueueExternalMessageReply(mace::EventOperationInfo const& eventOpInfo) {
  ADD_SELECTORS("ContextBaseClass::handleEnqueueExternalMessageReply");
  const mace::OrderID& eventId = eventOpInfo.eventId;
  macedbg(1) << "Recv enqueue enqueue message reply for Event("<< eventId <<")!" << Log::endl;
  ScopedLock sl(eventExecutingSyncMutex);
  std::map<mace::EventOperationInfo, pthread_cond_t*>::iterator cond_iter = eventExecutingSyncConds.find(eventOpInfo);
  if( cond_iter != eventExecutingSyncConds.end()  ) {
    pthread_cond_signal(cond_iter->second);
    macedbg(1) << "Signal Event("<< eventId <<") in " << this->contextName << Log::endl;
  }
}

void mace::ContextBaseClass::handleEnqueueOwnershipOpsReply(mace::EventOperationInfo const& eventOpInfo) {
  ADD_SELECTORS("ContextBaseClass::handleEnqueueOwnershipOpsReply");
  const mace::OrderID& eventId = eventOpInfo.eventId;
  macedbg(1) << "Recv enqueue oops reply for Event("<< eventId <<")!" << Log::endl;
  ScopedLock sl(eventExecutingSyncMutex);
  std::map<mace::EventOperationInfo, pthread_cond_t*>::iterator cond_iter = eventExecutingSyncConds.find(eventOpInfo);
  if( cond_iter != eventExecutingSyncConds.end()  ) {
    pthread_cond_signal(cond_iter->second);
    macedbg(1) << "Signal Event("<< eventId <<") in " << this->contextName << Log::endl;
  }
}

void mace::ContextBaseClass::handleEventExecutePermission(BaseMaceService* sv, const mace::vector<mace::EventOperationInfo>& eops, 
    const mace::vector<mace::string>& permitContexts ) {

  ADD_SELECTORS("ContextBaseClass::handleEventExecutePermission");
  macedbg(1) << "Recv permission in " << this->contextName << " for "<< eops <<" and could execute in " << permitContexts << Log::endl;
  
  for( uint32_t i=0; i<eops.size(); i++ ){
    const mace::EventOperationInfo& eventOpInfo = eops[i];
    if( i == 0 ){
      this->addEventPermitContexts( eventOpInfo.eventId, permitContexts );
    }
    ScopedLock order_sl(this->contextEventOrderMutex);
    if( eventOpInfo.toContextName == this->contextName ) { // get permit to execute in toContext
      ScopedLock sl(this->executeEventMutex);
      this->enqueueContextEvent(sv, eventOpInfo.eventId);
    } else {
      if( eventOpInfo.opType == mace::EventOperationInfo::ROUTINE_OP ){
        ScopedLock sl(eventExecutingSyncMutex);
        std::map<mace::EventOperationInfo, pthread_cond_t*>::iterator cond_iter = eventExecutingSyncConds.find(eventOpInfo);
        if( cond_iter != eventExecutingSyncConds.end()  ) { // it's a sync call which is blocked
          pthread_cond_signal(cond_iter->second);
          macedbg(1) << "Signal Event("<< eventOpInfo.eventId <<") in " << this->contextName << Log::endl;
        }
      } else if( eventOpInfo.opType == mace::EventOperationInfo::BROADCAST_OP ){ // Forward waiting async method
        ScopedLock sl(this->executeEventMutex);
        this->forwardWaitingBroadcastRequset(sv, eventOpInfo);
      } else {
        ASSERT(false);
      }
    }
  }
}

void mace::ContextBaseClass::handleEventCommitDone( BaseMaceService* sv, mace::OrderID const& eventId, 
    mace::set<mace::string> const& coaccess_contexts ){
  ADD_SELECTORS("ContextBaseClass::handleEventCommitDone");
  ASSERT( contextEventOrder.getExecuteEventTicket(eventId)>0 );

  mace::map< mace::string, mace::set<mace::string> > context_type_name_map;
  for( mace::set<mace::string>::const_iterator iter = coaccess_contexts.begin(); iter != coaccess_contexts.end(); iter++ ){
    mace::string context_type = Util::extractContextType( *iter );

    context_type_name_map[ context_type ].insert( *iter );
  }

  // mace::vector<mace::ElasticityRule> coaccess_rules = (this->eConfig).getCoaccessRules();
  // for( uint32_t i=0; i<coaccess_rules.size(); i++ ) {
  //   mace::vector< mace::string > coaccess_contexts_in_rule = coaccess_rules[i].getCoaccessContexts(context_type_name_map);

  //   for( uint32_t j=0; j<coaccess_contexts_in_rule.size(); j++ ){
  //     (this->runtimeInfo).addCoaccessContext( coaccess_contexts_in_rule[j] );
  //   }
  // } 
  
  this->enqueueCommitEventQueue(sv, eventId, false);
}

// void mace::ContextBaseClass::enqueueLockRequests(BaseMaceService* sv, mace::vector<mace::EventOperationInfo> const& eops) {
//   ADD_SELECTORS("ContextBaseClass::enqueueLockRequests");

//   if( eops.size() == 0 ){
//     return;
//   }
  
//   ContextService* _service = static_cast<ContextService*>(sv);
//   ContextStructure& contextStructure = _service->contextStructure;

//   contextStructure.getLock(ContextStructure::READER);
//   const mace::string dominatorName = contextStructure.getUpperBoundContextName(requireContextName);
//   if( dominatorName != requireContextName ){
//     contextStructure.releaseLock(ContextStructure::READER);
//     _service->send__event_enqueueLockRequests( dominatorName, requireContextName, eops );
//     return;
//   }
//   contextStructure.releaseLock(ContextStructure::READER);

//   mace::set<mace::string> p_ctx_names;
//   for( uint32_t i=0; i<eops.size(); i++ ){
//     this->checkEventExecutePermission( sv, requireContextName, eops[i], false, p_ctx_names );
//   }
// }


void mace::ContextBaseClass::unlockContext( BaseMaceService* sv, mace::EventOperationInfo const& eventOpInfo,
    mace::vector<mace::EventOperationInfo> const& localLockRequests, mace::vector<mace::string> const& lockedContexts ) {
  ADD_SELECTORS("ContextBaseClass::unlockContext");
  
  ContextService* _service = static_cast<ContextService*>(sv);
  ContextStructure& contextStructure = _service->contextStructure;

  ScopedLock dsl( this->contextDominatorMutex );  
  macedbg(1) << "Event("<< eventOpInfo.eventId <<") in context("<< this->contextName <<") unlock "<< eventOpInfo << Log::endl;

  contextStructure.getLock(ContextStructure::READER);
  const mace::string dominatorName = contextStructure.getUpperBoundContextName(eventOpInfo.fromContextName);

  mace::vector<mace::string> releaseContexts;
  mace::map< mace::string, mace::vector<mace::EventOperationInfo> > permittedEventOps;
  mace::map< mace::OrderID, mace::vector<mace::string> > permittedContextNames;

  if( eventOpInfo.toContextName == this->contextName ){ // ask fromContextName to unlock this context
    ScopedLock sl(eventExecutingSyncMutex);
    ASSERT( localLockRequests.size() == 0 && lockedContexts.size() == 0 );
    mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventOpInfo.eventId);
    ASSERT( iter != eventExecutionInfos.end() );

    mace::vector<mace::EventOperationInfo> local_lock_requests;
    mace::vector<mace::string> locked_contexts;

    // If it is a dominator, not submit local locks to fromContextName's dominator
    if( contextStructure.getUpperBoundContextName(eventOpInfo.toContextName) != eventOpInfo.toContextName ) {
      local_lock_requests = (iter->second).getLocalLockRequests();
      locked_contexts = (iter->second).getLockedChildren();
    
      (iter->second).clearLocalLockRequests();
      (iter->second).clearLockedChildren();
      macedbg(1) << "local_lock_requests: " << local_lock_requests << Log::endl;
      macedbg(1) << "locked_contexts: " << locked_contexts << Log::endl;
    }
    sl.unlock();
    contextStructure.releaseLock(ContextStructure::READER);
    dsl.unlock();
    _service->send__event_unlockContext(eventOpInfo.fromContextName, eventOpInfo, local_lock_requests, locked_contexts, this->contextName);
    return;
  } else if( eventOpInfo.hasAccessed(this->contextName) ) { // locally unlock request from descendants
    if( this->localUnlockContext( eventOpInfo, localLockRequests, lockedContexts) ) {
      contextStructure.releaseLock(ContextStructure::READER);
      return;
    }
  }

  if( dominatorName == this->contextName ){
    for( uint32_t i=0; i<localLockRequests.size(); i++ ) {
      dominator.checkEventExecutePermission( sv, localLockRequests[i] );
    }
    dominator.addLockedContexts( eventOpInfo.eventId, lockedContexts );

    if( !dominator.unlockContext( contextStructure, eventOpInfo, permittedEventOps, permittedContextNames, releaseContexts ) ) {
      macedbg(1) << "Fail to unlock ("<< eventOpInfo <<") in dominator("<< this->contextName <<")!" << Log::endl;
      dominator.addWaitingUnlockRequest( eventOpInfo );
    }

    if( localLockRequests.size() > 0 ) {
      dominator.unlockContextByWaitingRequests(contextStructure, eventOpInfo.eventId, permittedEventOps, permittedContextNames, 
        releaseContexts);
    }
    contextStructure.releaseLock(ContextStructure::READER);
    dsl.unlock();
    
    this->notifyReleasedContexts( _service, eventOpInfo.eventId, releaseContexts );
    this->notifyNextExecutionEvents( _service, permittedEventOps, permittedContextNames );

    return;
    
  } else  { // continue to pass unlock request to its parent context
    mace::string nextUpContextName = eventOpInfo.getPreAccessContextName( this->contextName );
    if( nextUpContextName == "" ){ // This context is the target context
      nextUpContextName = contextStructure.getUpperBoundContextName( this->contextName );
    }
    macedbg(1) << "Send msg to unlock eventOp("<< eventOpInfo <<") to context("<< nextUpContextName <<") with localLockRequests=" << localLockRequests << Log::endl;
    
    contextStructure.releaseLock(ContextStructure::READER);
    dsl.unlock();
    _service->send__event_unlockContext(nextUpContextName, eventOpInfo, localLockRequests, lockedContexts, this->contextName);
    return;
  } 

  
}

void mace::ContextBaseClass::releaseContext( BaseMaceService* sv, const mace::OrderID& eventId, const mace::string& lockedContextName,
    const mace::string& srcContext, const mace::vector<mace::EventOperationInfo>& localLockRequests, 
    const mace::vector<mace::string>& lockedContexts ) {

  ADD_SELECTORS("ContextBaseClass::releaseContext");
  
  ContextService* _service = static_cast<ContextService*>(sv);
  ContextStructure& contextStructure = _service->contextStructure;

  ScopedLock dsl( this->contextDominatorMutex );
  macedbg(1) << "Context("<< this->contextName <<") release locked context("<< lockedContextName <<") for " << eventId << Log::endl;
  contextStructure.getLock(ContextStructure::READER);

  const mace::string dominatorContext = contextStructure.getUpperBoundContextName(lockedContextName);

  mace::string srcDominator = contextStructure.getUpperBoundContextName(srcContext);
  ASSERT(srcDominator != "");

  mace::vector<mace::string> releaseContexts;
  mace::map< mace::string, mace::vector<mace::EventOperationInfo> > permittedEventOps;
  mace::map< mace::OrderID, mace::vector<mace::string> > permittedContextNames;

  if( lockedContextName == this->contextName ) {
    ScopedLock sl(eventExecutingSyncMutex);
    mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
    ASSERT( iter != eventExecutionInfos.end() );
    ASSERT( localLockRequests.size() == 0 && lockedContexts.size() == 0 );
      
    mace::vector<mace::EventOperationInfo> local_lock_requests = (iter->second).getLocalLockRequests();
    mace::vector<mace::string> locked_contexts = (iter->second).getLockedChildren();

    macedbg(1) << "Event("<< eventId <<") local_lock_requests: " << local_lock_requests << Log::endl;
    macedbg(1) << "Event("<< eventId <<") locked_contexts: " << locked_contexts << Log::endl;

    locked_contexts.push_back(this->contextName);
    
    (iter->second).clearLocalLockRequests();
    (iter->second).clearLockedChildren();
    sl.unlock();

    if( dominatorContext == this->contextName ) {
      for( uint32_t i=0; i<local_lock_requests.size(); i++ ) {
        dominator.checkEventExecutePermission( sv, local_lock_requests[i] );
      }
      dominator.addLockedContexts( eventId, locked_contexts );

      if( localLockRequests.size() > 0 ){
        dominator.unlockContextByWaitingRequests(contextStructure, eventId, permittedEventOps, permittedContextNames, releaseContexts);
      }

      dominator.releaseContext( contextStructure, eventId, lockedContextName, srcDominator, permittedEventOps, 
        permittedContextNames, releaseContexts );
    } else {
      dsl.unlock();
      contextStructure.releaseLock(ContextStructure::READER);
      _service->send__event_releaseContext( dominatorContext, eventId, lockedContextName, local_lock_requests, locked_contexts,
        this->contextName );
      return;
    }
    
  } else if( dominatorContext == this->contextName ){ // in the dominator
    for( uint32_t i=0; i<localLockRequests.size(); i++ ) {
      dominator.checkEventExecutePermission( sv, localLockRequests[i] );
    }
    dominator.addLockedContexts( eventId, lockedContexts );
    
    if( localLockRequests.size() > 0 ){
      dominator.unlockContextByWaitingRequests(contextStructure, eventId, permittedEventOps, permittedContextNames, releaseContexts);
    }   

    dominator.releaseContext( contextStructure, eventId, lockedContextName, srcDominator, permittedEventOps, permittedContextNames, 
      releaseContexts );
  }
  dsl.unlock();
  contextStructure.releaseLock(ContextStructure::READER);

  this->notifyReleasedContexts( _service, eventId, releaseContexts );
  this->notifyNextExecutionEvents( _service, permittedEventOps, permittedContextNames );
}

void mace::ContextBaseClass::handleEventOperationDone( BaseMaceService const* sv, mace::EventOperationInfo const& eop ) {
  ADD_SELECTORS("ContextBaseClass::handleEventOperationDone");
  ContextService const* _service = static_cast<ContextService const*>(sv);

  ContextService* _non_service = const_cast<ContextService*>(_service);

  mace::vector<mace::EventOperationInfo> local_lock_requests;
  mace::vector<mace::string> local_locked_contexts;
  this->unlockContext( _non_service, eop, local_lock_requests, local_locked_contexts );
}

void mace::ContextBaseClass::getReadyToCommit( BaseMaceService* sv, mace::OrderID const& eventId ) {
  ADD_SELECTORS("ContextBaseClass::getReadyToCommit");

  if( checkEventIsLockContext(eventId) ) {
    macedbg(1) << "context("<< this->contextName <<") is still locked by " << eventId << Log::endl;
    return;
  }
  
  ContextService* _service = static_cast<ContextService*>(sv);
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  if( iter == eventExecutionInfos.end() ) {
    maceerr << "Fail to find event("<< eventId <<")'s info in " << this->contextName << Log::endl;
    ASSERT(false);
    return;
  }
  
  const mace::ContextMapping& snapshot = _service->getLatestContextMapping();

  mace::EventExecutionInfo& eeInfo = iter->second;
  if( eeInfo.getToContextSize() == 0 ){
    macedbg(1) << "Event("<< eventId <<") could commit in "<< this->contextName << Log::endl;
    eeInfo.setAlreadyCommittedFlagTrue();
    if( eeInfo.targetContextName == this->contextName ){ // this event could commit in target context
      sl.unlock();
      this->enqueueCommitEventQueue( sv, eventId, true );
      return;
    }

    const mace::set<mace::string>& executedContexts = eeInfo.getToContextsCopy();
    mace::vector<mace::string> v_executedContexts;
    for( mace::set<mace::string>::const_iterator cIter = executedContexts.begin(); cIter != executedContexts.end(); cIter++ ){
      v_executedContexts.push_back( *cIter );
    }

    const mace::set<mace::string>& fromContexts = eeInfo.getFromContexts();
    ASSERT( fromContexts.size() > 0 );
    mace::map< mace::MaceAddr, mace::vector<mace::string> > fromContextAddrs;
    for( mace::set<mace::string>::const_iterator cIter = fromContexts.begin(); cIter != fromContexts.end(); cIter ++ ) {
      const mace::string& fromContext = *cIter;
      ASSERT( fromContext != this->contextName );
      
      const mace::MaceAddr& addr = mace::ContextMapping::getNodeByContext(snapshot, fromContext );
      fromContextAddrs[addr].push_back(fromContext);
    }

    mace::map< mace::MaceAddr, mace::vector<mace::string> >::const_iterator from_iter = fromContextAddrs.begin();
    for(; from_iter != fromContextAddrs.end(); from_iter ++ ){
      _service->send__event_notifyReadyToCommit(from_iter->first, eventId, this->contextName, from_iter->second, v_executedContexts );
    }
  } else {
    macedbg(1) << "In context("<< this->contextName <<"), event("<< eventId <<") toContextNames: " << eeInfo.getToContexts() << Log::endl;
  }
}

void mace::ContextBaseClass::notifyReadyToCommit( BaseMaceService* sv, mace::OrderID const& eventId, mace::string const& toContext, 
    mace::vector<mace::string> const& executedContextNames ) {
  ADD_SELECTORS("ContextBaseClass::notifyReadyToCommit");
  macedbg(1) << "Event("<< eventId <<") toContext=" << toContext << " executedContextNames=" << executedContextNames << " in " << this->contextName <<Log::endl; 
  ScopedLock rsl(this->releaseLockMutex);

  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  ASSERT( iter!= eventExecutionInfos.end() );
  //const mace::ContextMapping& snapshot = _service->getLatestContextMapping();

  mace::EventExecutionInfo& eeInfo = iter->second;
  eeInfo.eraseToContext( toContext );
  for( uint32_t i=0; i<executedContextNames.size(); i++ ){
    eeInfo.addEventToContextCopy( executedContextNames[i] );
  }
  sl.unlock();

  this->getReadyToCommit( sv, eventId );
}

// void mace::ContextBaseClass::updateDominator( BaseMaceService* sv, const mace::string& start_dominator, const uint64_t& ver, 
//     const mace::string& src_ctx_name, const mace::vector<mace::EventOperationInfo>& recv_eops, 
//     const mace::set<mace::string>& affected_contexts ) {
//   ADD_SELECTORS("ContextBaseClass::updateDominator");
//   ASSERT( affected_contexts.count(this->contextName) > 0 );
      
//   ContextService* _service = static_cast<ContextService*>(sv);
//   ContextStructure& ctxStructure = _service->contextStructure;

//   const mace::ContextMapping& snapshot = _service->getLatestContextMapping();
  
//   ctxStructure.getLock(ContextStructure::READER);
//   ScopedLock sl(eventExecutingSyncMutex);
  
//   dominator.addUpdateSourceContext(src_ctx_name);
//   mace::vector<mace::EventOperationInfo> eops, forward_eops, reply_eops;
//   if( dominator.updateDominator( start_dominator, ver, ctxStructure, eops) ) {
//     this->clearEventPermitCacheNoLock();
    
//     mace::set<mace::string> next_conetxts;
//     mace::vector<mace::string> children = ctxStructure.getAllChildContexts( this->contextName );
//     for( uint32_t i=0; i<children.size(); i++ ) {
//       if( affected_contexts.count(children[i]) > 0 ) {
//         next_contexts.insert(affected_contexts[i]);
//       }
//     }

//     mace::string pre_dominator = dominator.getPreDominator();
//     mace::string cur_dominator = dominator.getCurDominator();

//     if( pre_dominator == this->contextName ) { // it is a dominator before
//       if( cur_dominator == this->contextName ) { // it is still a dominator but dominates fewer or more contexts
//         forward_eops = recv_eops;
//         for( uint32_t i=0; i<eops.size(); i++ ) { // forward_eops.size > 0, this dominator dominates fewer contexts
//           forward_eops.push_back(eops[i]);
//         }
//       } else { // not dominator any more
//         // forward eops to new dominator
//         for( uint32_t i=0; i<eops.size(); i++ ) {
//           reply_eops.push_back( eops[i] );
//         }
//       }
//     } else if( cur_dominator == this->contextName ) { // non-dominator -> dominator
//       // add eops from old dominator
//       for( uint32_t i=0; i<recv_eops.size(); i++ ) {
//         if( ctxStructure.getUpperBoundContextName(recv_eops[i].requireContextName) == this->contextName ) {
//           dominator.checkEventExecutePermission(sv, recv_eops[i]);
//         } else {
//           // continue to forward eops from old dominator
//           forward_eops.push_back(sv, recv_eops[i]);
//         }
//       }

//     } else {
//       ASSERT(false);
//     }

//     dominator.addReplyEventOps(reply_eops);

//     // inform child affected contexts to update their dominators
//     mace::map< mace::MaceAddr, mace::set<mace::string> > dom_update_addrs;

//     for( mace::set<mace::string>::iterator iter = next_contexts.begin(); iter != next_contexts.end(); iter ++ ){
//       const mace::MaceAddr& destAddr = ContextMapping::getNodeByContext(snapshot, *iter );
//       dom_update_addrs[destAddr].insert(*iter);
//       dominator.addUpdateWaitingContext(*iter);
//     }
    
//     for(mace::map< mace::MaceAddr, mace::set<mace::string> >::const_iterator addrIter = dom_update_addrs.begin(); 
//         addrIter != dom_update_addrs.end(); addrIter ++ ){
//       _service->send__event_updateOwnershipAndDominators( addrIter->first, addrIter->second, affected_contexts, forward_eops, 
//         ctxStructure.getDominateRegionDAG(), ctxStructure.getCurrentVersion() );
//     }
//   }

//   if( !dominator.isUpdateWaitingForReply() ) {
//     mace::set<mace::string> src_contexts = dominator.getUpdateSourceContexts();
//     for( mace::set<mace::string>::iterator iter = src_contexts.begin(); iter != src_contexts.end(); iter ++ ){
//       _service->send__event_updateOwnershipReply( *iter, this->contextName, dominator.getReplyEventOps() );
//     }
//     dominator.clearUpdateSourceContexts();
//     dominator.clearUpdateReplyEventOps();
//   }
//   ctxStructure.releaseLock(ContextStructure::READER);
// }

mace::vector<mace::EventOperationInfo> mace::ContextBaseClass::updateDominator( BaseMaceService* sv, 
    const mace::string& src_contextName, const mace::vector<mace::EventOperationInfo>& recv_eops ) {
  ADD_SELECTORS("ContextBaseClass::updateDominator");
  
  macedbg(1) << "To update context("<< this->contextName <<") from dominator("<< src_contextName <<") with " << recv_eops << Log::endl;
  ContextService* _service = static_cast<ContextService*>(sv);
  ContextStructure& ctxStructure = _service->contextStructure;

  // const mace::ContextMapping& snapshot = _service->getLatestContextMapping();
  ScopedLock sl(this->contextDominatorMutex);
  ctxStructure.getLock(ContextStructure::READER);

  mace::vector<mace::EventOperationInfo> reply_eops;
  dominator.updateDominator( ctxStructure, reply_eops);
  this->clearEventPermitCacheNoLock();

  mace::string pre_dominator = dominator.getPreDominator();
  mace::string cur_dominator = dominator.getCurDominator();

  if( pre_dominator == this->contextName ) { // it is a dominator before
    if( cur_dominator == this->contextName ) { // it is still a dominator but dominates fewer or more contexts
      
    } else { // not dominator any more
      ScopedLock esl( this->eventExecutingSyncMutex );
      mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo( currOwnershipEventOp.eventId );
      if( iter != eventExecutionInfos.end() ) {
        mace::vector<mace::string> locked_contexts = (iter->second).getLockedChildren();
        for( uint32_t i=0; i<locked_contexts.size(); i++ ) {
          mace::EventOperationInfo eop;
          eop.eventId = currOwnershipEventOp.eventId;
          eop.requireContextName = this->contextName;
          eop.fromContextName = this->contextName;
          eop.toContextName = locked_contexts[i];

          reply_eops.push_back(eop);
        }
      }
      esl.unlock();
    }
  } else if( cur_dominator == this->contextName ) { // non-dominator -> dominator
    ASSERT( currOwnershipEventOp.eventOpType == mace::Event::EVENT_OP_OWNERSHIP 
      && currOwnershipEventOp.toContextName == this->contextName );
    if( currOwnershipEventOp.fromContextName != currOwnershipEventOp.toContextName ) {
      dominator.checkEventExecutePermission(sv, currOwnershipEventOp);
    }
    // add eops from old dominator
    for( uint32_t i=0; i<recv_eops.size(); i++ ) {
      if( ctxStructure.getUpperBoundContextName(recv_eops[i].requireContextName) == this->contextName || 
          recv_eops[i].toContextName == this->contextName ) {
        dominator.checkEventExecutePermission(sv, recv_eops[i]);
      }
    }
  }
  // _service->send__event_updateOwnershipReply( src_contextName, this->contextName, reply_eops );
  
  ctxStructure.releaseLock(ContextStructure::READER);
  return reply_eops;
}

void mace::ContextBaseClass::handleOwnershipUpdateDominatorReply( BaseMaceService* sv, 
    const mace::set<mace::string>& src_contextNames, const mace::vector<mace::EventOperationInfo>& eops) {
  ADD_SELECTORS("ContextBaseClass::handleOwnershipUpdateDominatorReply");
  ContextService* _service = static_cast<ContextService*>(sv);
  ContextStructure& ctxStructure = _service->contextStructure;

  const mace::ContextMapping& snapshot = _service->getLatestContextMapping();
  
  ctxStructure.getLock(ContextStructure::READER);
  ScopedLock sl(eventExecutingSyncMutex);

  macedbg(1) << "context("<< this->contextName <<") receive from context("<< src_contextNames <<") eops: " << eops << Log::endl;
  for( uint32_t i=0; i<eops.size(); i++ ) {
    ASSERT( ctxStructure.getUpperBoundContextName(eops[i].requireContextName) == this->contextName );
    dominator.addLockedContext( eops[i].eventId, eops[i].toContextName );
  }

  for( mace::set<mace::string>::const_iterator iter = src_contextNames.begin(); iter != src_contextNames.end(); iter ++ ) {
    dominator.removeUpdateWaitingContext(*iter);
  }

  if( !dominator.isUpdateWaitingForReply() ) {
    mace::set<mace::string> src_contexts = dominator.getUpdateSourceContexts();
    mace::vector<mace::EventOperationInfo> src_eops = dominator.getUpdateReplyEventOps();
    // macedbg(1) << "src_contexts: " << src_contexts << Log::endl;
    // macedbg(1) << "src_eops: " << src_eops << Log::endl;
    ASSERT( src_contexts.size() == 1 && src_eops.size() == 1 );
    const mace::string& src_ctx_name = *(src_contexts.begin());

    const mace::MaceAddr& replyAddr = ContextMapping::getNodeByContext(snapshot, src_ctx_name );
    _service->send__event_modifyOwnershipReply( replyAddr, src_ctx_name, src_eops[0] );

    dominator.clearUpdateSourceContexts();
    dominator.clearUpdateReplyEventOps();
  }

  ctxStructure.releaseLock(ContextStructure::READER);
}

// void mace::ContextBaseClass::updateEventQueue(BaseMaceService* sv, mace::string const& src_contextName, 
//     mace::ContextEventDominator::EventLockQueue const& lockQueues ) {
//   ADD_SELECTORS("ContextBaseClass::updateEventQueue");
//   mace::vector<DominatorRequest> dominatorRequests;
//   if( dominator.updateEventQueue(sv, src_contextName, lockQueues, dominatorRequests) ){
//     ContextService* _service = static_cast<ContextService*>(sv);
//     mace::set<mace::string> ctxNames;
//     ctxNames.insert( this->contextName);
//     _service->send__event_updateOwnershipReply(ctxNames);

//     dominator.clearWaitingDominatorRequests();
//     this->handleWaitingDominatorRequests( sv, dominatorRequests );
//   }
// }

void mace::ContextBaseClass::addEventToContext( mace::OrderID const& eventId, mace::string const& toContext ) {
  ADD_SELECTORS("ContextBaseClass::addEventToContext");
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  ASSERT( iter != eventExecutionInfos.end() );
  macedbg(1) << "Add event("<< eventId <<")'s new toContext("<< toContext <<") in " << this->contextName << Log::endl;
  (iter->second).addEventToContext( toContext );
}


void mace::ContextBaseClass::addEventFromContext( mace::OrderID const& eventId, mace::string const& fromContext ) {
  ADD_SELECTORS("ContextBaseClass::addEventFromContext");
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  ASSERT( iter != eventExecutionInfos.end() );
  // record all executed context names
  (iter->second).addEventFromContext( fromContext );
}

void mace::ContextBaseClass::addChildEventOp( const mace::EventOperationInfo& eventOpInfo ) {
  ADD_SELECTORS("ContextBaseClass::addChildEventOp");
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventOpInfo.eventId);
  ASSERT( iter != eventExecutionInfos.end() );
  (iter->second).enqueueLocalLockRequest( eventOpInfo );
  macedbg(1) << "context("<< this->contextName <<") add lock eventOp: " << eventOpInfo << Log::endl;
}

mace::set<mace::string> mace::ContextBaseClass::getEventToContextNames( mace::OrderID const& eventId ) {
  ADD_SELECTORS("ContextBaseClass::getEventToContextNames");
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  ASSERT( iter != eventExecutionInfos.end() );
  // record all executed context names
  return (iter->second).getToContextNames();
}

mace::vector< mace::EventOperationInfo > mace::ContextBaseClass::getAllLocalLockRequests( ) {
  ADD_SELECTORS("ContextBaseClass::getAllLocalLockRequest");
  ScopedLock sl(eventExecutingSyncMutex);
  mace::vector< mace::EventOperationInfo > all_eops;

  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter;
  for( iter = eventExecutionInfos.begin(); iter != eventExecutionInfos.end(); iter ++ ){
    mace::vector<mace::EventOperationInfo> eops = (iter->second).getLocalLockRequests();
    for( uint64_t i=0; i<eops.size(); i++ ) {
      all_eops.push_back( eops[i] );
    }
  }
  return all_eops;
}

mace::vector< mace::EventOperationInfo > mace::ContextBaseClass::getLocalLockRequests( mace::OrderID const& eventId ) {
  ADD_SELECTORS("ContextBaseClass::getLocalLockRequest");
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  ASSERT( iter != eventExecutionInfos.end() );
  
  return (iter->second).getLocalLockRequests();
}

mace::vector< mace::string > mace::ContextBaseClass::getLockedChildren( mace::OrderID const& eventId ) {
  ADD_SELECTORS("ContextBaseClass::getLockedChildren");
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  ASSERT( iter != eventExecutionInfos.end() );
  
  return (iter->second).getLockedChildren();
}

void mace::ContextBaseClass::clearLocalLockRequests( mace::OrderID const& eventId ) {
  ADD_SELECTORS("ContextBaseClass::clearLocalLockRequests");
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  ASSERT( iter != eventExecutionInfos.end() );
  // record all executed context names
  (iter->second).clearLocalLockRequests();
}

void mace::ContextBaseClass::clearLockedChildren( mace::OrderID const& eventId ) {
  ADD_SELECTORS("ContextBaseClass::clearLockedChildren");
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<mace::OrderID, mace::EventExecutionInfo>::iterator iter = getEventExecutionInfo(eventId);
  ASSERT( iter != eventExecutionInfos.end() );
  // record all executed context names
  (iter->second).clearLockedChildren();
}

void mace::ContextBaseClass::releaseLock( BaseMaceService* sv, mace::OrderID const& eventId) {
  ADD_SELECTORS("ContextBaseClass::releaseLock");
  macedbg(1) << "Release lock of event("<< eventId <<") on " << this->contextName << Log::endl;
  int8_t lock_flag;
  
  ScopedLock rsl(this->releaseLockMutex);
  // release lock on this context
  ScopedLock sl(_context_ticketbooth);
  if( this->readerEvents.count(eventId) > 0 ) {
    lock_flag = mace::ContextLock::RELEASE_READ_MODE;
  } else if(this->writerEvents.count(eventId) > 0 ) {
    lock_flag = mace::ContextLock::RELEASE_WRITE_MODE;
  } else {
    maceerr << "Event("<< eventId <<") doesn't lock " << this->contextName << Log::endl;
    return;
  }
  sl.unlock();

  mace::ContextLock _lock( *this, eventId, true, lock_flag);

  this->getReadyToCommit(sv, eventId);
}

bool mace::ContextBaseClass::checkEventIsLockContext( mace::OrderID const& eventId ) {
  ScopedLock sl(_context_ticketbooth);
  if( this->readerEvents.count(eventId) > 0 || this->writerEvents.count(eventId) > 0 ) {
    return true;
  } else {
    return false;
  }
}

void mace::ContextBaseClass::addReaderEvent(mace::OrderID const& eventId) {
  if( readerEvents.count(eventId) == 0 ) {
    readerEvents.insert(eventId);
  }
}

void mace::ContextBaseClass::addWriterEvent(mace::OrderID const& eventId) {
  if( writerEvents.count(eventId) == 0 ) {
    writerEvents.insert(eventId);
  }
}

void mace::ContextBaseClass::removeReaderEvent(mace::OrderID const& eventId) {
  readerEvents.erase(eventId);
}
void mace::ContextBaseClass::removeWriterEvent(mace::OrderID const& eventId) {
  ADD_SELECTORS("ContextBaseClass::removeWriterEvent");
  macedbg(1) << "context("<< this->contextName <<") removes writer event("<< eventId <<")!" << Log::endl;
  writerEvents.erase(eventId);
}

bool mace::ContextBaseClass::checkReEnterEvent(mace::OrderID const& eventId) const {
  ADD_SELECTORS("ContextBaseClass::checkReEnterEvent");

  if( readerEvents.count(eventId)!=0 || writerEvents.count(eventId) != 0 ) {
    return true;
  } else {
    return false;
  }
}

void mace::ContextBaseClass::notifyExecutedContexts(mace::OrderID const& eventId ) {
  ADD_SELECTORS("ContextBaseClass::notifyExecutedContexts");
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map< mace::OrderID, EventExecutionInfo >::iterator iter = getEventExecutionInfo(eventId);
  ASSERT( iter != eventExecutionInfos.end() );
  mace::set<mace::string> executedContextNames = (iter->second).getToContextsCopy();
  mace::set<mace::string>::iterator sIter = executedContextNames.begin();
  AsyncEventReceiver* sv = BaseMaceService::getInstance(this->serviceId);
  ContextService* _service = static_cast<ContextService*>(sv);
  for(; sIter != executedContextNames.end(); sIter ++ ) {
    const mace::string& ctxName = *sIter;
    if( ctxName == this->contextName ) {
      continue;
    }
    macedbg(1) << "Inform context("<< ctxName <<") to commit event("<< eventId <<") from " << this->contextName << Log::endl;
    _service->send__event_CommitDoneMsg(this->contextName, ctxName, eventId, executedContextNames);
  }
  //eventExecutionInfos.erase(iter);
}

mace::map<mace::OrderID, EventExecutionInfo>::iterator ContextBaseClass::getEventExecutionInfo( const mace::OrderID& eventId ){
  ADD_SELECTORS("ContextBaseClass::getEventExecutionInfo");

  mace::map<mace::OrderID, EventExecutionInfo>::iterator iter = eventExecutionInfos.find(eventId);
  if( iter == eventExecutionInfos.end() ){
    for( mace::map<mace::OrderID, EventExecutionInfo>::iterator iter2=eventExecutionInfos.begin(); iter2!=eventExecutionInfos.end(); iter2++ ){
      if( iter2->first == eventId ){
        iter = iter2;
        ASSERTMSG(false, "How this could happen!!!!");
        break;
      }
    }
  }  
  return iter;
}

void ContextBaseClass::deleteEventExecutionInfo( OrderID const& eventId ) { 
  ADD_SELECTORS("ContextBaseClass::deleteEventExecutionInfo");
  macedbg(1) << "Remove event("<< eventId <<") executionInfo in " << this->contextName << Log::endl;
  ScopedLock sl(eventExecutingSyncMutex);
  mace::map<mace::OrderID, EventExecutionInfo>::iterator iter = this->getEventExecutionInfo( eventId );
  if( iter != eventExecutionInfos.end() ){
    eventExecutionInfos.erase(eventId); 
    // deletedEventIds.push_back(eventId);
  }
}

mace::ElasticityBehaviorAction* ContextBaseClass::proposeAndCheckMigrationAction(const mace::ContextMapping& snapshot, 
    const mace::string& marker, const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info, 
    mace::map< mace::MaceAddr, double >& servers_cpu_usage, mace::map< mace::MaceAddr, double >& servers_cpu_time, 
    mace::map< mace::MaceAddr, mace::set<mace::string> >& migration_ctx_sets){
  
  ADD_SELECTORS("ContextBaseClass::proposeAndCheckMigrationAction");
  macedbg(1) << "To propose and check migration actions of context("<< this->contextName <<")!" << Log::endl;
  
  const mace::vector< mace::ElasticityRule >& rules = eConfig.getElasticityRules();
  std::vector<mace::ElasticityBehaviorAction*> actions;

  for( mace::vector< mace::ElasticityRule >::const_iterator iter = rules.begin(); iter != rules.end(); iter ++ ) {
    const mace::ElasticityRule& rule = *iter;
    if( rule.ruleType == mace::ElasticityRule::SLA_ACTOR_MARKER_RULE ) {
      if( rule.isSLAMaxValueRule() ) {
        double max_latency = rule.getSLAMaxValue();
        double cur_latency = runtimeInfo.getMarkerAvgLatency(marker) * 0.000001;

        if( cur_latency/max_latency <= 1 ) {
          continue;
        }

        double ctx_exec_time = runtimeInfo.getCPUTime() * 0.000001;
        uint64_t count = runtimeInfo.getMarkerCount( marker );
        double local_cpu_usage = servers_cpu_usage[Util::getMaceAddr()];
        double local_cpu_time = servers_cpu_time[Util::getMaceAddr()];
        double migrated_local_cpu = 100 * ( local_cpu_usage*local_cpu_time*0.01 - ctx_exec_time ) / local_cpu_time;
          
        mace::map<mace::string, uint64_t> ctxs_inter_count = runtimeInfo.getContextInteractionCount();
        mace::map<mace::string, uint64_t> ctxs_inter_size = runtimeInfo.getEstimateContextsInteractionSize();

        if( ctxs_inter_size.size() == 0 || ctxs_inter_count.size() == 0 ){
          continue;
        }
        
        double ctx_cpu_usage = ctx_exec_time*100/servers_cpu_time[Util::getMaceAddr()];
        macedbg(1) << "ctx_exec_time=" << ctx_exec_time / count << Log::endl;

        double local_msg_count1 = 0.0;
        double remote_msg_size1 = 0.0;
        double remote_msg_count1 = 0.0;
        
        for( mace::map<mace::string, uint64_t>::iterator iter = ctxs_inter_count.begin(); iter != ctxs_inter_count.end(); iter ++ ){
          bool flag = false;
          // This context is planed to be migrated
          for( mace::map< mace::MaceAddr, mace::set<mace::string> >::const_iterator s_iter = migration_ctx_sets.begin(); 
              s_iter != migration_ctx_sets.end(); s_iter++ ) {
            if( (s_iter->second).count(iter->first) > 0 ) {
              remote_msg_size1 += ctxs_inter_size[iter->first];
              remote_msg_count1 += iter->second;
              flag = true;
              break;
            }
          }

          if(flag) continue;

          if( mace::ContextMapping::getNodeByContext( snapshot, iter->first) != Util::getMaceAddr() ) {
            remote_msg_size1 += ctxs_inter_size[iter->first];
            remote_msg_count1 += iter->second;
            continue;
          }

          local_msg_count1 += iter->second;
        }

        if( count > 1 ){
          local_msg_count1 /= count;
          remote_msg_size1 /= count;
          remote_msg_count1 /= count;
        }

        double benefit_percent = 0.0;
        mace::MaceAddr target_addr;
        // double expect_benefit = cur_latency - max_latency;

        for( mace::map< mace::MaceAddr, double >::const_iterator iter = servers_cpu_usage.begin(); iter != servers_cpu_usage.end(); 
            iter ++ ) {
          if( Util::getMaceAddr() == iter->first ){
            continue;
          }

          double remote_cpu_usage = servers_cpu_usage[iter->first];
          double remote_cpu_time = servers_cpu_time[iter->first];

          double migrated_remote_cpu = 100 * ( remote_cpu_usage*remote_cpu_time*0.01 + ctx_exec_time ) / remote_cpu_time;
          macedbg(1) << "context("<< this->contextName <<") migrated_local_cpu("<< Util::getMaceAddr() <<")=" << migrated_local_cpu << ", migrated_remote_cpu("<< iter->first <<")=" << migrated_remote_cpu << Log::endl;
      
          double local_msg_count2 = 0.0;
          double remote_msg_size2 = 0.0;
          double remote_msg_count2 = 0.0;
          
          mace::map< mace::MaceAddr, mace::set<mace::string> >::const_iterator mc_iter = migration_ctx_sets.find( iter->first );
          for( mace::map<mace::string, uint64_t>::iterator cc_iter = ctxs_inter_count.begin(); cc_iter != ctxs_inter_count.end(); 
              cc_iter ++ ){
            if( ( mc_iter != migration_ctx_sets.end() && (mc_iter->second).count(cc_iter->first) > 0 ) || 
                mace::ContextMapping::getNodeByContext( snapshot, cc_iter->first) == iter->first ) {
              local_msg_count2 += cc_iter->second;
            } else {
              remote_msg_size2 += ctxs_inter_size[cc_iter->first];
              remote_msg_count2 += cc_iter->second;
            }
          }

          if( count > 1 ){
            local_msg_count2 /= count;
            remote_msg_size2 /= count;
            remote_msg_count2 /= count;
          }

          macedbg(1) << "context("<< this->contextName <<"): remote_msg_count1=" << remote_msg_count1
                      << ", remote_msg_size1=" << remote_msg_size1
                      << ", remote_msg_count2=" << remote_msg_count2
                      << ", remote_msg_size2=" << remote_msg_size2 << Log::endl;

          double est_benefit_percent1 = mace::eMonitor::estimateBenefitPercent( 0, ctx_cpu_usage, local_cpu_usage, 
            local_msg_count1, remote_msg_size1, migrated_remote_cpu, local_msg_count2, remote_msg_size2 );

          // double est_benefit_percent2 = mace::eMonitor::estimateBenefitPercent( expect_benefit, ctx_cpu_usage, local_cpu_usage, 
          //   local_msg_count1, remote_msg_size1, migrated_remote_cpu, local_msg_count2, remote_msg_size2 );

          double est_benefit_percent2 = mace::eMonitor::estimateBenefitPercent( 0, ctx_cpu_usage, migrated_remote_cpu, 
            local_msg_count2, remote_msg_size2, local_cpu_usage, local_msg_count1, remote_msg_size1 );          

          if( cur_latency < max_latency ){
            est_benefit_percent1 = 1.0 - est_benefit_percent1;
            est_benefit_percent2 = 1.0 - est_benefit_percent2;
          }

          mace::map<mace::MaceAddr, ServerRuntimeInfo>::const_iterator sinfo_iter = servers_info.find(iter->first);
          ASSERT( sinfo_iter != servers_info.end() );
          double migration_threshold = (sinfo_iter->second).contextMigrationThreshold;

          macedbg(1) << "context("<< this->contextName <<"): cur_latency=" << cur_latency << ", max_latency=" << max_latency << ", migration_threshold=" << migration_threshold
                    << ", est_benefit_percent1=" << est_benefit_percent1 << ", est_benefit_percent2=" << est_benefit_percent2 << Log::endl;

          double est_benefit_percent = est_benefit_percent1 - est_benefit_percent2;
          if( est_benefit_percent >= migration_threshold /*&& est_benefit_percent2 < migration_threshold*/ && 
              (est_benefit_percent - migration_threshold ) > benefit_percent ) {

            benefit_percent = est_benefit_percent - migration_threshold;
            target_addr = iter->first;
          }  
        }

        if( benefit_percent > 0 ) {
          macedbg(1) << "context("<< this->contextName <<") will migrate to server("<< target_addr <<") with benefit("<< benefit_percent <<")!" << Log::endl;
          ElasticityBehaviorAction* action = new ElasticityBehaviorAction( mace::ElasticityBehaviorAction::EBACTION_MIGRATION, 
            target_addr, this->contextName, benefit_percent );
          action->curLatency = cur_latency;
          action->threshold = max_latency;
          action->specialRequirement = mace::MigrationContextInfo::MAX_LATENCY;

          actions.push_back(action);
        }
      }
    }
  }

  double largest_benefit_per = 0.0;
  ElasticityBehaviorAction* action = NULL;
  for( uint64_t i=0; i<actions.size(); i++ ) {
    if( actions[i]->benefit > largest_benefit_per ) {
      action = actions[i];
    }
  }

  for( uint64_t i=0; i<actions.size(); i++ ) {
    if( actions[i] != action ) {
      delete actions[i];
    }
    actions[i] = NULL;
  }

  return action;
}

void mace::ContextBaseClass::markMigrationTicket( const uint64_t& ticket ) {
  ScopedLock sl(createEventMutex);
  if( skipCreateTickets.count(ticket) == 0 ){
    skipCreateTickets.insert(ticket);
  }
}

mace::ElasticityBehaviorAction* mace::ContextBaseClass::getImprovePerformanceAction( mace::ContextMapping const& snapshot, 
      mace::map<mace::MaceAddr, mace::ServerRuntimeInfo> const& servers_info, mace::string const& marker,
      const mace::map< mace::MaceAddr, mace::set<mace::string> >& migration_ctx_sets, 
      const mace::map< mace::MaceAddr, double >& servers_cpu_usage, const mace::map< mace::MaceAddr, double >& servers_cpu_time ) {
  ADD_SELECTORS("ContextBaseClass::getImprovePerformanceAction");
  double latency = runtimeInfo.getMarkerAvgLatency( marker ) * 0.000001;
  uint64_t count = runtimeInfo.getMarkerCount(marker);

  if( count == 0 ){
    return NULL;
  }

  double ctx_exec_time = runtimeInfo.getCPUTime() * 0.000001;
  
  macedbg(1) << "context("<< this->contextName <<") latency=" << latency << ", ctx_exec_time=" << ctx_exec_time << Log::endl;

  mace::map< mace::MaceAddr, double >::const_iterator usage_iter = servers_cpu_usage.find(Util::getMaceAddr());
  mace::map< mace::MaceAddr, double >::const_iterator time_iter = servers_cpu_time.find(Util::getMaceAddr());
  ASSERT( usage_iter != servers_cpu_usage.end() && time_iter != servers_cpu_time.end() );

  double local_cpu_usage = usage_iter->second;
  
  mace::map<mace::string, uint64_t> ctxs_comm_count = runtimeInfo.getContextInteractionCount();

  uint64_t local_msg_count1 = 0;
  uint64_t remote_msg_count1 = 0;

  for( mace::map<mace::string, uint64_t>::iterator iter = ctxs_comm_count.begin(); iter != ctxs_comm_count.end(); iter ++ ){
    bool flag = false;
    for( mace::map< mace::MaceAddr, mace::set<mace::string> >::const_iterator s_iter = migration_ctx_sets.begin(); 
        s_iter != migration_ctx_sets.end(); s_iter++ ) {
      if( (s_iter->second).count(iter->first) > 0 ) {
        remote_msg_count1 += iter->second;
        flag = true;
        break;
      }
    }

    if( !flag && mace::ContextMapping::getNodeByContext( snapshot, iter->first) != Util::getMaceAddr() ) {
      remote_msg_count1 += iter->second;
      continue;
    }

    local_msg_count1 += iter->second;
  }

  double most_benefit = 0.0;
  mace::MaceAddr target_addr;

  for( mace::map< mace::MaceAddr, mace::ServerRuntimeInfo >::const_iterator iter = servers_info.begin(); iter != servers_info.end(); 
      iter ++ ) {
    if( Util::getMaceAddr() == iter->first ){
      continue;
    }

    const ServerRuntimeInfo& server_info = iter->second;

    usage_iter = servers_cpu_usage.find(iter->first);
    time_iter = servers_cpu_time.find(iter->first);
    ASSERT( usage_iter != servers_cpu_usage.end() && time_iter != servers_cpu_time.end() );

    double remote_cpu_usage = 100*( ( (time_iter->second)*(usage_iter->second)*0.01 + ctx_exec_time )/ (time_iter->second) );
        
    uint64_t local_msg_count2 = 0;
    uint64_t remote_msg_count2 = 0;

    mace::map< mace::MaceAddr, mace::set<mace::string> >::const_iterator mc_iter = migration_ctx_sets.find( iter->first );
    for( mace::map<mace::string, uint64_t>::iterator cc_iter = ctxs_comm_count.begin(); cc_iter != ctxs_comm_count.end(); cc_iter ++ ){
      if( ( mc_iter != migration_ctx_sets.end() && (mc_iter->second).count(cc_iter->first) > 0 ) || 
          mace::ContextMapping::getNodeByContext( snapshot, cc_iter->first) == iter->first ) {
        local_msg_count2 += cc_iter->second;
      } else {
        remote_msg_count2 += cc_iter->second;
      }
    }

    double benefit = mace::eMonitor::predictBenefit( ctx_exec_time, local_cpu_usage, local_msg_count1, remote_msg_count1, 
      remote_cpu_usage, local_msg_count2, remote_msg_count2 );
    macedbg(1) << "context("<< this->contextName <<")'s migration benefit on server("<< iter->first <<") will be " << benefit << ", threshold=" << server_info.contextMigrationThreshold << Log::endl;

    if( ( benefit - server_info.contextMigrationThreshold ) > most_benefit ) {
      most_benefit = benefit - server_info.contextMigrationThreshold;
      target_addr = iter->first;
    } 
  }

  if( most_benefit > 0 ) {
    ElasticityBehaviorAction* action = new ElasticityBehaviorAction( mace::ElasticityBehaviorAction::EBACTION_MIGRATION, target_addr, 
      this->contextName, most_benefit);
    macedbg(1) << "To move context("<< this->contextName <<") to server("<< target_addr <<") with benefit("<< most_benefit <<")!" << Log::endl;
    return action;
  } else {
    return NULL;
  }
}

bool mace::ContextBaseClass::isImprovePerformance( const mace::MaceAddr& dest_node, const mace::ContextMapping& snapshot,
    const mace::map< mace::MaceAddr, mace::set<mace::string> >& migration_ctx_sets, 
    const mace::map< mace::MaceAddr, double >& servers_cpu_usage, 
    const mace::map< mace::MaceAddr, double >& servers_cpu_time, const mace::string& marker ) {
  ADD_SELECTORS("ContextBaseClass::isImprovePerformance");
  double latency = runtimeInfo.getMarkerTotalLatency( marker ) * 0.000001;
  double ctx_exec_time = runtimeInfo.getCPUTime() * 0.000001;
    
  mace::map< mace::MaceAddr, double >::const_iterator usage_iter = servers_cpu_usage.find(dest_node);
  mace::map< mace::MaceAddr, double >::const_iterator time_iter = servers_cpu_time.find(dest_node);

  ASSERT( usage_iter != servers_cpu_usage.end() && time_iter != servers_cpu_time.end() );

  double remote_cpu_usage = ( usage_iter->second * time_iter->second * 0.01 + ctx_exec_time ) / ( time_iter->second );
  remote_cpu_usage = remote_cpu_usage * 100;

  mace::map<mace::string, uint64_t> ctxs_comm_count = runtimeInfo.getContextInteractionCount();

  mace::map< mace::MaceAddr, double >::const_iterator local_usage_iter = servers_cpu_usage.find( Util::getMaceAddr() );
  ASSERT( local_usage_iter != servers_cpu_usage.end() );
  double local_cpu_usage = local_usage_iter->second;
  

  uint64_t local_msg_count1 = 0;
  uint64_t remote_msg_count1 = 0;

  for( mace::map<mace::string, uint64_t>::iterator iter = ctxs_comm_count.begin(); iter != ctxs_comm_count.end(); iter ++ ){
    bool flag = false;
    for( mace::map< mace::MaceAddr, mace::set<mace::string> >::const_iterator s_iter = migration_ctx_sets.begin(); 
        s_iter != migration_ctx_sets.end(); s_iter++ ) {
      if( (s_iter->second).count(iter->first) > 0 ) {
        remote_msg_count1 += iter->second;
        flag = true;
        break;
      }
    }

    if( !flag && mace::ContextMapping::getNodeByContext( snapshot, iter->first) != Util::getMaceAddr() ) {
      remote_msg_count1 += iter->second;
      continue;
    }

    local_msg_count1 += iter->second;
  }

  uint64_t local_msg_count2 = 0;
  uint64_t remote_msg_count2 = 0;

  mace::map< mace::MaceAddr, mace::set<mace::string> >::const_iterator mc_iter = migration_ctx_sets.find( dest_node );
  for( mace::map<mace::string, uint64_t>::iterator iter = ctxs_comm_count.begin(); iter != ctxs_comm_count.end(); iter ++ ){
    if( ( mc_iter != migration_ctx_sets.end() && (mc_iter->second).count(iter->first) > 0 ) || 
        mace::ContextMapping::getNodeByContext( snapshot, iter->first) == dest_node ) {
      local_msg_count2 += iter->second;
    } else {
      remote_msg_count2 += iter->second;
    }
  }

  // local_msg_count1 = (uint64_t)( local_msg_count1 / count );
  // remote_msg_count1 = (uint64_t)( remote_msg_count1 / count );
  // local_msg_count2 = (uint64_t)( local_msg_count2 / count );
  // remote_msg_count2 = (uint64_t)( remote_msg_count2 / count );

  double benefit = eMonitor::predictBenefit( ctx_exec_time, local_cpu_usage, local_msg_count1, remote_msg_count1, 
      remote_cpu_usage, local_msg_count2, remote_msg_count2 ); 
  macedbg(1) << "context(" << this->contextName << ") latency=" << latency << ", benefit=" << benefit << Log::endl;
  if( benefit > 0.0 ){
    return true;
  } else {
    return false;
  }
}

/************************************* class ContextEventOrder *************************************************************/
void mace::ContextEventOrder::clear() {
  executeTicketEventMap.clear();
  executeEventTicketMap.clear();
  executeTicketNumber = 1;
}


uint64_t mace::ContextEventOrder::addExecuteEvent(OrderID const& eventId) {
  ADD_SELECTORS("ContextEventOrder::addExecuteEvent");
  ScopedLock sl(eventOrderMutex);
  uint64_t executeTicket = executeTicketNumber ++;
  executeTicketEventMap[executeTicket] = eventId;
  executeEventTicketMap[eventId] = executeTicket;

  macedbg(1) << "Event("<< eventId <<")'s executeTicket=" << executeTicket << " in " << contextName <<Log::endl; 
  return executeTicket;
}

void mace::ContextEventOrder::removeEventExecuteTicket(mace::OrderID const& eventId ) {
  ADD_SELECTORS("ContextEventOrder::removeEventExecuteTicket");
  ScopedLock sl(eventOrderMutex);
  mace::map<OrderID, uint64_t>::iterator o_iter = executeEventTicketMap.find(eventId);
  if( o_iter != executeEventTicketMap.end() ) {
    const uint64_t ticket = o_iter->second;
    executeEventTicketMap.erase(eventId);
    executeTicketEventMap.erase(ticket);
    macedbg(1) << "Delete event("<< eventId <<")'s execute ticket in "<< this->contextName << Log::endl;
  }
}

OrderID mace::ContextEventOrder::getExecuteEventOrderID( const uint64_t ticket ) {
  ScopedLock sl(eventOrderMutex);
  mace::map<uint64_t, OrderID>::iterator iter = executeTicketEventMap.find(ticket);
  if( iter == executeTicketEventMap.end() ) {
    OrderID id;
    return id;
  } else {
    return iter->second;
  }
}

uint64_t mace::ContextEventOrder::getExecuteEventTicket( OrderID const& eventId ) {
  ADD_SELECTORS("ContextEventOrder::getExecuteEventTicket");
  //macedbg(1) << "ContextEventOrder("<< contextName <<") contextEventOrder="<< this <<" eventOrderMutex = " << &eventOrderMutex << Log::endl;
  ScopedLock sl(eventOrderMutex);
  mace::map<OrderID, uint64_t>::const_iterator iter = executeEventTicketMap.find(eventId);
  if( iter == executeEventTicketMap.end() ) {
    return 0;
  } else {
    if( iter->second >= executeTicketNumber ){
      maceerr << "In context("<< this->contextName <<") event("<< eventId <<") executeTicket=" << iter->second << " executeTicketNumber=" << executeTicketNumber << Log::endl;
      ASSERT(false);
    }
    return iter->second;
  }
}

void mace::ContextEventOrder::serialize(std::string& str) const{
  mace::serialize( str, &contextName );
  mace::serialize( str, &executeTicketEventMap );
  mace::serialize( str, &executeEventTicketMap );
  mace::serialize( str, &executeTicketNumber );
}

int mace::ContextEventOrder::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &contextName );
  serializedByteSize += mace::deserialize( is, &executeTicketEventMap   );
  serializedByteSize += mace::deserialize( is, &executeEventTicketMap   );
  serializedByteSize += mace::deserialize( is, &executeTicketNumber   );
  return serializedByteSize;
}

/*****************************class __EventStorageStruct__**********************************************/
void mace::__EventStorageStruct__::serialize(std::string& str) const{
  mace::serialize( str, &messageType );
  mace::serialize( str, msg );
  mace::serialize( str, &contextEventType );
  mace::serialize( str, &eventOpInfo );
}

int mace::__EventStorageStruct__::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &messageType);
  switch( messageType ) {
    case mace::InternalMessage::ASYNC_EVENT: {
      serializedByteSize += deserializeEvent(is);
      break;
    }
    case mace::InternalMessage::ROUTINE: {
      serializedByteSize += deserializeRoutine(is);
      break;
    }
    case mace::InternalMessage::COMMIT_SINGLE_CONTEXT: {
      msg = InternalMessageHelperPtr( new commit_single_context_Message() );
      serializedByteSize += msg->deserialize(is);
      break;
    }

    default:
      ASSERTMSG(false, "Unkown message type!");
  }
  serializedByteSize += mace::deserialize( is, &contextEventType );
  serializedByteSize += mace::deserialize( is, &eventOpInfo );
  return serializedByteSize;
}

void mace::ContextBaseClassParams::initialize( ContextBaseClass* ctxObj) {
    ADD_SELECTORS("ContextBaseClassParams::initialize");
    macedbg(1) << "context=" << ctxObj->contextName << Log::endl;
    this->contextName = ctxObj->contextName; ///< The canonical name of the context
    this->contextTypeName = ctxObj->contextTypeName;
    this->serviceId = ctxObj->serviceId; ///< The service in which the context belongs to
    this->contextId = ctxObj->contextId;
    
    macedbg(1) << "createQueue=" << ctxObj->createEventQueue.size() << Log::endl;
    while( ctxObj->createEventQueue.size() > 0 ) {
      ContextBaseClass::RQType& createEvent = ctxObj->createEventQueue.front();
      mace::__CreateEventStorage__ mCreateEvent( static_cast<mace::InternalMessageHelperPtr>(createEvent.param), createEvent.eventId, serviceId );
      this->createEventQueue.push_back(mCreateEvent);
      createEvent.param = NULL;
      ctxObj->createEventQueue.pop();
      mCreateEvent.msg = NULL;
    }

    macedbg(1) << "waitingEvents=" << ctxObj->waitingEvents.size() << Log::endl;
    mace::ContextBaseClass::EventStorageType::iterator wIter = ctxObj->waitingEvents.begin();
    for(; wIter != ctxObj->waitingEvents.end(); wIter++ ) {
      mace::vector<mace::__EventStorageStruct__>& wevents = wIter->second;
      for( uint32_t i=0; i<wevents.size(); i++ ){
        mace::__EventStorageStruct__& we = wevents[i];
        this->waitingEvents[ wIter->first ].push_back(we);
        we.msg = NULL;
      }
    }
    
    ASSERT( executeCommitEventQueue.size() == 0 );

    macedbg(1) << "executeQueue=" << ctxObj->executeEventQueue.size() << Log::endl;
    while( ctxObj->executeEventQueue.size() > 0 ){
      mace::ContextEvent& ce = ctxObj->executeEventQueue.front();
      this->executeEventQueue.push_back(ce);
      ce.param = NULL;
      ctxObj->executeEventQueue.pop_front();
    }
    
    this->create_now_committing_ticket = ctxObj->create_now_committing_ticket;
    this->execute_now_committing_eventId = ctxObj->execute_now_committing_eventId;
    this->execute_now_committing_ticket = ctxObj->execute_now_committing_ticket;

    this->now_serving_eventId = ctxObj->now_serving_eventId;
    this->now_serving_execute_ticket = ctxObj->now_serving_execute_ticket;
    
    this->lastWrite = ctxObj->lastWrite;

    this->contextEventOrder = ctxObj->contextEventOrder;
    this->dominator = ctxObj->dominator;

    this->runtimeInfo = ctxObj->runtimeInfo;

    this->createTicketNumber = ctxObj->createTicketNumber;
    this->executeTicketNumber = ctxObj->executeTicketNumber;

    ASSERT( ctxObj->executedEventsCommitFlag.size() == 0 );
    this->executedEventsCommitFlag = ctxObj->executedEventsCommitFlag;

    this->now_serving_create_ticket = ctxObj->now_serving_create_ticket;
    this->now_max_execute_ticket = ctxObj->now_max_execute_ticket;
        
    this->eventExecutionInfos = ctxObj->eventExecutionInfos;
    this->readerEvents = ctxObj->readerEvents;
    this->writerEvents = ctxObj->writerEvents;
    this->migrationEventWaiting = ctxObj->migrationEventWaiting;
    this->skipCreateTickets = ctxObj->skipCreateTickets;

    macedbg(1) << "numReaders=" << this->readerEvents.size() << " numWriters=" << this->writerEvents.size() << Log::endl; 
}

void mace::ContextBaseClassParams::serialize(std::string& str) const{
  mace::serialize(str, &contextName); ///< The canonical name of the context
  mace::serialize(str, &contextTypeName);
  
  mace::serialize(str, &serviceId); ///< The service in which the context belongs to
  mace::serialize(str, &contextId);
  
  mace::serialize(str, &createEventQueue);
  mace::serialize(str, &waitingEvents);

  mace::serialize(str, &executeCommitEventQueue);
  
  mace::serialize(str, &executeEventQueue);

  mace::serialize(str, &create_now_committing_ticket);
  mace::serialize(str, &execute_now_committing_eventId);
  mace::serialize(str, &execute_now_committing_ticket);

  mace::serialize(str, &now_serving_eventId);
  mace::serialize(str, &now_serving_execute_ticket);
  
  mace::serialize(str, &lastWrite);

  mace::serialize(str, &contextEventOrder);
  mace::serialize(str, &dominator);

  mace::serialize(str, &runtimeInfo);

  mace::serialize(str, &createTicketNumber);
  mace::serialize(str, &executeTicketNumber);

  mace::serialize(str, &executedEventsCommitFlag);
  mace::serialize(str, &now_serving_create_ticket);
  mace::serialize(str, &now_max_execute_ticket);
  
  mace::serialize(str, &eventExecutionInfos);

  mace::serialize(str, &readerEvents);
  mace::serialize(str, &writerEvents);
  mace::serialize(str, &migrationEventWaiting);
  mace::serialize(str, &skipCreateTickets );
}

int mace::ContextBaseClassParams::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;

  serializedByteSize += mace::deserialize(is, &contextName); ///< The canonical name of the context
  serializedByteSize += mace::deserialize(is, &contextTypeName);
  
  serializedByteSize += mace::deserialize(is, &serviceId); ///< The service in which the context belongs to
  serializedByteSize += mace::deserialize(is, &contextId);
  
  serializedByteSize += mace::deserialize(is, &createEventQueue);
  serializedByteSize += mace::deserialize(is, &waitingEvents);

  serializedByteSize += mace::deserialize(is, &executeCommitEventQueue);
  
  serializedByteSize += mace::deserialize(is, &executeEventQueue);
  serializedByteSize += mace::deserialize(is, &create_now_committing_ticket);
  serializedByteSize += mace::deserialize(is, &execute_now_committing_eventId);
  serializedByteSize += mace::deserialize(is, &execute_now_committing_ticket);

  serializedByteSize += mace::deserialize(is, &now_serving_eventId);
  serializedByteSize += mace::deserialize(is, &now_serving_execute_ticket);
  
  serializedByteSize += mace::deserialize(is, &lastWrite);

  serializedByteSize += mace::deserialize(is, &contextEventOrder);
  serializedByteSize += mace::deserialize(is, &dominator);

  serializedByteSize += mace::deserialize(is, &runtimeInfo);

  serializedByteSize += mace::deserialize(is, &createTicketNumber);
  serializedByteSize += mace::deserialize(is, &executeTicketNumber);

  serializedByteSize += mace::deserialize(is, &executedEventsCommitFlag);
  serializedByteSize += mace::deserialize(is, &now_serving_create_ticket);
  serializedByteSize += mace::deserialize(is, &now_max_execute_ticket);
  
  serializedByteSize += mace::deserialize(is, &eventExecutionInfos);

  serializedByteSize += mace::deserialize(is, &readerEvents);
  serializedByteSize += mace::deserialize(is, &writerEvents);
  serializedByteSize += mace::deserialize(is, &migrationEventWaiting);
  serializedByteSize += mace::deserialize(is, &skipCreateTickets );
  return serializedByteSize;
}

/****************************************** class LockRequest *******************************************************************/
bool LockRequest::enqueueEventOperation( mace::EventOperationInfo const& eventOpInfo ) {
  ADD_SELECTORS("LockRequest::enqueueEventOperation");
  bool exist = false;
  for( uint32_t i=0; i<eventOpInfos.size(); i++ ) {
    if( eventOpInfos[i] == eventOpInfo ){
      exist = true;
    }
  }

  if( !exist ){
    eventOpInfos.push_back(eventOpInfo);
  }

  return true;
}

bool LockRequest::unlock( mace::EventOperationInfo const& op ) {
  ADD_SELECTORS("LockRequest::unlock");

  bool unlock_flag = false;
  for( mace::vector<mace::EventOperationInfo>::iterator iter = eventOpInfos.begin(); iter != eventOpInfos.end(); iter ++ ){
    if( *iter == op ){
      unlock_flag = true;
      eventOpInfos.erase(iter);
      break;
    }
  }
  return unlock_flag;
}

void mace::LockRequest::serialize(std::string& str) const{
  mace::serialize( str, &lockType );
  mace::serialize( str, &eventId );
  mace::serialize( str, &lockedContextName );
  mace::serialize( str, &eventOpInfos );
  mace::serialize( str, &hasNotified );
}

int mace::LockRequest::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &lockType );
  serializedByteSize += mace::deserialize( is, &eventId   );
  serializedByteSize += mace::deserialize( is, &lockedContextName   );
  serializedByteSize += mace::deserialize( is, &eventOpInfos   );
  serializedByteSize += mace::deserialize( is, &hasNotified   );
  return serializedByteSize;
}

/****************************************** class DomLockRequest ***********************************************************/
void mace::DomLockRequest::serialize(std::string& str) const{
  mace::serialize( str, &lockType );
  mace::serialize( str, &eventId );
  mace::serialize( str, &lockOps );
  mace::serialize( str, &lockedContexts );
  mace::serialize( str, &hasNotified );
}

int mace::DomLockRequest::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &lockType );
  serializedByteSize += mace::deserialize( is, &eventId   );
  serializedByteSize += mace::deserialize( is, &lockOps );
  serializedByteSize += mace::deserialize( is, &lockedContexts   );
  serializedByteSize += mace::deserialize( is, &hasNotified   );
  return serializedByteSize;
}

void mace::DomLockRequest::addEventOp( const mace::EventOperationInfo& op ) {
  ADD_SELECTORS("DomLockRequest::addEventOp");
  bool exist = false;
  for( uint32_t i=0; i<lockOps.size(); i++ ) {
    if( lockOps[i] == op ) {
      maceerr << "EventOp("<< op <<") already exists!" << Log::endl;
      exist = true;
      break;
    }
  }
  if( !exist ){
    lockOps.push_back(op);
    if( lockedContexts.count(op.toContextName) == 0 ) {
      lockedContexts.insert(op.toContextName);
    }
  }
}

void mace::DomLockRequest::addLockedContexts( const mace::vector<mace::string>& locked_contexts ) {
  ADD_SELECTORS("DomLockRequest::addLockedContexts");
  for( uint32_t i=0; i<locked_contexts.size(); i++ ) {
    if( lockedContexts.count(locked_contexts[i]) == 0 ) {
      lockedContexts.insert(locked_contexts[i]);
      macedbg(1) << "Event("<< eventId <<") adds locked context: " << locked_contexts[i] << ", " << lockedContexts << Log::endl;
    }
  }
}

void mace::DomLockRequest::addLockedContext( const mace::string& locked_context ) {
  ADD_SELECTORS("DomLockRequest::addLockedContext");
  if( lockedContexts.count(locked_context) == 0 ) {
    lockedContexts.insert(locked_context);
    macedbg(1) << "Event("<< eventId <<") adds locked context: " << locked_context << ", " << lockedContexts << Log::endl;
  }
}

bool mace::DomLockRequest::toRemove() const {
  if( lockOps.size() == 0 ) {
    return true;
  } else {
    return false;
  }
}

mace::vector<mace::string> mace::DomLockRequest::releaseContext( const mace::string& dominator, const mace::string& fromContext,
    const mace::string& locked_context, const ContextStructure& contextStructure ) {
  ADD_SELECTORS("DomLockRequest::releaseContext");
  bool to_continue = true;
  bool exist = false;
  while(to_continue) {
    to_continue = false;
    for( mace::vector<mace::EventOperationInfo>::iterator iter = lockOps.begin(); iter != lockOps.end(); iter ++ ){
      if( (*iter).toContextName == locked_context 
          && contextStructure.getUpperBoundContextName( (*iter).fromContextName ) == fromContext ) {
        macedbg(1) << "To remove "<< *iter << Log::endl;
        lockOps.erase(iter);
        exist = true;
        to_continue = true;
        break;
      }
    }
  }
  ASSERT(exist);

  mace::vector<mace::string> releaseContexts;
  if( this->lockType != LockRequest::DLOCK ) {
    return releaseContexts;
  }

  macedbg(1) << "Event("<< eventId <<") lockOps: " << lockOps << Log::endl;
  if( lockOps.size() == 0 ){
    for( mace::set<mace::string>::iterator iter = lockedContexts.begin(); iter != lockedContexts.end(); iter++ ){
      releaseContexts.push_back(*iter);
    }
    lockedContexts.clear();
  }
  return releaseContexts;
}

bool mace::DomLockRequest::lockingContext( const mace::string& ctx_name ) const {
  for( mace::vector<mace::EventOperationInfo>::const_iterator iter = lockOps.begin(); iter != lockOps.end(); iter ++ ){
    if( (*iter).toContextName == ctx_name ) {
      return true;
    }
  }
  return false;
}

bool mace::DomLockRequest::unlock( const mace::string& dominator, const mace::EventOperationInfo& eop, 
    const ContextStructure& contextStructure, mace::vector<mace::string>& releaseContexts ) {
  bool exist = false;
  
  for( mace::vector<mace::EventOperationInfo>::iterator iter = lockOps.begin(); iter != lockOps.end(); iter ++ ){
    if( (*iter) == eop ) {
      lockOps.erase(iter);
      exist = true;
      break;
    }
  }
  if( !exist ){
    return false;
  }

  // The event only releases the dominator when it does not lock any non-dominator child context (exclude dominator itself)
  if( this->lockType != LockRequest::DLOCK ) {
    return true;
  }

  if( lockOps.size() == 0 ){
    for( mace::set<mace::string>::iterator iter = lockedContexts.begin(); iter != lockedContexts.end(); iter++ ){
      releaseContexts.push_back(*iter);
    }
    lockedContexts.clear();
  }
  return true;
}

void DomLockRequest::removeNotLockedContext( const mace::set<mace::string>& dominatedContexts ) {
  bool to_continue = true;
  while( to_continue ) {
    to_continue = false;
    for(mace::set<mace::string>::iterator iter = lockedContexts.begin(); iter != lockedContexts.end(); iter ++ ) {
      if( dominatedContexts.count(*iter) == 0 ) {
        lockedContexts.erase(iter);
        to_continue = true;
        break;
      }
    }
  }
}

/****************************************** class DominatorRequest ***********************************************************/
void mace::DominatorRequest::serialize(std::string& str) const{
  mace::serialize( str, &lockType );
  mace::serialize( str, &eventId );
  mace::serialize( str, &ctxName );
  mace::serialize( str, &eventOpInfo );
}

int mace::DominatorRequest::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &lockType );
  serializedByteSize += mace::deserialize( is, &eventId   );
  serializedByteSize += mace::deserialize( is, &ctxName   );
  serializedByteSize += mace::deserialize( is, &eventOpInfo   );
  return serializedByteSize;
}

/****************************************** class ContextEventDominator *******************************************************************/
void ContextEventDominator::initialize( const mace::string& contextName, const mace::string& domCtx, const uint64_t& ver,
    const mace::vector<mace::string>& dominateCtxs) {
  ADD_SELECTORS("ContextEventDominator::initialize");
  macedbg(1) << "To initialize Dominator("<< contextName <<") with dominateCtxs=" << dominateCtxs << Log::endl;
  ScopedLock sl(dominatorMutex);

  this->contextName = contextName;
  this->dominateContexts = dominateCtxs;
  this->version = ver;
  this->preDominator = "";
  this->curDominator = domCtx;

  this->eventOrderQueue.clear();
  for( uint32_t i=0; i<dominateContexts.size(); i++ ){
    mace::vector<LockRequest> queue;
    eventOrderQueue[ dominateContexts[i] ] = queue;
  }
}

mace::set<mace::string> ContextEventDominator::checkEventExecutePermission(BaseMaceService* sv, 
    mace::EventOperationInfo const& eventOpInfo ) {
  ADD_SELECTORS("ContextEventDominator::checkEventExecutePermission");
    
  ContextService* _service = static_cast<ContextService*>(sv);
  ContextStructure& contextStructure = _service->contextStructure;
  
  ScopedLock sl(dominatorMutex);
  macedbg(1) << "Check "<< eventOpInfo <<" in context "<< this->contextName << Log::endl;

  EventLockQueue::iterator iter = eventOrderQueue.find( eventOpInfo.toContextName );
  if( iter == eventOrderQueue.end() ){
    maceerr << "Fail to find context("<< eventOpInfo.toContextName <<") in " << this->contextName << " dominatedContexts=" << dominateContexts << Log::endl;
    ASSERT(false);
  }

  // Check if the event is already in the domLockRequestQueue
  bool exist_in_dom_queue = false;
  for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ){
    DomLockRequest& request = domLockRequestQueue[i];
    if( request.eventId == eventOpInfo.eventId ) {
      request.addEventOp(eventOpInfo);
      macedbg(1) << "LockRequest("<< eventOpInfo.eventId <<") already exists in domLockRequestQueue in dominator("<< this->contextName <<")!" << Log::endl;
      exist_in_dom_queue = true;
      break;
    }
  }

  // Adding new dominator lock request
  if( !exist_in_dom_queue ) {
    uint16_t lock_type = 0;
    if( eventOpInfo.eventOpType == mace::Event::EVENT_OP_READ ) {
      lock_type = LockRequest::RLOCK;
    } else if( eventOpInfo.eventOpType == mace::Event::EVENT_OP_WRITE ) {
      lock_type = LockRequest::WLOCK;
    } else if( eventOpInfo.eventOpType == mace::Event::EVENT_OP_OWNERSHIP ) {
      lock_type = LockRequest::DLOCK;
    } else {
      ASSERT(false);
    }

    DomLockRequest request(lock_type, eventOpInfo.eventId);
    request.addEventOp(eventOpInfo);
    domLockRequestQueue.push_back(request);
    macedbg(1) << "LockRequest("<< eventOpInfo.eventId <<") is enqueued into domLockRequestQueue in dominator("<< this->contextName <<")!" << Log::endl;
  }

  this->enqueueEventOrderQueue( contextStructure, eventOpInfo);
    
  return this->getEventPermitContexts( eventOpInfo.eventId );
}

void ContextEventDominator::enqueueEventOrderQueue( ContextStructure& ctxStructure, const mace::EventOperationInfo& eventOpInfo ){
  ADD_SELECTORS("ContextEventDominator::enqueueEventOrderQueue");
  EventLockQueue::iterator iter = eventOrderQueue.find( eventOpInfo.toContextName );
  ASSERT( iter != eventOrderQueue.end() );

  // Ownership events will not enqueue
  if( eventOpInfo.eventOpType == mace::Event::EVENT_OP_OWNERSHIP ) {
    macedbg(1) << "Event("<< eventOpInfo.eventId <<") will modify the DAG in dominator("<< this->contextName <<")!" << Log::endl;
    return;
  }

  bool to_lock_dominator = false;
  for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ){
    DomLockRequest& request = domLockRequestQueue[i];
    if( request.lockType == LockRequest::DLOCK ) {
      break;
    }

    if( request.eventId == eventOpInfo.eventId ) {
      macedbg(1) << "Event("<< eventOpInfo.eventId <<") could lock dominator("<< this->contextName <<")!" << Log::endl;
      to_lock_dominator = true;
      break;
    }
  }

  if( !to_lock_dominator ) {
    return;
  }

  mace::vector<LockRequest>& queue = iter->second;

  uint16_t lockType = LockRequest::INVALID;
  uint16_t vlockType = LockRequest::INVALID;
  if( eventOpInfo.eventOpType == mace::Event::EVENT_OP_READ ) {
    lockType = LockRequest::RLOCK;
    vlockType = LockRequest::VRLOCK;
  } else if( eventOpInfo.eventOpType == mace::Event::EVENT_OP_WRITE ){
    lockType = LockRequest::WLOCK;
    vlockType = LockRequest::VWLOCK;
  } else {
    ASSERT(false);
  }

  bool exit_flag = false;
  for( uint32_t i=0; i<queue.size(); i++ ){
    if( queue[i].eventId == eventOpInfo.eventId ) {
      queue[i].lockType = lockType;
      queue[i].enqueueEventOperation( eventOpInfo );
      exit_flag = true;
      macedbg(1) << "In dominator("<< this->contextName <<"), enqueue lock "<< queue[i] <<" into existing "<< eventOpInfo.toContextName << Log::endl;
    }
  }

  if( !exit_flag ){
    LockRequest lockRequest( lockType, eventOpInfo.toContextName, eventOpInfo.eventId );
    lockRequest.enqueueEventOperation( eventOpInfo );
    queue.push_back(lockRequest);
    macedbg(1) << "In dominator("<< this->contextName <<"), enqueue lock "<< lockRequest <<" into "<< eventOpInfo.toContextName << Log::endl;
  }

  //enqueue vlock into children contexts' queues
  if( !exit_flag ){
    bool exist_flag2 = false;
    for( uint32_t i=0; i<dominateContexts.size(); i++ ){
      if( dominateContexts[i] == eventOpInfo.toContextName ){
        continue;
      }

      if( ctxStructure.isElderContext(dominateContexts[i], eventOpInfo.toContextName) ){
        iter = eventOrderQueue.find( dominateContexts[i] );
        ASSERT( iter!=eventOrderQueue.end() );
        mace::vector<LockRequest>& queue = iter->second;

        exist_flag2 = false;
        for( uint32_t j=0; j<queue.size(); j++ ){
          if( queue[j].eventId == eventOpInfo.eventId ) {
            exist_flag2 = true;
            break;
          }
        }

        if( !exist_flag2 ){
          LockRequest lockRequest( vlockType, dominateContexts[i], eventOpInfo.eventId );
          macedbg(1) << "In dominator("<< this->contextName <<"), enqueue vlock "<< lockRequest <<" into "<< dominateContexts[i] << Log::endl;
          queue.push_back(lockRequest);
        }

      }
    }
  }
}

mace::set<mace::string> ContextEventDominator::getEventPermitContexts( const mace::OrderID& eventId ) {
  ADD_SELECTORS("ContextEventDominator::getEventPermitContexts");

  mace::set<mace::string> permitContexts;

  bool exist = false;
  uint16_t lock_type;
  for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ) {
    DomLockRequest& request = domLockRequestQueue[i];
    if( request.eventId == eventId ){
      lock_type = request.lockType;
      exist = true;
      break;
    } 
  }
  ASSERT( exist );

  mace::set<mace::OrderID> can_lock_eventIds = this->getCanLockEventIDs();
  if( can_lock_eventIds.count(eventId) > 0 ) {
    this->labelRequestNotified( eventId );
    if( lock_type == LockRequest::DLOCK ){
      for( uint32_t i=0; i<dominateContexts.size(); i++ ) {
        permitContexts.insert( dominateContexts[i] );
      }
    }
  }
  
  if( lock_type == LockRequest::DLOCK ) {
    return permitContexts;
  }

  // check contexts the WR event could access directly
  for( EventLockQueue::iterator iter=eventOrderQueue.begin(); iter != eventOrderQueue.end(); iter++ ) {
    mace::vector<LockRequest>& lockQueue = iter->second;
    for( uint32_t i=0; i<lockQueue.size(); i++ ){
      if( lockQueue[i].eventId == eventId ){
        lockQueue[i].hasNotified = true;
        if( (lock_type == LockRequest::WLOCK && i == 0) || lock_type == LockRequest::RLOCK ) {
          permitContexts.insert(iter->first);
        }
        break;
      } 

      if( lock_type == LockRequest::WLOCK || lockQueue[i].lockType == LockRequest::WLOCK 
          || lockQueue[i].lockType == LockRequest::VWLOCK || lockQueue[i].lockType == LockRequest::UNLOCK ){
        break;
      }
    }
  } 
  return permitContexts;
}

void ContextEventDominator::printWaitingEventID( const mace::EventOperationInfo& eop ) const {
  ADD_SELECTORS("ContextEventDominator::printWaitingEventID");
  if( eop.eventOpType == mace::Event::EVENT_OP_OWNERSHIP ){
    for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ) {
      const DomLockRequest& request = domLockRequestQueue[i];
      if( request.eventId == eop.eventId ) {
        ASSERT( i > 0 );
        const DomLockRequest& pre_request = domLockRequestQueue[i-1];
        macedbg(1) << "Event("<< eop.eventId <<") is waiting for event("<< pre_request.eventId <<") in dominator("<< this->contextName <<")!" << Log::endl;
        break;
      }
    }
  } else {
    EventLockQueue::const_iterator iter = eventOrderQueue.find( eop.toContextName );
    ASSERT( iter != eventOrderQueue.end() );
    const mace::vector<LockRequest>& queue = iter->second;

    bool locked_dom = false;
    for( uint32_t i=0; i<queue.size(); i++ ) {
      const LockRequest& request = queue[i];
      if( request.eventId == eop.eventId ) {
        ASSERT( i > 0 );
        const LockRequest& pre_request = queue[i-1];
        macedbg(1) << "Event("<< eop.eventId <<") is waiting for event("<< pre_request.eventId <<") on context("<< eop.toContextName <<") in dominator("<< this->contextName <<")!" << Log::endl;
        locked_dom = true;
        break;
      }
    }
    
    if( !locked_dom ) {
      for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ) {
        const DomLockRequest& request = domLockRequestQueue[i];
        if( request.eventId == eop.eventId ) {
          ASSERT( i > 0 );
          const DomLockRequest& pre_request = domLockRequestQueue[i-1];
          macedbg(1) << "Event("<< eop.eventId <<") is waiting for event("<< pre_request.eventId <<") in dominator("<< this->contextName <<")!" << Log::endl;
          break;
        }
      }
    }
  }
}

mace::set<mace::OrderID> ContextEventDominator::getCanLockEventIDs() const {
  mace::set<mace::OrderID> can_lock_eventIds;
  for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ) {
    const DomLockRequest& request = domLockRequestQueue[i];
    if( request.lockType == LockRequest::DLOCK ){
      if( i == 0 ){
        can_lock_eventIds.insert( request.eventId );
      }
      break;
    } else {
      can_lock_eventIds.insert( request.eventId );
    }
  }
  return can_lock_eventIds;
}

bool ContextEventDominator::checkContextInclusion( mace::string const& ctxName ) {
  ADD_SELECTORS("ContextEventDominator::checkContextInclusion");
  ScopedLock sl(dominatorMutex);
  if( eventOrderQueue.find(ctxName) != eventOrderQueue.end() ) {
    return true;
  } else {
    return false;
  }
}

bool ContextEventDominator::checkContextInclusionNoLock( mace::string const& ctxName ) const {
  ADD_SELECTORS("ContextEventDominator::checkContextInclusionNoLock");
  
  if( eventOrderQueue.find(ctxName) != eventOrderQueue.end() ) {
    return true;
  } else {
    return false;
  }
}

void ContextEventDominator::labelRequestNotified( const mace::OrderID& eventId ) {
  for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ) {
    DomLockRequest& request = domLockRequestQueue[i];
    if( request.eventId == eventId ){
      request.hasNotified = true;
      break;
    }
  }
}

bool ContextEventDominator::unlockContext( ContextStructure& contextStructure, 
    mace::EventOperationInfo const& eventOpInfo, mace::map<mace::string, mace::vector<mace::EventOperationInfo> >& permittedEventOps, 
    mace::map< mace::OrderID, mace::vector<mace::string> >& permittedContextNames, mace::vector<mace::string>& releaseContexts ) {
  
  ADD_SELECTORS("ContextEventDominator::unlockContext");
  if( contextStructure.getUpperBoundContextName( eventOpInfo.fromContextName) != this->contextName ) {
    macedbg(1) << "fromContextName="<< eventOpInfo.fromContextName <<"; dominator=" << this->contextName << Log::endl;
    ASSERT(false)
  }
  ASSERT( checkContextInclusion(eventOpInfo.toContextName) );  
  
  ScopedLock sl(dominatorMutex);
  mace::set<mace::string> locked_contexts;
  macedbg(1) << eventOpInfo <<" tries to unlock context("<< eventOpInfo.toContextName <<") in dominator("<< this->contextName <<")!" << Log::endl;
  for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ) {
    DomLockRequest& request = domLockRequestQueue[i];
    if( request.eventId == eventOpInfo.eventId ) {
      if( !request.unlock(this->contextName, eventOpInfo, contextStructure, releaseContexts ) ){
        return false;
      }
      locked_contexts = request.lockedContexts;
      break;
    }
  }

  uint16_t lockType = LockRequest::INVALID;

  if( eventOpInfo.eventOpType == mace::Event::EVENT_OP_READ ) {
    lockType = LockRequest::RLOCK;
  } else if( eventOpInfo.eventOpType == mace::Event::EVENT_OP_WRITE ) {
    lockType = LockRequest::WLOCK;
  } else if( eventOpInfo.eventOpType == mace::Event::EVENT_OP_OWNERSHIP ) {
    lockType = LockRequest::DLOCK;
  } else {
    ASSERT(false);
  }

  if( lockType == LockRequest::RLOCK || lockType == LockRequest::WLOCK ) {
    mace::vector<LockRequest>& queue = eventOrderQueue[eventOpInfo.toContextName];
    bool unlock_flag = false;
    for( uint32_t i=0; i<queue.size(); i++ ){
      LockRequest& lockrequest = queue[i];
      if( lockrequest.eventId == eventOpInfo.eventId && lockrequest.lockType == lockType && lockrequest.unlock(eventOpInfo) ){
        if( lockrequest.getOpNumber() == 0 ) {
          macedbg(1) << "Change lockrequest("<< lockrequest <<") to UNLOCK in " << this->contextName <<Log::endl;
          lockrequest.lockType = LockRequest::UNLOCK;
        }
        unlock_flag = true;
        break;
      }
    }

    if( !unlock_flag ){
      maceerr << "Event("<< eventOpInfo.eventId <<") doesn't lock context("<< eventOpInfo.toContextName <<") in dominator("<< this->contextName <<")!" << Log::endl;
      ASSERT(false);
    }
  }
  
  this->checkEventOrderQueue( contextStructure, eventOpInfo.eventId, lockType, locked_contexts, permittedEventOps, 
    permittedContextNames, releaseContexts );
  
  return true;
}

bool ContextEventDominator::unlockContextWithoutLock( ContextStructure& contextStructure, 
    mace::EventOperationInfo const& eventOpInfo, mace::map<mace::string, mace::vector<mace::EventOperationInfo> >& permittedEventOps, 
    mace::map< mace::OrderID, mace::vector<mace::string> >& permittedContextNames, mace::vector<mace::string>& releaseContexts ) {
  
  ADD_SELECTORS("ContextEventDominator::unlockContextWithoutLock");
  if( contextStructure.getUpperBoundContextName( eventOpInfo.fromContextName) != this->contextName ) {
    macedbg(1) << "fromContextName="<< eventOpInfo.fromContextName <<"; dominator=" << this->contextName << Log::endl;
    ASSERT(false)
  }
  
  mace::set<mace::string> locked_contexts;
  macedbg(1) << eventOpInfo <<" tries to unlock context("<< eventOpInfo.toContextName <<") in dominator("<< this->contextName <<")!" << Log::endl;
  for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ) {
    DomLockRequest& request = domLockRequestQueue[i];
    if( request.eventId == eventOpInfo.eventId ) {
      if( !request.unlock(this->contextName, eventOpInfo, contextStructure, releaseContexts ) ){
        return false;
      }
      locked_contexts = request.lockedContexts;
      break;
    }
  }

  uint16_t lockType = LockRequest::INVALID;

  if( eventOpInfo.eventOpType == mace::Event::EVENT_OP_READ ) {
    lockType = LockRequest::RLOCK;
  } else if( eventOpInfo.eventOpType == mace::Event::EVENT_OP_WRITE ) {
    lockType = LockRequest::WLOCK;
  } else if( eventOpInfo.eventOpType == mace::Event::EVENT_OP_OWNERSHIP ) {
    lockType = LockRequest::DLOCK;
  } else {
    ASSERT(false);
  }

  if( lockType == LockRequest::RLOCK || lockType == LockRequest::WLOCK ) {
    mace::vector<LockRequest>& queue = eventOrderQueue[eventOpInfo.toContextName];
    bool unlock_flag = false;
    for( uint32_t i=0; i<queue.size(); i++ ){
      LockRequest& lockrequest = queue[i];
      if( lockrequest.eventId == eventOpInfo.eventId && lockrequest.lockType == lockType && lockrequest.unlock(eventOpInfo) ){
        if( lockrequest.getOpNumber() == 0 ) {
          macedbg(1) << "Change lockrequest("<< lockrequest <<") to UNLOCK in " << this->contextName <<Log::endl;
          lockrequest.lockType = LockRequest::UNLOCK;
        }
        unlock_flag = true;
        break;
      }
    }

    if( !unlock_flag ){
      maceerr << "Event("<< eventOpInfo.eventId <<") doesn't lock context("<< eventOpInfo.toContextName <<") in dominator("<< this->contextName <<")!" << Log::endl;
      ASSERT(false);
    }
  }
  
  this->checkEventOrderQueue( contextStructure, eventOpInfo.eventId, lockType, locked_contexts, permittedEventOps, 
    permittedContextNames, releaseContexts );
  
  return true;
}

void ContextEventDominator::unlockContextByWaitingRequests( ContextStructure& contextStructure, 
    const mace::OrderID& eventId, mace::map<mace::string, mace::vector<mace::EventOperationInfo> >& permittedEventOps, 
    mace::map< mace::OrderID, mace::vector<mace::string> >& permittedContextNames, mace::vector<mace::string>& releaseContexts ) {
  
  ADD_SELECTORS("ContextEventDominator::unlockContextByWaitingRequests");
  ScopedLock sl(dominatorMutex);
  mace::vector< mace::EventOperationInfo > waiting_unlock_requests;
  for( uint32_t i=0; i<waitingUnlockRequests.size(); i++ ){
    const mace::EventOperationInfo& eop = waitingUnlockRequests[i];
    sl.unlock();
    bool executed = false;
    if( eop.eventId == eventId ) {
      executed = this->unlockContextWithoutLock( contextStructure, eop, permittedEventOps, permittedContextNames, releaseContexts);
    }

    if( !executed ) {
      waiting_unlock_requests.push_back(eop);
    } else {
      macedbg(1) << "Executed waiting unlock request: " << eop << " in " << this->contextName << Log::endl;
    }
  }

  if( waitingUnlockRequests.size() != waiting_unlock_requests.size() ) {
    waitingUnlockRequests = waiting_unlock_requests;
  }
}

void ContextEventDominator::addWaitingUnlockRequest( const mace::EventOperationInfo& eop ) {
  ADD_SELECTORS("ContextEventDominator::addWaitingUnlockRequest");
  ScopedLock sl(dominatorMutex);
  waitingUnlockRequests.push_back(eop);
  macedbg(1) << "Add waiting unlock request " << eop << " to " << this->contextName << Log::endl;
}

bool ContextEventDominator::releaseContext( ContextStructure& contextStructure, mace::OrderID const& eventId, 
    mace::string const& lockedContextName, mace::string const& srcContext,
    mace::map<mace::string, mace::vector<mace::EventOperationInfo> >& permittedEventOps, 
    mace::map< mace::OrderID, mace::vector<mace::string> >& permittedContextNames,
    mace::vector<mace::string>& releaseContexts ) {
  ADD_SELECTORS("ContextEventDominator::releaseContext");
  macedbg(1) << "Event("<< eventId <<") release context("<< lockedContextName <<") in dominator("<< this->contextName <<")!" << Log::endl;
  ASSERT( checkContextInclusion(lockedContextName) );

  ScopedLock sl(dominatorMutex);
  bool exist = false;
  uint16_t lock_type;
  mace::set<mace::string> locked_contexts;
  for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ) {
    DomLockRequest& request = domLockRequestQueue[i];
    if( request.eventId == eventId ){
      releaseContexts = request.releaseContext(this->contextName, srcContext, lockedContextName, contextStructure);
      lock_type = request.lockType;
      locked_contexts = request.lockedContexts;
      exist = true;
      break;
    }
  }
  if( !exist ) {
    maceerr << "Fail to find event("<< eventId <<") in context("<< this->contextName <<")'s domLockQueue!" << Log::endl;
    ASSERT(exist);
  }
  
  if( lock_type == LockRequest::WLOCK || lock_type == LockRequest::RLOCK ){
    mace::vector<LockRequest>& queue = eventOrderQueue[lockedContextName];
    bool exist_flag = false;
    for( mace::vector<LockRequest>::iterator iter=queue.begin(); iter!=queue.end(); iter++ ){
      LockRequest& lockrequest = *iter;
      if( lockrequest.eventId == eventId ){
        macedbg(1) << "Mark LockReuqeust of " << eventId << " as UNLOCK for "<< lockedContextName <<" in " << this->contextName << Log::endl;
        lockrequest.lockType = mace::LockRequest::UNLOCK;
        exist_flag = true;
        break;
      }
    }
    if( !exist_flag ) {
      for( uint32_t i=0; i<queue.size(); i++ ) {
        macedbg(1) << queue[i] << Log::endl;
      }

      maceerr << "In dominator("<< this->contextName <<"), event("<< eventId <<") does not lock " << lockedContextName << Log::endl;
      ASSERT(false);
    }
    // macedbg(1) << "Event("<< eventId <<") release "<< lockedContextName <<" in dominator("<< this->contextName <<")!" << Log::endl;
  }
  this->checkEventOrderQueue( contextStructure, eventId, lock_type, locked_contexts, permittedEventOps, permittedContextNames, 
    releaseContexts );
  return true;
}

void ContextEventDominator::checkEventOrderQueue( ContextStructure& ctxStructure, const mace::OrderID& eventId, 
    const uint16_t& lock_type, const mace::set<mace::string>& locked_contexts,
    mace::map<mace::string, mace::vector<mace::EventOperationInfo> >& permittedEventOps,
    mace::map< mace::OrderID, mace::vector<mace::string> >& permittedContextNames, mace::vector< mace::string >& releaseContexts ) {
  ADD_SELECTORS("ContextEventDominator::checkEventOrderQueue");
  bool to_continue = true;
  while( to_continue ) {
    to_continue = false;
    for( mace::vector<DomLockRequest>::iterator iter = domLockRequestQueue.begin(); iter != domLockRequestQueue.end(); iter ++ ) {
      if( (*iter).toRemove() ) {
        macedbg(1) << "To remove request("<< (*iter).eventId <<") from domLockRequestQueue in dominator("<< this->contextName <<")!" << Log::endl; 
        domLockRequestQueue.erase(iter);
        to_continue = true;
        break;
      }
    }
  }

  // Check next request which could lock the dominator
  mace::vector<mace::EventOperationInfo> toLockEventOps;

  for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ) {
    DomLockRequest& request = domLockRequestQueue[i];
    if( !request.hasNotified ){
      if( request.lockType == LockRequest::DLOCK && i != 0 ) {
        break;
      }

      request.hasNotified = true;
      mace::vector<mace::EventOperationInfo>& lock_ops = request.lockOps;
      ASSERT( lock_ops.size() > 0 );
      for( uint32_t j=0; j<lock_ops.size(); j++ ) {
        ASSERT( lock_ops[j].requireContextName != "" );
        if( request.lockType == LockRequest::DLOCK ) { // next available request is DLOCK
          permittedEventOps[ lock_ops[j].requireContextName ].push_back(lock_ops[j]);
        } else {
          toLockEventOps.push_back(lock_ops[j]);
        }
      }
      if( request.lockType == LockRequest::DLOCK ){
        permittedContextNames[request.eventId] = dominateContexts;
      }
    }

    if( request.lockType == LockRequest::DLOCK ){
      break;
    }
  }

  // Enqueue next READ/WRITE eops into eventOrderQueue
  for( uint32_t i=0; i<toLockEventOps.size(); i++ ) {
    this->enqueueEventOrderQueue( ctxStructure, toLockEventOps[i] );
  }

  // remove vlock & unlock request of eventId from all queues
  if( lock_type == LockRequest::WLOCK || lock_type == LockRequest::RLOCK ){
    mace::set<mace::string> lockedContextNames; // contexts are locked by event
    for( uint32_t i=0; i<dominateContexts.size(); i++ ){
      bool continue_flag = false;
      for( mace::set<mace::string>::const_iterator cIter = lockedContextNames.begin(); cIter!=lockedContextNames.end(); cIter++ ){
        // if this context's ancestor contexts are locked by event, to ingore this event
        if( ctxStructure.isElderContext( dominateContexts[i], *cIter) ) {
          macedbg(1) << "context("<< *cIter <<") is the parent of context("<< dominateContexts[i] <<"). And former context is locked by event("<< eventId <<"). Ignore this queue!" << Log::endl;
          continue_flag = true;
          break;
        }
      }

      if( continue_flag ){
        continue;
      }

      mace::vector<LockRequest>& queue = eventOrderQueue[ dominateContexts[i] ];

      for( mace::vector<LockRequest>::iterator iter=queue.begin(); iter!=queue.end(); iter++ ) {
        LockRequest& lockRequest = *iter;
        if( lockRequest.eventId == eventId ){
          if( lockRequest.lockType == LockRequest::UNLOCK ) {
            releaseContexts.push_back( dominateContexts[i] );
            queue.erase(iter);
            macedbg(1) << "Remove UNLOCK of event("<< eventId <<") from " << dominateContexts[i] << " in dominator " << this->contextName << Log::endl;
          } else if( lockRequest.lockType == LockRequest::VRLOCK || lockRequest.lockType == LockRequest::VWLOCK ) {
            queue.erase(iter);
            if( locked_contexts.count(dominateContexts[i]) > 0 ) {
              macedbg(1) << "Event("<< eventId <<") has locked context("<< dominateContexts[i] <<"), should release it!" << Log::endl;
              releaseContexts.push_back( dominateContexts[i] );
            }
            macedbg(1) << "Remove VLOCK of event("<< eventId <<") from " << dominateContexts[i] << " in dominator " << this->contextName << Log::endl;
          } else {
            macedbg(1) << "In Dominator("<< this->contextName <<") event("<< eventId <<") locked " << dominateContexts[i] << Log::endl;
            lockedContextNames.insert( dominateContexts[i] );
          }
          break;
        }
      }
    }
  }

  // check next executing events
  for( uint32_t i=0; i<dominateContexts.size(); i++ ){
    mace::vector<LockRequest>& queue = eventOrderQueue[ dominateContexts[i] ];
    for( mace::vector<LockRequest>::iterator iter=queue.begin(); iter!=queue.end(); iter++ ) {
      LockRequest& lockRequest = *iter;
      // macedbg(1) << "Check lock request: " << lockRequest << Log::endl;
      if( (lockRequest.lockType == LockRequest::WLOCK || lockRequest.lockType == LockRequest::VWLOCK) && iter != queue.begin() ){
        break;
      }

      if( !lockRequest.hasNotified && lockRequest.lockType != LockRequest::UNLOCK  ) {
        lockRequest.hasNotified = true;
        mace::vector<EventOperationInfo> eops = lockRequest.getEventOps();
        for( uint32_t j=0; j<eops.size(); j++ ){
          permittedEventOps[ eops[j].requireContextName ].push_back( eops[j] );
        }
        permittedContextNames[ lockRequest.eventId ].push_back( dominateContexts[i] );
      }
        
      if( lockRequest.lockType == LockRequest::WLOCK || lockRequest.lockType == LockRequest::VWLOCK 
          || lockRequest.lockType == LockRequest::UNLOCK ){
        break;
      } 
    }
  }
}

void ContextEventDominator::addLockedContexts( const mace::OrderID& eventId, const mace::vector<mace::string>& locked_contexts ) {
  ScopedLock sl(dominatorMutex);
  bool exist = false;
  for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ) {
    DomLockRequest& request = domLockRequestQueue[i];
    if( request.eventId == eventId ){
      exist = true;
      request.addLockedContexts(locked_contexts);
      break;
    }
  }
  ASSERT(exist);
}

void ContextEventDominator::addLockedContext( const mace::OrderID& eventId, const mace::string& ctx_name ) {
  ScopedLock sl(dominatorMutex);
  bool exist = false;
  for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ) {
    DomLockRequest& request = domLockRequestQueue[i];
    if( request.eventId == eventId ){
      exist = true;
      request.addLockedContext(ctx_name);
      break;
    }
  }
  ASSERT(exist);
}

uint32_t ContextEventDominator::getLockedContextsNumber( mace::OrderID const& eventId ) {
  uint32_t n = 0;
  ScopedLock sl(dominatorMutex);
  for( uint32_t i=0; i<dominateContexts.size(); i++ ){
    mace::vector<LockRequest>& queue = eventOrderQueue[ dominateContexts[i] ];

    for( mace::vector<LockRequest>::iterator iter=queue.begin(); iter!=queue.end(); iter++ ) {
      LockRequest& lockRequest = *iter;
      if( lockRequest.eventId == eventId ){
        n++;
      }
    }
  }
  return n;
}

void mace::ContextEventDominator::serialize(std::string& str) const{
  mace::serialize( str, &contextName );
  mace::serialize( str, &preDominator );
  mace::serialize( str, &curDominator );
  mace::serialize( str, &version );
  mace::serialize( str, &dominateContexts );
  mace::serialize( str, &eventOrderQueue );
  mace::serialize( str, &domLockRequestQueue );
}

int mace::ContextEventDominator::deserialize(std::istream & is) throw (mace::SerializationException){
  int serializedByteSize = 0;
  serializedByteSize += mace::deserialize( is, &contextName );
  serializedByteSize += mace::deserialize( is, &preDominator );
  serializedByteSize += mace::deserialize( is, &curDominator );
  serializedByteSize += mace::deserialize( is, &version );
  serializedByteSize += mace::deserialize( is, &dominateContexts   );
  serializedByteSize += mace::deserialize( is, &eventOrderQueue   );
  serializedByteSize += mace::deserialize( is, &domLockRequestQueue );
  return serializedByteSize;
}

bool mace::ContextEventDominator::updateDominator( ContextStructure& ctxStructure, mace::vector<mace::EventOperationInfo>& feops ) {
  ADD_SELECTORS("ContextEventDominator::updateDominator");
  ScopedLock sl(dominatorMutex);
  
  this->preDominator = this->curDominator;
  this->curDominator = ctxStructure.getUpperBoundContextName(this->contextName);
  dominateContexts = ctxStructure.getDominateContexts( this->contextName );

  mace::set<mace::string> dominated_ctx_set;
  for( uint32_t i=0; i<dominateContexts.size(); i++ ){
    dominated_ctx_set.insert( dominateContexts[i] );
  }

  macedbg(1) << "context("<< this->contextName <<")'s preDominator="<< this->preDominator <<", curDominator=" << this->curDominator << " and dominate: " << dominateContexts << Log::endl; 
  
  eventOrderQueue.clear();
  mace::vector<LockRequest> queue;
  for( uint32_t i=0; i<dominateContexts.size(); i++ ) {
    eventOrderQueue[ dominateContexts[i] ] = queue;
  }

  mace::set<mace::OrderID> to_remove_eventIds;
  for( uint32_t i=0; i<domLockRequestQueue.size(); i++ ) {
    DomLockRequest& request = domLockRequestQueue[i];
    mace::vector<mace::EventOperationInfo>& eops = request.lockOps;
    bool to_continue = true;
    while(to_continue) {
      to_continue = false;
      for( mace::vector<mace::EventOperationInfo>::iterator iter = eops.begin(); iter != eops.end(); iter ++ ) {
        mace::EventOperationInfo& eop = *iter;
        if( ctxStructure.getUpperBoundContextName( eop.requireContextName ) != this->contextName ) {
          macedbg(1) << eop << " has new dominator("<< ctxStructure.getUpperBoundContextName( eop.requireContextName ) <<") instead of old dominator("<< this->contextName <<")!" << Log::endl; 
          feops.push_back( *iter );
          
          eops.erase(iter);
          to_continue = true;
          break;
        }
      }
    }
    if( eops.size() == 0 ) {
      to_remove_eventIds.insert( request.eventId );
    } else {
      request.removeNotLockedContext(dominated_ctx_set);
    }
  }

  bool to_continue = true;
  while(to_continue) {
    to_continue = false;
    for( mace::vector<DomLockRequest>::iterator iter = domLockRequestQueue.begin(); iter != domLockRequestQueue.end(); iter ++ ) {
      mace::DomLockRequest& request = *iter;
      if( to_remove_eventIds.count(request.eventId) > 0 ) {
        domLockRequestQueue.erase(iter);
        to_continue = true;
        break;
      }
    }
  }
  if( dominateContexts.size() == 0 ) {
    ASSERT(domLockRequestQueue.size() == 0);
  }

  return true; 
}

void mace::ContextEventDominator::addUpdateSourceContext( const mace::string& ctx_name ) {
  ScopedLock sl(dominatorMutex);
  if( updateSourceContexts.count(ctx_name) == 0 ) {
    updateSourceContexts.insert(ctx_name);
  }
}

void mace::ContextEventDominator::addUpdateWaitingContext( const mace::string& ctx_name ) {
  ScopedLock sl(dominatorMutex);
  if( updateWaitingContexts.count(ctx_name) == 0 && ctx_name != this->contextName ){
    updateWaitingContexts.insert(ctx_name);
  }
}

void mace::ContextEventDominator::setWaitingUnlockRequests( const mace::vector<mace::EventOperationInfo>& eops ) {
  ScopedLock sl(dominatorMutex);
  updateReplyEventOps = eops;
}

bool mace::ContextEventDominator::isUpdateWaitingForReply() {
  ScopedLock sl(dominatorMutex);
  if( updateWaitingContexts.size() > 0 ){
    return true;
  } else {
    return false;
  }
}

void mace::ContextEventDominator::clearUpdateSourceContexts() {
  ScopedLock sl(dominatorMutex);
  updateSourceContexts.clear();
}

// void mace::ContextEventDominator::addReplyEventOps( const mace::vector<mace::EventOperationInfo>& reply_eops ) {
//   ScopedLock sl(dominatorMutex);
//   for( uint32_t i=0; i<reply_eops.size(); i++ ){
//     bool exist = false;
//     for( uint32_t j=0; j<updateReplyEventOps.size(); j++ ) {
//       if( reply_eops[i] == updateReplyEventOps[j] ){
//         exist = true;
//         break;
//       }
//     }
//     if( !exist ){
//       updateReplyEventOps.push_back(reply_eops[i]);
//     }
//   }
// }

void mace::ContextEventDominator::addReplyEventOp( const mace::EventOperationInfo& reply_eop ) {
  ScopedLock sl(dominatorMutex);
  mace::EventOperationInfo eop = reply_eop;
  bool exist = false;
  for( uint32_t j=0; j<updateReplyEventOps.size(); j++ ) {
    if( eop == updateReplyEventOps[j] ){
      exist = true;
      break;
    }
  }
  
  if( !exist ){
    updateReplyEventOps.push_back(reply_eop);
  }
}

// bool mace::ContextEventDominator::updateEventQueue( BaseMaceService* sv, mace::string const& src_contextName, 
//     mace::ContextEventDominator::EventLockQueue const& lockQueues, mace::vector<DominatorRequest>& dominatorRequests){
//   ADD_SELECTORS("ContextEventDominator::updateEventQueue");
//   ScopedLock sl(dominatorMutex);
//   macedbg(1) << "Dominator("<< this->contextName <<") receive eventqueues("<< lockQueues <<") from " << src_contextName <<" waitingContexts=" << waitingContexts << Log::endl;
//   waitingContextsQueues[ src_contextName ] = lockQueues;
//   dominatorRequests = waitingDominatorRequests;

//   ContextService* _service = static_cast<ContextService*>(sv);
//   return checkAndUpdateEventQueues(_service->contextStructure);
// }

// bool mace::ContextEventDominator::checkAndUpdateEventQueues(ContextStructure& contextStructure) {
//   ADD_SELECTORS("ContextEventDominator::checkAndUpdateEventQueues");
//   if( updateDominatorFlag && waitingContexts.size() == 0 ) {
//     waitingContexts.clear();
//     waitingContextsQueues.clear();
//     updateDominatorFlag = false;

//     for( uint32_t i=0; i<dominateContexts.size(); i++ ){
//       if( eventOrderQueue.find(dominateContexts[i]) == eventOrderQueue.end() ) {
//         mace::vector<LockRequest> lockqueue;
        
//         ASSERT( eventOrderQueue.find(this->contextName) != eventOrderQueue.end() );
//         mace::vector<LockRequest>& domLockQueue = eventOrderQueue[ this->contextName ];
//         for( uint32_t j=0; j<domLockQueue.size(); j++ ){
//           if( domLockQueue[j].lockType == LockRequest::WLOCK ){
//             LockRequest lockRequest( LockRequest::VWLOCK, dominateContexts[i], domLockQueue[j].eventId );
//             lockqueue.push_back(lockRequest);
//           } else if( domLockQueue[j].lockType == LockRequest::RLOCK ){
//             LockRequest lockRequest( LockRequest::VRLOCK, dominateContexts[i], domLockQueue[j].eventId );
//             lockqueue.push_back(lockRequest);
//           }
//         }
//         eventOrderQueue[ dominateContexts[i] ] = lockqueue;
//         macedbg(1) << "Dominator("<< this->contextName <<") add context("<< dominateContexts[i] <<")'s queue!" << Log::endl;
//       }
//     }

//     macedbg(1) << "Dominator("<< this->contextName <<") needn't wait for any eventqueues!" << Log::endl;
//     return true;
//   }

//   if( !updateDominatorFlag || waitingContexts.size() != waitingContextsQueues.size() ){
//     return false;
//   }

//   for( mace::set<mace::string>::iterator iter = waitingContexts.begin(); iter!=waitingContexts.end(); iter++ ){
//     if( waitingContextsQueues.find(*iter) == waitingContextsQueues.end() ) {
//       macedbg(1) << "ERROR: fail to find eventqueues from " << *iter << Log::endl;
//       return false;
//     }
//   }

//   macedbg(1) << "To update the dominator " << this->contextName << Log::endl;
//   for( mace::set<mace::string>::iterator iter = waitingContexts.begin(); iter!=waitingContexts.end(); iter++ ){
//     EventLockQueue& lockQueues = waitingContextsQueues[ *iter ];
//     for( EventLockQueue::iterator newIter = lockQueues.begin(); newIter!=lockQueues.end(); newIter++ ){
//       macedbg(1) << "context("<< newIter->first <<")'s queue("<< (newIter->second).size() <<") from " << *iter << Log::endl;
//       if( eventOrderQueue.find(newIter->first) != eventOrderQueue.end() ) { // there are two queues for the same context
//         macedbg(1) << "There are two queues of context("<< *iter <<") in dominator " << this->contextName << Log::endl;

//         if( eventOrderQueue[newIter->first].size() == 0 ) {
//           eventOrderQueue[newIter->first] = newIter->second;
//           continue;
//         }

//         if( *iter == newIter->first ){
//           mace::vector<LockRequest>& oldQueue = eventOrderQueue[newIter->first];
//           mace::vector<LockRequest>& newQueue = newIter->second;

//           // if one event is labeled as UNLOCK, it should be unlock
//           for( uint32_t m=0; m<oldQueue.size(); m++ ){
//             if( oldQueue[m].lockType == LockRequest::UNLOCK ){
//               for( uint32_t n=0; n<newQueue.size(); n++ ){
//                 if( newQueue[n].eventId == oldQueue[m].eventId ){
//                   newQueue[n].lockType = LockRequest::UNLOCK;
//                   break;
//                 }
//               }
//             }
//           }

//           eventOrderQueue[ newIter->first ] = newQueue;
//         } else {
//           mace::vector<LockRequest>& oldQueue = newIter->second;
//           mace::vector<LockRequest>& newQueue = eventOrderQueue[newIter->first];

//           // if one event is labeled as UNLOCK, it should be unlock
//           for( uint32_t m=0; m<oldQueue.size(); m++ ){
//             if( oldQueue[m].lockType == LockRequest::UNLOCK ){
//               for( uint32_t n=0; n<newQueue.size(); n++ ){
//                 if( newQueue[n].eventId == oldQueue[m].eventId ){
//                   newQueue[n].lockType = LockRequest::UNLOCK;
//                   break;
//                 }
//               }
//             }
//           }

//         }
//       } else {
//         eventOrderQueue[ newIter->first ] = newIter->second;
//       }
//     }
//   }

//   for( uint32_t i=0; i<dominateContexts.size(); i++ ){
//     if( eventOrderQueue.find(dominateContexts[i]) == eventOrderQueue.end() ) {
//       mace::vector<LockRequest> lockqueue;
//       eventOrderQueue[ dominateContexts[i] ] = lockqueue;
//       macedbg(1) << "Dominator("<< this->contextName <<") add context("<< dominateContexts[i] <<")'s queue!" << Log::endl;
//     }
//   }

//   //remove unnecessary lockrequests
//   mace::map< mace::OrderID, mace::set<mace::string> > lockedContextNames; // contexts are locked by event
//   mace::map< mace::OrderID, mace::set<mace::string> > existContextNames;
//   for( uint32_t i=0; i<dominateContexts.size(); i++ ){
//     mace::vector<LockRequest>& queue = eventOrderQueue[ dominateContexts[i] ];

//     for( mace::vector<LockRequest>::iterator iter=queue.begin(); iter!=queue.end(); iter++ ) {
//       LockRequest& lockRequest = *iter;
      
//       existContextNames[ lockRequest.eventId ].insert( dominateContexts[i] );
//       if( lockRequest.lockType == LockRequest::RLOCK || lockRequest.lockType == LockRequest::WLOCK ){
//         lockedContextNames[ lockRequest.eventId ].insert( dominateContexts[i] );
//       }
//     }
//   }

//   for( uint32_t i=0; i<dominateContexts.size(); i++ ){
//     mace::vector<LockRequest>& queue = eventOrderQueue[ dominateContexts[i] ];

//     bool continue_flag = true;
//     while( continue_flag ){
//       continue_flag = false;
//       for( mace::vector<LockRequest>::iterator iter=queue.begin(); iter!=queue.end(); iter++ ) {
//         LockRequest& lockRequest = *iter;
//         if( lockRequest.lockType == LockRequest::VRLOCK || lockRequest.lockType == LockRequest::VWLOCK ){
//           if( lockedContextNames.find(lockRequest.eventId) == lockedContextNames.end() ){
//             macedbg(1) << "Remove " << *iter << " from lockedContext("<< dominateContexts[i] <<") in " << this->contextName << Log::endl;
//             queue.erase(iter);
//             continue_flag = true;
//             break;
//           } else {
//             mace::set< mace::string >& lockedCtxNames = lockedContextNames[ lockRequest.eventId ];
//             mace::set< mace::string >::iterator sIter = lockedCtxNames.begin();
//             for( ; sIter != lockedCtxNames.end(); sIter ++ ){
//               if( contextStructure.isElderContext(dominateContexts[i], *sIter) ){
//                 break;
//               }
//             }
//             if( sIter == lockedCtxNames.end() ){
//               macedbg(1) << "Remove " << *iter << " from lockedContext("<< dominateContexts[i] <<") in " << this->contextName << Log::endl;
//               queue.erase(iter);
//               continue_flag = true;
//               break;
//             }

//           }
//         }
//       }
//     }
//   }

//   // add new vlock
//   mace::vector<LockRequest>& queue = eventOrderQueue[ this->contextName ];
//   for( uint32_t i=0; i<queue.size(); i++ ){
//     LockRequest& lrequest = queue[i];
//     if( lrequest.lockType == LockRequest::WLOCK || lrequest.lockType == LockRequest::RLOCK ) {
//       for( uint32_t j=0; j<dominateContexts.size(); j++ ){
//         if( dominateContexts[j] == this->contextName ){
//           continue;
//         }

//         if( existContextNames[lrequest.eventId].count(dominateContexts[j]) == 0 ){
//           uint16_t vlockType;
//           if( lrequest.lockType == LockRequest::WLOCK ){
//             vlockType = LockRequest::VWLOCK;
//           } else {
//             vlockType = LockRequest::VRLOCK;
//           }

//           LockRequest lockRequest( vlockType, dominateContexts[j], lrequest.eventId );
//           macedbg(1) << "Enqueue vlock "<< lockRequest <<" into context("<< dominateContexts[j] <<") in dominator("<< this->contextName <<")." << Log::endl;
//           eventOrderQueue[ dominateContexts[j] ].push_back(lockRequest);
//         }
//       }
//     }
//   }

//   printEventQueues();
//   updateDominatorFlag = false;
//   waitingContexts.clear();
//   waitingContextsQueues.clear();

  
//   return true;
// }

// void mace::ContextEventDominator::adjustEventQueue() {
//   ADD_SELECTORS("ContextEventDominator::adjustEventQueue");

// }

// mace::vector<DominatorRequest> mace::ContextEventDominator::checkWaitingDominatorRequests( mace::OrderID const& eventId ) {
//   ADD_SELECTORS("ContextEventDominator::checkWaitingDominatorRequests");
//   macedbg(1) << "Check delay requests for " << eventId << Log::endl;
//   ScopedLock sl( dominatorMutex );
//   mace::vector<DominatorRequest> dominatorRequests;

//   if( updateDominatorFlag || waitingDominatorRequests.size() == 0 ){
//     return dominatorRequests;
//   }


//   bool continue_flag = true;
//   while(continue_flag) {
//     continue_flag = false;
//     mace::vector<DominatorRequest>::iterator iter;
//     for( iter = waitingDominatorRequests.begin(); iter != waitingDominatorRequests.end(); iter++ ){
//       const DominatorRequest& request = *iter;
//       if( request.lockType == DominatorRequest::UNLOCK_CONTEXT && request.eventOpInfo.eventId == eventId ){
//         continue_flag = true;
//         dominatorRequests.push_back( request );
//         waitingDominatorRequests.erase(iter);
//         break;
//       }
//     }
//   }
//   return dominatorRequests;
// }

void mace::ContextEventDominator::printEventQueues() {
  ADD_SELECTORS("ContextEventDominator::printEventQueues");
  for( EventLockQueue::iterator iter = eventOrderQueue.begin(); iter!=eventOrderQueue.end(); iter++ ){
    macedbg(1) << "context("<< iter->first <<") queue: " << Log::endl;
    mace::vector<LockRequest>& lockqueue = iter->second;
    for( uint32_t i=0; i<lockqueue.size(); i++ ){
      macedbg(1) << lockqueue[i] << Log::endl;
    }
  }

}

template<class T> pthread_mutex_t ObjectPool< T >::lock = PTHREAD_MUTEX_INITIALIZER;
