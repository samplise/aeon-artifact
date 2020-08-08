#include "HeadEventDispatch.h"
//#include "ContextBaseClass.h"
#include "HierarchicalContextLock.h"
#include "Event.h"
#include <queue>
#include <list>
#include "ContextService.h"
#include "Log.h"
#include "mace-macros.h"


HeadEventDispatch::EventRequestTSType HeadEventDispatch::eventRequestTime;
// the timestamp where the event request is processed
HeadEventDispatch::EventRequestTSType HeadEventDispatch::eventStartTime;
pthread_mutex_t HeadEventDispatch::startTimeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t HeadEventDispatch::requestTimeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t HeadEventDispatch::samplingMutex = PTHREAD_MUTEX_INITIALIZER;
bool HeadEventDispatch::sampleEventLatency = false;
uint32_t HeadEventDispatch::accumulatedLatency = 0;
uint32_t HeadEventDispatch::accumulatedEvents = 0;

uint64_t HeadEventDispatch::HeadEventTP::now_serving_ticket = 1;
uint64_t HeadEventDispatch::HeadEventTP::next_ticket = 1;

bool HeadEventDispatch::HeadEventTP::migrating_flag = false;

uint64_t HeadEventDispatch::HeadEventTP::req_start_time = 0;
uint8_t HeadEventDispatch::HeadEventTP::req_type = 0;

uint64_t HeadEventDispatch::HeadEventTP::total_latency = 0;
uint64_t HeadEventDispatch::HeadEventTP::create_latency = 0;
uint64_t HeadEventDispatch::HeadEventTP::ownership_latency = 0;
uint64_t HeadEventDispatch::HeadEventTP::migrating_latency = 0;

uint64_t HeadEventDispatch::HeadEventTP::total_req_count = 0;
uint64_t HeadEventDispatch::HeadEventTP::create_req_count = 0;
uint64_t HeadEventDispatch::HeadEventTP::ownership_req_count = 0;
uint64_t HeadEventDispatch::HeadEventTP::migrating_req_count = 0;

uint64_t HeadEventDispatch::HeadEventTP::GLOBAL_EVENT_OUTPUT = 0;


bool HeadEventDispatch::HeadEventTP::migrating_waiting_flag = false; 
mace::set<uint64_t> HeadEventDispatch::HeadEventTP::waitingCommitEventIds;

mace::map<uint32_t, mace::string> HeadEventDispatch::HeadEventTP::migratingContexts;
mace::map<uint32_t, mace::string> HeadEventDispatch::HeadEventTP::migratingContextsCopy;
mace::map< mace::MaceAddr, mace::set<mace::string> > HeadEventDispatch::HeadEventTP::migratingContextsOrigAddrs;
mace::MaceAddr HeadEventDispatch::HeadEventTP::destAddr;

HeadEventDispatch::MessageQueue HeadEventDispatch::HeadTransportTP::mqueue;

namespace HeadEventDispatch {
  template<class T>
  class HeadEventCommitQueue{
  private:
    typedef std::deque< T > QueueType;
  public:
    HeadEventCommitQueue() {

    }
    bool empty(){ return queue.empty(); }
    T top() const{ return queue.front(); }
    void pop() {
      queue.pop_front();
    }
    void push( T event ){
      queue.push_back(event);
    }
  private:
    QueueType queue;
  };

  template<class T>
  class HeadEventQueue{
  private:
    typedef std::deque< T > QueueType;
  public:
    HeadEventQueue(): offset(0), frontmost(0), size(0) {

    }
    bool empty(){ return size==0; }
    T top() const{ 
      if( frontmost-offset-1 == 0 )
        return queue.front();
      else
        return queue[ frontmost-offset-1]; 
    }
    void pop() {
      ADD_SELECTORS("HeadEventQueue::pop");
      typename QueueType::size_type eraseLength = frontmost-offset;
      
      frontmost++;
      typename QueueType::const_iterator qIt = queue.begin()+( frontmost-offset-1 );
      while( frontmost-offset < queue.size() && 
        //queue[ frontmost-offset-1 ].ticket == 0  ){
        qIt->ticket == 0  ){

        frontmost++;
        qIt++;
      }

      offset+=eraseLength;
      size--;

      if( eraseLength == 1 ){
        queue.pop_front();
      }else{
        queue.erase( queue.begin(), queue.begin()+ (eraseLength ) );
      }
      //macedbg(1)<<"after pop, frontmost="<<frontmost<<", offset="<<offset<<", size="<<size<<" container size"<< queue.size()<<Log::endl;
    }
    void push( T event ){
      ADD_SELECTORS("HeadEventQueue::push");
      typename QueueType::size_type qsize = queue.size();
      if( frontmost > event.ticket ){
        frontmost = event.ticket;
      }else if( qsize == 0 ){
        frontmost = event.ticket;
      }

      if( event.ticket - offset > qsize ){ // queue not long enough
        queue.resize( event.ticket - offset );
      }
      //ASSERT( queue[ event->eventID-offset-1 ] == NULL );
      queue[ event.ticket-offset-1 ] = event;

      size++;

      //macedbg(1)<<"after push, frontmost="<<frontmost<<", offset="<<offset<<", size="<<size<<" container size"<< queue.size()<<Log::endl;
    }
  private:
    uint64_t offset;
    uint64_t frontmost;
    uint64_t size;
    QueueType queue;
  };

  template<typename T>
  struct QueueComp{
    bool operator()( const T& p1, const T& p2 ){
      return p1.eventId > p2.eventId;
    }
  };
  //typedef std::pair<uint64_t, HeadEventDispatch::HeadEvent> RQType;
  typedef HeadEventDispatch::HeadEvent RQType;
  typedef std::priority_queue< RQType, std::vector< RQType >, QueueComp<HeadEventDispatch::HeadEvent> > EventRequestQueueType;
	//bsang
	typedef HeadEventDispatch::GlobalHeadEvent* GRQType;
	typedef std::queue<GRQType> GlobalEventRequestQueueType;

  //typedef HeadEventQueue<HeadEventDispatch::HeadEvent> EventRequestQueueType;
  typedef HeadEventCommitQueue< mace::Event* > EventCommitQueueType;

  EventRequestQueueType headEventQueue;///< used by head context
  EventCommitQueueType headCommitEventQueue;

  GlobalEventRequestQueueType globalHeadEventQueue;

  typedef HeadEventDispatch::ElasticityHeadEvent* ERQType;
  typedef std::queue<ERQType> ElasticityEventRequestQueueType;
  ElasticityEventRequestQueueType elasticityHeadEventQueue;

  // memory pool for events
  // TODO: make it a Singleton
  // TODO: Lockfree queue is supported in boost 1.53.0 Use it some day in the future.
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
      /*T* obj;
      if( ! objqueue.pop( obj ) ){
        obj = new T;
      }
      return obj;*/
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
    //boost::lockfree::queue< T* > objqueue;
  };
  ObjectPool< mace::Event > eventObjectPool;


  // the timestamp where the event request is created

  bool halting = false;
  bool halted = false;
  mace::OrderID exitEventId;
  bool haltingCommit = false;


  uint32_t minThreadSize;
  uint32_t maxThreadSize;
  uint32_t minGlobalThreadSize;
  uint32_t maxGlobalThreadSize;
  pthread_t* HeadEventTP::headThread;
  pthread_t* HeadEventTP::globalHeadThread;
  pthread_t HeadEventTP::headCommitThread;

  pthread_mutex_t HeadEventTP::executingSyncMutex;
  pthread_cond_t HeadEventTP::executingSyncCond;

  pthread_mutex_t HeadMigration::lock = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t eventQueueMutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t commitQueueMutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t rpcWaitMutex = PTHREAD_MUTEX_INITIALIZER;

	pthread_mutex_t globalEventQueueMutex = PTHREAD_MUTEX_INITIALIZER;
  pthread_mutex_t executeGlobalEventMutex = PTHREAD_MUTEX_INITIALIZER;

  pthread_cond_t migratingContextCond = PTHREAD_COND_INITIALIZER;

  pthread_mutex_t elasticityEventQueueMutex = PTHREAD_MUTEX_INITIALIZER;

  uint16_t HeadMigration::state = HeadMigration::HEAD_STATE_NORMAL;
  mace::OrderID HeadMigration::migrationEventId;
  mace::MaceAddr HeadMigration::newHeadAddr;
  mace::set<mace::string> HeadEventTP::waitingReplyContextNames;

  pthread_t* HeadEventTP::elasticityHeadThread;
  mace::set<mace::string> HeadEventTP::waitingMigrationContexts;

  HeadTransportTP* _tinst;
  HeadTransportTP* HeadTransportTPInstance() {
    return _tinst;
  }

  void insertEventStartTime(const mace::OrderID& eventID){
    ScopedLock sl( startTimeMutex );
    eventStartTime[ eventID ] = TimeUtil::timeu() ;
  }
  void insertEventRequestTime(const mace::OrderID& eventID){
    ScopedLock sl( requestTimeMutex );
    eventRequestTime[ eventID ] = TimeUtil::timeu() ;
  }

  mace::ContextBaseClass* HeadEvent::getCtxObj() {
    if(eventId.ctxId > 0) {
      ContextService* sv = static_cast<ContextService*>( cl ); 
      return sv->getContextObjByID(eventId.ctxId);
    } else {
      return NULL;
    }
  }
  

  HeadEventTP::HeadEventTP( const uint32_t minThreadSize, const uint32_t maxThreadSize, const uint32_t minGlobalThreadSize, const uint32_t maxGlobalThreadSize) :
    idle( 0 ),
    sleeping( NULL ),
    args( NULL ),
    busyCommit( false ),
    minThreadSize( minThreadSize ), 
    maxThreadSize( maxThreadSize ), 
    globalIdle( 0 ),
		minGlobalThreadSize( minGlobalThreadSize ), 
		maxGlobalThreadSize( maxGlobalThreadSize ),
    globalSleeping( NULL ),
    elasticityIdle( 0 ),
    minElasticityThreadSize( 1 ), 
    maxElasticityThreadSize( 1 ),
    elasticitySleeping( NULL ) {

    Log::log("HeadEventTP::constructor") << "Created HeadEventTP threadpool with " << minThreadSize << " threads. Max: "<< maxThreadSize <<" and global threads: "<< minGlobalThreadSize << " threads. Max: "<< maxGlobalThreadSize << Log::endl;

    ASSERT(pthread_cond_init(&signalv, 0) == 0);
    ASSERT(pthread_cond_init(&signalc, 0) == 0);
		ASSERT(pthread_cond_init(&signalg, 0) == 0);

    ASSERT( pthread_mutex_init(&executingSyncMutex, 0) == 0 );
    ASSERT( pthread_cond_init(&executingSyncCond, 0) == 0 );


    headThread = new pthread_t[ minThreadSize ];
    sleeping = new bool[ minThreadSize ];
    args = new ThreadArg[ minThreadSize ];

		globalHeadThread = new pthread_t[ minGlobalThreadSize ];
		globalSleeping = new bool[ minGlobalThreadSize ];
		globalArgs = new ThreadArg[ minGlobalThreadSize ];

    GLOBAL_EVENT_OUTPUT = params::get<uint64_t>("GLOBAL_EVENT_OUTPUT", 0 );

    for( uint32_t nThread = 0; nThread < minThreadSize; nThread++ ){
      sleeping[ nThread ] = 0;
      args[ nThread ].p = this;
      args[ nThread ].i = nThread;
      ASSERT(  pthread_create( & headThread[nThread] , NULL, HeadEventTP::startThread, (void*)&args[nThread] ) == 0 );
    }

		for( uint32_t nThread = 0; nThread < minGlobalThreadSize; nThread++ ){
			globalSleeping[ nThread ] = 0;
			globalArgs[ nThread ].p = this;
			globalArgs[ nThread ].i = nThread;
			ASSERT( pthread_create( &globalHeadThread[nThread], NULL, HeadEventTP::startGlobalThread, (void*)&globalArgs[nThread]) == 0 );
		}

    ASSERT(pthread_cond_init(&signale, 0) == 0);

    elasticityHeadThread = new pthread_t[ minElasticityThreadSize ];
    elasticitySleeping = new bool[ minElasticityThreadSize ];
    elasticityArgs = new ThreadArg[ minElasticityThreadSize ];

    for( uint32_t nThread = 0; nThread < minElasticityThreadSize; nThread++ ){
      elasticitySleeping[ nThread ] = 0;
      elasticityArgs[ nThread ].p = this;
      elasticityArgs[ nThread ].i = nThread;
      ASSERT( pthread_create( &elasticityHeadThread[nThread], NULL, HeadEventTP::startElasticityThread, (void*)&elasticityArgs[nThread]) == 0 );
    }

    ASSERT(  pthread_create( & headCommitThread, NULL, HeadEventTP::startCommitThread, (void*)this ) == 0 );
  }

  HeadEventTP::~HeadEventTP() {
    delete headThread;
    delete args;
    delete sleeping;

    delete globalHeadThread;
    delete globalArgs;
    delete globalSleeping;

    eventObjectPool.clear();
  }
  // cond func
  bool HeadEventTP::hasPendingEvents(){
    if( headEventQueue.empty() ) return false;
    ADD_SELECTORS("HeadEventTP::hasPendingEvents");
    HeadEventDispatch::HeadEvent const& top = headEventQueue.top();
    //macedbg(1)<<" top ticket = "<< top.ticket << Log::endl;
    //if( top.ticket == 0 ) return false;
    //if( halting == true && exitTicket <= top.ticket ){
		if( halting == true ) {
      halted = true;
      //macedbg(1)<<"halted! exitTicket=" << exitTicket << Log::endl;
    }

		//ContextBaseClass* ctxObj = top.cl->getContextObjByID(top.)
    ScopedLock sl( mace::AgentLock::_agent_ticketbooth);
    if( top.eventId.ticket == mace::AgentLock::now_serving ){
      return true;
    }
    return false;
  }

	bool HeadEventTP::hasPendingGlobalEvents() {
    ADD_SELECTORS("HeadEventTP::hasPendingGlobalEvents");
    macedbg(1) << "Check globalEventQueue="<< globalHeadEventQueue.size() << Log::endl;
		if( globalHeadEventQueue.empty() ){
			return false;
		}
    GRQType ge = globalHeadEventQueue.front();
    ASSERTMSG(ge->ticket >= now_serving_ticket, "Order problem in global event queue!");
		macedbg(1) << "There are pending global event, ticket = "<< ge->ticket << Log::endl;    
    if( halting == true ) {
      halted = true;
    }

    if( ge->ticket > now_serving_ticket ) {
      return false;
    }

    return true;
	}
  bool HeadEventTP::hasUncommittedEvents(){
    if( headCommitEventQueue.empty()  ) return false;
    ADD_SELECTORS("HeadEventTP::hasUncommittedEvents");

    mace::Event* top = headCommitEventQueue.top();
    if( top == NULL ) return false;

    const mace::OrderID& eventId = top->eventId;
    
    if( eventId.ctxId == 0 && eventId == mace::AgentLock::now_committing ){
      committingEvent = top;
      headCommitEventQueue.pop();
      return true;
    }
    ASSERT( eventId.ctxId != 0 || eventId > mace::AgentLock::now_committing );
    return false;
  }
  // setup
  void HeadEventTP::executeEventSetup( ){
      data = headEventQueue.top();
      ADD_SELECTORS("HeadEventTP::executeEventSetup");
      //macedbg(1)<<"erase&fire headEventQueue = " << data.eventId << Log::endl;
      headEventQueue.pop();
  }
	void HeadEventTP::executeGlobalEventSetup() {
    ADD_SELECTORS("HeadEventTP::executeGlobalEventSetup");
    globalEvent = globalHeadEventQueue.front();
    //macedbg(1) << "erase & fire a global event." << Log::endl;
		globalHeadEventQueue.pop();
  }


  void HeadEventTP::commitEventSetup( ){

  }
  // process
  void HeadEventTP::executeEventProcess() {
      data.fire();
  }
	void HeadEventTP::executeGlobalEventProcess() {
    ADD_SELECTORS("HeadEventTP::executeGlobalEventProcess");
    //macedbg(1) << " Start to process global event " << Log::endl;
    
    req_start_time = TimeUtil::timeu();
    if(globalEvent->globalEventType == GlobalHeadEvent::GlobalHeadEvent_CREATECONTEXT){
			req_type = GlobalHeadEvent::GlobalHeadEvent_CREATECONTEXT;
      executeGlobalContextCreateEventProcess();
    } else if( globalEvent->globalEventType == GlobalHeadEvent::GlobalHeadEvent_MIGRATION) {
      req_type = GlobalHeadEvent::GlobalHeadEvent_MIGRATION;
      macedbg(1) << "Start to execute migration event: " << globalEvent->ticket << Log::endl;
      migrating_flag = true;
      ContextService* _service = static_cast<ContextService*>(globalEvent->cl);
      HeadContextMigrationEvent* mevent = static_cast<HeadContextMigrationEvent*>(globalEvent->helper);
      _service->handle__event_MigrateContext(mevent->msg);
      mevent->msg = NULL;
    } else if( globalEvent->globalEventType == GlobalHeadEvent::GlobalHeadEvent_MODIFY_OWNERSHIP ) {
      req_type = GlobalHeadEvent::GlobalHeadEvent_MODIFY_OWNERSHIP;
      executeGlobalModifyOwnershipEventProcess();
    }

    delete globalEvent;
	}

  void HeadEventTP::executeGlobalContextCreateEventProcess() {
    ADD_SELECTORS("HeadEventTP::executeGlobalContextCreateEventProcess");
    HeadCreateContextEvent* e = static_cast<HeadCreateContextEvent*>(globalEvent->helper);
    
    ContextService* _service = static_cast<ContextService*>(globalEvent->cl);

    mace::string ctx_type = Util::extractContextType(e->createContextName);
    macedbg(1) << "Try to create context("<< e->createContextName <<") of type " << ctx_type << Log::endl;
    // mace::ElasticityRule rule = (_service->eConfig).getContextInitialPlacement( ctx_type );
    mace::ElasticityRule rule;
    mace::string pContextName = "";

    const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs =  e->ownershipPairs;
    for( uint32_t i=0; i<ownershipPairs.size(); i++ ) {
      const mace::pair<mace::string, mace::string>& owp = ownershipPairs[i];
      if( owp.second == e->createContextName ) {
        pContextName = owp.first;
        break;
      }
    }

    mace::ContextMapping& contextMapping = _service->getContextMapping();
    std::pair< mace::MaceAddr, uint32_t > newMappingReturn = contextMapping.newMapping(e->createContextName, rule, pContextName);
    const mace::ContextMapping& ctxmapCopy = contextMapping.getLatestContextMapping();
    uint64_t current_version = contextMapping.getCurrentVersion();
    uint32_t contextId = newMappingReturn.second;
        
    macedbg(1) << "Execute context create event to create " << e->createContextName <<" contextId=" << contextId <<" ContextMappingVersion="<< current_version << " version="<< ctxmapCopy.getCurrentVersion() << Log::endl;
      
    mace::map< uint32_t, mace::string> contextSet;
    contextSet[ contextId ] = e->createContextName;

    mace::OrderID globalEventId(0, globalEvent->ticket);
    if(_service->isLocal(newMappingReturn.first ) ) {
      ContextStructure& contextStructure = _service->contextStructure;
      contextStructure.getLock(ContextStructure::WRITER);
      contextStructure.updateOwnerships( e->ownershipPairs, e->vers );
      contextStructure.releaseLock(ContextStructure::WRITER);
      
      mace::ContextBaseClass* newCtxObj = _service->createContextObjectWrapper(globalEventId, e->createContextName, contextId, 
        current_version, false);
      ASSERTMSG(newCtxObj != NULL, "Fail to create context object!");
      _service->notifyContextMappingUpdate(e->createContextName);
      commitGlobalEvent(globalEvent->ticket);
    } else{
      _service->send__event_AllocateContextObjectMsg( globalEventId, ctxmapCopy, newMappingReturn.first, contextSet, 
        mace::Event::ALLOCATE_CTX_OBJECT, current_version, e->ownershipPairs, e->vers );
    }
    
  }

  void HeadEventTP::executeGlobalModifyOwnershipEventProcess() {
    ADD_SELECTORS("HeadEventTP::executeGlobalModifyOwnershipEventProcess");
    // HeadModifyOwnershipEvent* e = static_cast<HeadModifyOwnershipEvent*>(globalEvent->helper);
    
    // ContextService* _service = static_cast<ContextService*>(globalEvent->cl);
    // const mace::ContextMapping& snapshot = _service->getLatestContextMapping();
    
    // ContextStructure& contextStructure = _service->contextStructure;
    
    // contextStructure.getLock(ContextStructure::WRITER);
    // mace::set<mace::string> affected_contexts_set = contextStructure.modifyOwnerships( mace::ContextMapping::GLOBAL_CONTEXT_NAME, e->ownershipOpInfos);
    // contextStructure.releaseLock(ContextStructure::WRITER);

    // macedbg(1) << "AffectedContextNames: " << affected_contexts_set << Log::endl;

    // mace::set<mace::string> exist_affected_contexts_set;
    // for(mace::set<mace::string>::const_iterator c_iter = affected_contexts_set.begin(); c_iter != affected_contexts_set.end(); 
    //     c_iter ++ ) {
    //   if( ContextMapping::hasContext2( snapshot, *c_iter) == 0 ) {
    //     macedbg(1) << "context("<< *c_iter <<") doesn't exist now! Ignore it!" << Log::endl;
    //     continue;
    //   }
    //   exist_affected_contexts_set.insert(*c_iter);
    // }
    // mace::set<mace::string> next_contexts = contextStructure.getContextsClosedToRoot(exist_affected_contexts_set);
    // mace::vector<mace::string> affected_contexts = contextStructure.sortContexts(exist_affected_contexts_set);

    // mace::vector< mace::pair<mace::string, mace::string> > ownershipPairs = contextStructure.getAllOwnerships() ;
    // const uint32_t ownershipVer = contextStructure.getCurrentVersion() ;

    // const mace::ContextMapping& snapshot = _service->getLatestContextMapping();

    // waitingReplyContextNames.clear();
    // mace::map< mace::MaceAddr, mace::set<mace::string> > dom_update_addrs;

    // for( mace::set<mace::string>::iterator iter = next_contexts.begin(); iter != next_contexts.end(); iter ++ ){
    //   const mace::MaceAddr& destAddr = ContextMapping::getNodeByContext(snapshot, *iter );
    //   dom_update_addrs[destAddr].insert(*iter);
    //   waitingReplyContextNames.insert(*iter);
    // }    

    // ScopedLock sl(executingSyncMutex);
    // mace::vector<mace::EventOperationInfo> eops;
    // for(mace::map< mace::MaceAddr, mace::set<mace::string> >::const_iterator addrIter = dom_update_addrs.begin(); 
    //     addrIter != dom_update_addrs.end(); addrIter ++ ){
    //   _service->send__event_updateOwnershipAndDominators( addrIter->first, mace::ContextMapping::GLOBAL_CONTEXT_NAME, addrIter->second, 
    //     affected_contexts, eops, ownershipPairs, ownershipVer);
    // }    
    // pthread_cond_wait( &executingSyncCond, &executingSyncMutex );
    // macedbg(1) << "OwnershipModifyEvent("<< globalEvent->ticket <<") is done!" << Log::endl;
    // sl.unlock();

    // const mace::MaceAddr& replyAddr = ContextMapping::getNodeByContext(snapshot, e->ctxName );
    // _service->send__event_modifyOwnershipReply( replyAddr, e->ctxName, e->eop );
    // commitGlobalEvent(globalEvent->ticket);
  }

  void HeadEventTP::executeEventFinish(){
  }
  void HeadEventTP::commitEventProcess() {
    mace::AgentLock::commitEvent( *committingEvent );
  }

  void HeadEventTP::commitEventFinish() {

    // event committed.
    static bool recordRequestTime = params::get("EVENT_REQUEST_TIME",false);

    mace::Event const& event = *committingEvent;


    if( recordRequestTime || sampleEventLatency ){
      accumulateEventRequestCommitTime( event );
    }
    if( event.getEventType() == mace::Event::ENDEVENT ){
      haltingCommit = true;
    }
    /**
     * TODO: update the event as committed 
     * */
    //delete committingEvent;
    committingEvent->subevents.clear();
    committingEvent->eventMessages.clear();
    committingEvent->eventUpcalls.clear();
    eventObjectPool.put( committingEvent );

  }

  void HeadEventTP::wait() {
    ASSERT(pthread_cond_wait(&signalv, &eventQueueMutex) == 0);
  }
	void HeadEventTP::globalWait() {
		ASSERT(pthread_cond_wait(&signalg, &globalEventQueueMutex) == 0);	
	}
  void HeadEventTP::commitWait() {
    ASSERT(pthread_cond_wait(&signalc, &commitQueueMutex) == 0);
  }
  void HeadEventTP::signalSingle() {
    ADD_SELECTORS("HeadEventTP::signalSingle");
    macedbg(2) << "signal() called - just one thread." << Log::endl;
    pthread_cond_signal(&signalv);
  } // signal
  void HeadEventTP::signalGlobalSingle() {
    ADD_SELECTORS("HeadEventTP::signalGlobalSingle");
    macedbg(2) << "global signal() called - just one thread." << Log::endl;
    pthread_cond_signal(&signalg);
  }
  void HeadEventTP::signalAll() {
    ADD_SELECTORS("HeadEventTP::signalAll");
    macedbg(2) << "signal() called - all head events threads." << Log::endl;
    pthread_cond_broadcast(&signalv);
  } // signal
  void HeadEventTP::signalGlobalAll() {
    ADD_SELECTORS("HeadEventTP::signalGlobalAll");
    macedbg(2) << "signal() called - all global head events threads." << Log::endl;
    pthread_cond_broadcast(&signalg);
  }
  void HeadEventTP::signalCommitThread() {
    ADD_SELECTORS("HeadEventTP::signalCommitThread");
    macedbg(2) << "signal() called - just one thread." << Log::endl;
    pthread_cond_signal(&signalc);
  } // signal

  /*void HeadEventTP::lock() const {
    ASSERT(pthread_mutex_lock(&mace::AgentLock::_agent_ticketbooth) == 0);
  } // lock

  void HeadEventTP::unlock() const {
    ASSERT(pthread_mutex_unlock(&mace::AgentLock::_agent_ticketbooth) == 0);
  } // unlock
  */

  void* HeadEventTP::startThread(void* arg) {
    struct ThreadArg* targ = ((struct ThreadArg*)arg);
    HeadEventTP* t = targ->p;
    t->run( targ->i  );
    return 0;
  }

	void* HeadEventTP::startGlobalThread(void* arg) {
		struct ThreadArg* targ = ((struct ThreadArg*)arg);
		HeadEventTP* t = targ->p;
		t->globalrun(targ->i);
		return 0;
	}
  void* HeadEventTP::startCommitThread(void* arg) {
    HeadEventTP* t = (HeadEventTP*)(arg);
    t->runCommit();
    return 0;
  }
  void HeadEventTP::run(uint32_t n){
    ADD_SELECTORS("HeadEventTP::run");
    ScopedLock sl(eventQueueMutex);
    while( !halted ){
      // wait for the data to be ready
      if( !hasPendingEvents() ){
        if( sleeping[ n ] == false ){
          sleeping[ n ] = true;
          idle ++;
        }
        macedbg(1)<<"sleep"<<Log::endl;
        wait();
        continue;
      }
      if( sleeping[n] == true){
        sleeping[n] = false;
        idle --;
      }

      // pickup the data
      executeEventSetup();
      
			// execute the data
			
			mace::ContextBaseClass* ctxObjPtr = data.getCtxObj();
			ScopedLock ctx_sl(ctxObjPtr->createEventTicketMutex);
      sl.unlock();
      executeEventProcess();
			ctx_sl.unlock();
      //executeEventFinish();

      sl.lock();
    }

    ASSERT(pthread_cond_destroy(&signalv) == 0);
    sl.unlock();

    HeadTransportTPInstance()->haltAndWait();


    pthread_exit(NULL);
  }

	void HeadEventTP::globalrun(uint32_t n){
		ADD_SELECTORS("HeadEventTP::globalrun");
		ScopedLock sl(globalEventQueueMutex);
		while( !halted ){
			if( !hasPendingGlobalEvents() ){
				if( globalSleeping[n] == false ){
					globalSleeping[n] = true;
					globalIdle ++;
				}
        //macedbg(1) << "Global sleeping." << Log::endl;
				globalWait();
        //macedbg(1) << "Global wakeup!" << Log::endl;
				continue;
			}
			if(globalSleeping[n] == true){
				globalSleeping[n] = false;
				globalIdle --;
			}

      macedbg(1) << "Global thread(" << n << ") start to handle global event!" << Log::endl;

			executeGlobalEventSetup();

      sl.unlock();
			executeGlobalEventProcess();

			sl.lock();
		} 
		ASSERT(pthread_cond_destroy(&signalg) == 0 );
		sl.unlock();

		pthread_exit(NULL);
	}
  void HeadEventTP::runCommit(){
    ScopedLock sl(commitQueueMutex);
    while( !haltingCommit ){
      // wait for the data to be ready
      if( !hasUncommittedEvents() ){
        busyCommit = false;
        commitWait();
        continue;
      }
      busyCommit = true;

      // pickup the data
      //commitEventSetup();
      // execute the data
      sl.unlock();
      commitEventProcess();

      commitEventFinish();

      sl.lock();
    }
    ASSERT(pthread_cond_destroy(&signalc) == 0);

  }


  void HeadEventTP::prepareHalt(const mace::OrderID& _exitEventId) {
    ADD_SELECTORS("HeadEventTP::prepareHalt");
    ScopedLock sl(eventQueueMutex);
    halting = true;
    exitEventId = _exitEventId;
    macedbg(1)<<"exit EventId = "<< exitEventId << Log::endl;
    HeadEventTPInstance()->signalAll();
    sl.unlock();

    /*void* status;
    for( uint32_t nThread = 0; nThread < minThreadSize; nThread ++ ){
      int rc = pthread_join( headThread[ nThread ], &status );
      if( rc != 0 ){
        perror("pthread_join");
      }
    }*/

  }
  void HeadEventTP::haltAndWaitHead() {
  // force it to halt no wait
    ScopedLock sl(eventQueueMutex);
    if( halted ) return;
    halted = true;
    HeadEventTPInstance()->signalAll();
    sl.unlock();

    void* status;
    for( uint32_t nThread = 0; nThread < minThreadSize; nThread ++ ){
      int rc = pthread_join( headThread[ nThread ], &status );
      if( rc != 0 ){
        perror("pthread_join");
      }
    }

    //ASSERT(pthread_cond_destroy(&signalv) == 0);
  }

  void HeadEventTP::haltAndWaitGlobalHead() {
  // force it to halt no wait
    ScopedLock sl(globalEventQueueMutex);
    if( halted ) return;
    halted = true;
    HeadEventTPInstance()->signalGlobalAll();
    sl.unlock();

    void* status;
    for( uint32_t nThread = 0; nThread < minGlobalThreadSize; nThread ++ ){
      int rc = pthread_join( globalHeadThread[ nThread ], &status );
      if( rc != 0 ){
        perror("pthread_join");
      }
    }

    //ASSERT(pthread_cond_destroy(&signalv) == 0);
  }
  void HeadEventTP::haltAndWaitCommit() {
    ScopedLock sl2(commitQueueMutex);
    if( haltingCommit ) return;
    HeadEventTPInstance()->signalCommitThread();
    sl2.unlock();

    void* status;
    int rc = pthread_join( headCommitThread, &status );
    if( rc != 0 ){
      perror("pthread_join");
    }

  }
  void HeadEventTP::haltAndNoWaitCommit() {
    ScopedLock sl2(commitQueueMutex);
    if( haltingCommit ) return;
    haltingCommit = true;
    HeadEventTPInstance()->signalCommitThread();
    sl2.unlock();

    void* status;
    int rc = pthread_join( headCommitThread, &status );
    if( rc != 0 ){
      perror("pthread_join");
    }

  }
  void HeadEventTP::executeEvent(eventfunc func, mace::Event::EventRequestType subevents, bool useTicket){
    ADD_SELECTORS("HeadEventTP::executeEvent");

    mace::ContextBaseClass* ctx_obj = ThreadStructure::myContext();
    mace::OrderID eventId = ThreadStructure::newTickets( ctx_obj, subevents.size() );
    // WC: predictive programming
    uint8_t svid = subevents.begin()->sid;
    AsyncEventReceiver* sv = BaseMaceService::getInstance( svid );

    HeadEvent thisev (sv, func, subevents.begin()->request, eventId, ctx_obj->contextName);

    ScopedLock sl(eventQueueMutex);

    if ( halted ) 
      return;

    for( mace::Event::EventRequestType::iterator subeventIt = subevents.begin(); subeventIt != subevents.end(); subeventIt++ ){

      if( subeventIt->sid != svid ){
        svid = subeventIt->sid;
        thisev.cl = BaseMaceService::getInstance( svid );
      }
      thisev.param = subeventIt->request;
      thisev.eventId.ticket = ctx_obj->createTicketNumber++;

      doExecuteEvent(thisev );
    }

    tryWakeup();
  }
  void HeadEventTP::executeEvent(AsyncEventReceiver* sv, eventfunc func, mace::Message* p, bool useTicket){
    ADD_SELECTORS("HeadEventTP::executeEvent");
    mace::ContextBaseClass* ctx_obj = ThreadStructure::myContext();

    ScopedLock sl(ctx_obj->createEventMutex);

    if ( halted ) 
      return;

    mace::OrderID myEventId;
		

    if( !useTicket ){
      myEventId = ThreadStructure::newTicket(ctx_obj);
    }else{
      myEventId = ThreadStructure::myEventID();
    }
    HeadEvent thisev (sv,func,p, myEventId, ctx_obj->contextName);
    doExecuteEvent( thisev );

    tryWakeup();
  }

  // should only be called after lock is acquired
  
  void HeadEventTP::doExecuteEvent(HeadEvent const& thisev){
    
  }
  

  void HeadEventTP::tryWakeup(){
    ADD_SELECTORS("HeadEventTP::tryWakeup");
    HeadEventTP* tp = HeadEventTPInstance();
    if( tp->idle > 0  /*&& tp->hasPendingEvents()*/ ){
      macedbg(1)<<"head thread idle, signal it"<< Log::endl;
      tp->signalSingle();
    }
  }

  void HeadEventTP::tryWakeupGlobal(){
    ADD_SELECTORS("HeadEventTP::tryWakeupGlobal");
    HeadEventTP* tp = HeadEventTPInstance();
    if( tp->globalIdle > 0 ){
      macedbg(1)<<"Global head thread idle, signal it"<< Log::endl;
      tp->signalGlobalSingle();
    }
  }

	void HeadEventTP::executeContextCreateEvent(AsyncEventReceiver* cl, const mace::string& ctxName, const mace::OrderID& eventId, 
      const mace::MaceAddr& src, const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, 
      const mace::map< mace::string, uint64_t>& vers) {

		ADD_SELECTORS("HeadEventTP::executeContextCreateEvent");

    ScopedLock sl(executeGlobalEventMutex);
    macedbg(1) << "Try to create context: " << ctxName << Log::endl;
    ContextService* _service = static_cast<ContextService*>(cl);

    mace::ContextMapping& contextMapping = _service->getContextMapping();
    if(contextMapping.hasContext(ctxName)){
			if(_service->hasNewCreateContext(ctxName)) {
        macedbg(1) << "It has already start context object creation for " << ctxName << Log::endl;
        _service->addNewCreateContext(ctxName, src);
        return;
      } else {
        macedbg(1) << "Context object(" << ctxName << ") is already created!" << Log::endl;
        _service->notifyContextMappingUpdate(ctxName, src);
        return;
      }
		} else {
      if( !_service->addNewCreateContext(ctxName, src) ) {
        return;
      }
      macedbg(1) << "Context object(" << ctxName << ") is not created!" << Log::endl;
    }
    
    HeadCreateContextEvent* helper = new HeadCreateContextEvent(ctxName, eventId, ownershipPairs, vers);
    GlobalHeadEvent* thisgev = new GlobalHeadEvent(GlobalHeadEvent::GlobalHeadEvent_CREATECONTEXT, helper, cl, next_ticket++);
    //sl.unlock();
    enqueueGlobalHeadEvent( thisgev );    
	}

  void HeadEventTP::executeContextMigrationEvent(AsyncEventReceiver* cl, uint8_t const serviceID, mace::string const& contextName, MaceAddr const& destNode, bool const rootOnly) {
    ADD_SELECTORS("HeadEventTP::executeContextMigrationEvent");
    ScopedLock sl(executeGlobalEventMutex);
    const uint64_t ticket = next_ticket ++;
    macedbg(1) << "Migrating context("<< contextName <<") event ticket=" << ticket << " now_serving_ticket = " << now_serving_ticket << Log::endl;
    mace::OrderID eventId(0, ticket);

    mace::__event_MigrateContext *msg = new mace::__event_MigrateContext( eventId, serviceID, contextName, destNode, rootOnly );
  
    HeadContextMigrationEvent* helper = new HeadContextMigrationEvent(msg);

    GlobalHeadEvent* thisgev = new GlobalHeadEvent(GlobalHeadEvent::GlobalHeadEvent_MIGRATION, helper, cl, ticket);
    //sl.unlock();
    enqueueGlobalHeadEvent( thisgev );
  }

  void HeadEventTP::executeModifyOwnershipEvent(AsyncEventReceiver* cl, mace::EventOperationInfo const& eop, mace::string const& ctxName,
      mace::vector<mace::EventOperationInfo> const& ownershipOpInfos) {
    ADD_SELECTORS("HeadEventTP::executeModifyOwnershipEvent");
    ScopedLock sl(executeGlobalEventMutex);
    const uint64_t ticket = next_ticket ++;
      
    HeadModifyOwnershipEvent* helper = new HeadModifyOwnershipEvent(eop, ctxName, ownershipOpInfos);
    GlobalHeadEvent* thisgev = new GlobalHeadEvent(GlobalHeadEvent::GlobalHeadEvent_MODIFY_OWNERSHIP, helper, cl, ticket);
    enqueueGlobalHeadEvent( thisgev );
  }


  void HeadEventTP::accumulateEventLifeTIme(mace::Event const& event){
    ScopedLock sl( startTimeMutex );
    EventRequestTSType::iterator rit = eventStartTime.find(event.eventId);
    ASSERT( rit != eventStartTime.end() );
    uint64_t duration = TimeUtil::timeu() - rit->second ;
    eventStartTime.erase( rit );
    sl.unlock();

    switch( event.eventType ){
      case mace::Event::ASYNCEVENT:
        Accumulator::Instance(Accumulator::ASYNC_EVENT_LIFE_TIME)->accumulate( duration );
        break;
      case mace::Event::MIGRATIONEVENT:
        Accumulator::Instance(Accumulator::MIGRATION_EVENT_LIFE_TIME)->accumulate( duration );
        break;
      default:
        break;
    }

  }
  void sampleLatency( bool flag ){
    ScopedLock sl( samplingMutex );
    sampleEventLatency = flag;
  }
  double getAverageLatency(  ){
    ScopedLock sl( samplingMutex );

    if( accumulatedEvents == 0 ){
      accumulatedLatency = 0;
      return 0.0f;
    }
    double avgLatency = (double)accumulatedLatency / (double)accumulatedEvents;
    accumulatedEvents = 0;
    accumulatedEvents = 0;
    return avgLatency;
  }
  void HeadEventTP::accumulateEventRequestCommitTime(mace::Event const& event){
    ScopedLock sl( requestTimeMutex );
    EventRequestTSType::iterator rit = eventRequestTime.find(event.eventId);
    // chuangw: this is possible for maceInit, maceExit and other application downcalls.
    //ASSERT( rit != eventRequestTime.end() );
    if( rit == eventRequestTime.end() ){
      return;
    }
    uint64_t duration = TimeUtil::timeu() - rit->second ;
    eventRequestTime.erase( rit );
    sl.unlock();

    if( sampleEventLatency ){
      ScopedLock sl2( samplingMutex );
      accumulatedLatency += duration;
      accumulatedEvents ++;
    }

    switch( event.eventType ){
      case mace::Event::ASYNCEVENT:
        Accumulator::Instance(Accumulator::ASYNC_EVENT_REQCOMMIT_TIME)->accumulate( duration );
        break;
      case mace::Event::MIGRATIONEVENT:
        Accumulator::Instance(Accumulator::MIGRATION_EVENT_REQCOMMIT_TIME)->accumulate( duration );
        break;
      default:
        break;
    }

  }
  void HeadEventTP::commitEvent( const mace::Event& event){
    static bool recordLifeTime = params::get("EVENT_LIFE_TIME",false);
    static bool recordCommitCount = params::get("EVENT_READY_COMMIT",true);

    ADD_SELECTORS("HeadEventTP::commitEvents");

    if( recordCommitCount ){
      Accumulator::Instance(Accumulator::EVENT_READY_COMMIT)->accumulate( 1 );

      switch( event.eventType ){
        case mace::Event::ASYNCEVENT:
          Accumulator::Instance(Accumulator::ASYNC_EVENT_COMMIT)->accumulate( 1 );
          break;
        case mace::Event::MIGRATIONEVENT:
          Accumulator::Instance(Accumulator::MIGRATION_EVENT_COMMIT)->accumulate( 1 );
          break;
        default:
          break;
      }
    }
    // WC: copy the event before acquiring the lock. it seems to optimizes a bit.
    //mace::Event *copiedEvent = new mace::Event(event);
    mace::Event *copiedEvent = eventObjectPool.get();
    *copiedEvent = event;
    HeadEventTP* tp = HeadEventTPInstance();

    ScopedLock sl(commitQueueMutex);
    /**
     * TODO: record the event, finished, but uncommitted 
     * */

    macedbg(1)<<"enqueue commit event= "<< event.eventId<<Log::endl;
    headCommitEventQueue.push( copiedEvent  );
    if( recordLifeTime ){
      accumulateEventLifeTIme(event);
    }

    if( !tp->busyCommit /*&& tp->hasUncommittedEvents()*/ ){
      tp->signalCommitThread();
    }
  }

  void HeadEventTP::commitGlobalEvent( const uint64_t ticket ) {
    ADD_SELECTORS("HeadEventTP::commitGlobalEvent");
    ScopedLock sl(executeGlobalEventMutex);
    ASSERTMSG(ticket == now_serving_ticket, "now_serving_ticket is not the expected value");

    collectGlobalEventInfo();
    if( migrating_flag ){
      if( migrating_waiting_flag ) {
        macedbg(1) << "Current migration("<< ticket <<") has not done yet!" << Log::endl;
        ASSERT( waitingCommitEventIds.count(ticket) == 0 );
        waitingCommitEventIds.insert(ticket);
        return;
      }
      macedbg(1) << "Current migration("<< ticket <<") period: " << TimeUtil::timeu() - req_start_time << Log::endl;
      migrating_flag = false;
    }

    now_serving_ticket++;
    macedbg(1) << "Increasing now_serving_ticket to " << now_serving_ticket << Log::endl;
    sl.unlock();

    tryWakeupGlobal();
  }

  void HeadEventTP::collectGlobalEventInfo() {
    ADD_SELECTORS("HeadEventTP::collectGlobalEventInfo");

    uint64_t latency = TimeUtil::timeu() - req_start_time;
    total_latency += latency;
    total_req_count ++;

    if(req_type == GlobalHeadEvent::GlobalHeadEvent_CREATECONTEXT){
      create_req_count ++;
      create_latency += latency;
    } else if( req_type == GlobalHeadEvent::GlobalHeadEvent_MIGRATION) {
      migrating_req_count ++;
      migrating_latency += latency;
    } else if( req_type == GlobalHeadEvent::GlobalHeadEvent_MODIFY_OWNERSHIP ) {
      ownership_req_count ++;
      ownership_latency += latency;
    }

    if( GLOBAL_EVENT_OUTPUT > 0 && total_req_count % GLOBAL_EVENT_OUTPUT == 0 ){
      double avg = total_latency / GLOBAL_EVENT_OUTPUT;

      double create_avg = 0;
      if( create_req_count > 0 ){
        create_avg = create_latency / create_req_count;
      }

      double ownership_avg = 0;
      if( ownership_req_count > 0 ){
        ownership_avg = ownership_latency / ownership_req_count;
      }

      double migrating_avg = 0;
      if( migrating_req_count > 0 ){
        migrating_avg = migrating_latency / migrating_req_count;
      }

      macedbg(1) << "avg=" << avg << " reqCount=" << total_req_count 
                  << " create_avg=" << create_avg << " createReqCount=" << create_req_count 
                  << " migrating_avg=" << migrating_avg << " migratingReqCount=" << migrating_req_count
                  << " ownership_avg=" << ownership_avg << " ownershipReqCount=" << ownership_req_count
                  << Log::endl;

      total_latency = 0;
      create_latency = 0;
      ownership_latency = 0;
      migrating_latency = 0;

      create_req_count = 0;
      ownership_req_count = 0;
      migrating_req_count = 0;
    }
  }

  void HeadEventTP::commitMigrationEvent( const BaseMaceService* sv, mace::OrderID const& eventId, const uint32_t ctxId ) {
    ADD_SELECTORS("HeadEventTP::commitMigrationEvent");
    ScopedLock sl(executeGlobalEventMutex);
    macedbg(1) << "Commit ticket = " << eventId.ticket <<" now_serving_ticket = " << now_serving_ticket << Log::endl;

    if( eventId.ticket != now_serving_ticket ) {
      maceerr << "Commit ticket = " << eventId.ticket <<" now_serving_ticket = " << now_serving_ticket << Log::endl;
    }
    ASSERT( eventId.ticket == now_serving_ticket );

    const ContextService* _service = static_cast<const ContextService*>(sv);
    migratingContexts.erase(ctxId);
    if( migratingContexts.size() == 0 ){
      ContextService* _nc_service = const_cast<ContextService*>(_service);
      _nc_service->setContextMappingUpdateFlag(true);

      const mace::ContextMapping& ctxMapping = _service->getLatestContextMapping();
      _service->send__event_MigrationControlMsg(destAddr, mace::MigrationControl_Message::MIGRATION_DONE, eventId.ticket, migratingContextsCopy, ctxMapping);

      mace::map< mace::MaceAddr, mace::set<mace::string> >::const_iterator iter = migratingContextsOrigAddrs.begin();
      
      for(; iter != migratingContextsOrigAddrs.end(); iter++) {
        const mace::set<mace::string>& ctxs = iter->second;
        mace::map<uint32_t, mace::string> ctxsMap;
        mace::set<mace::string>::const_iterator cIter = ctxs.begin();
        for(; cIter != ctxs.end(); cIter ++) {
          const uint32_t ctxId = mace::ContextMapping::hasContext2(ctxMapping, *cIter);
          ctxsMap[ctxId] = *cIter;
        } 

        _service->send__event_MigrationControlMsg(iter->first, mace::MigrationControl_Message::MIGRATION_DONE, eventId.ticket, ctxsMap, ctxMapping);
      }
      migratingContextsOrigAddrs.clear();
      sl.unlock();
      commitGlobalEvent(eventId.ticket);
    }

  }

  void HeadEventTP::setMigratingContexts( BaseMaceService* sv, mace::OrderID const& eventId, mace::map< uint32_t, mace::string > const& my_migratingContexts, 
      mace::map<mace::MaceAddr, mace::set<mace::string> > const& origAddrs, mace::MaceAddr const& destNode ) {
    ADD_SELECTORS("HeadEventTP::setMigratingContexts");

    macedbg(1) << "Try to set migrating contexts: " << my_migratingContexts << Log::endl;
    ContextService* _service = static_cast<ContextService*>(sv);
    ScopedLock sl(executeGlobalEventMutex);
    if( eventId.ticket != now_serving_ticket ) {
      maceerr << "HeadEventTP::setMigratingContexts: ticket=" << eventId.ticket << " now_serving_ticket=" << now_serving_ticket << Log::endl;
    }
    ASSERT( eventId.ticket == now_serving_ticket );
    migratingContexts = my_migratingContexts;
    migratingContextsCopy = migratingContexts;
    migratingContextsOrigAddrs = origAddrs;
    destAddr = destNode;

    mace::ContextMapping ctxMapping;
    _service->send__event_MigrationControlMsg( destNode, mace::MigrationControl_Message::MIGRATION_PREPARE_RECV_CONTEXTS,
      eventId.ticket, migratingContexts, ctxMapping );

    pthread_cond_wait( &migratingContextCond, &executeGlobalEventMutex);
    macedbg(1) << "Set migrating contexts: " << my_migratingContexts << Log::endl;
  }

  void HeadEventTP::waitingForMigrationContextMappingUpdate( BaseMaceService* sv, mace::map< mace::MaceAddr, mace::set<uint32_t> > const& origContextIdAddrs, mace::MaceAddr const& destNode,
      mace::Event const& event, const uint64_t preVersion, mace::ContextMapping const& ctxMapping, mace::OrderID const& eventId ) {
    
    ADD_SELECTORS("HeadEventTP::waitingForMigrationContextMappingUpdate");
    ASSERT( eventId.ticket == now_serving_ticket );
    macedbg(1) << "Enter now!" << Log::endl;
    ScopedLock sl(executeGlobalEventMutex);
    migrating_waiting_flag = true;
    ContextService* _service = static_cast<ContextService*>(sv);
    mace::map< mace::MaceAddr, mace::set<uint32_t> >::const_iterator addrIter = origContextIdAddrs.begin();
    for(; addrIter != origContextIdAddrs.end(); addrIter++) {
      macedbg(1) << "Inform context to migrate for event=" << eventId.ticket << Log::endl;
      _service->send__event_ContextMigrationRequest( addrIter->first, destNode, event, preVersion, addrIter->second, ctxMapping  );
    }
    macedbg(1) << "Waiting migration event enter contexts("<< origContextIdAddrs <<") ticket=" << eventId.ticket << Log::endl;
    pthread_cond_wait( &migratingContextCond, &executeGlobalEventMutex);
    migrating_waiting_flag = false;

    uint64_t ticket = 0;
    if( waitingCommitEventIds.count(now_serving_ticket) > 0 ){
      macedbg(1) << "To commit migration event=" << now_serving_ticket << Log::endl;
      ticket = now_serving_ticket;
      waitingCommitEventIds.erase(ticket);
    }
    sl.unlock();
    if( ticket > 0 ){
      commitGlobalEvent(ticket);
    }
  }

  void HeadEventTP::signalMigratingContextThread(const uint64_t ticket) {
    ADD_SELECTORS("HeadEventTP::signalMigratingContextThread");
    ScopedLock sl(executeGlobalEventMutex);
    macedbg(1) << "now_serving_ticket=" << now_serving_ticket << " ticket=" << ticket <<Log::endl; 
    if( ticket != now_serving_ticket ){
      maceerr << "now_serving_ticket=" << now_serving_ticket << " ticket=" << ticket << Log::endl;
    }
    ASSERT(ticket == now_serving_ticket);
    pthread_cond_signal(&migratingContextCond);
  } 

  void HeadEventTP::deleteMigrationContext( const mace::string& context_name ) {
    ScopedLock sl(executeGlobalEventMutex);
    // waitingMigrationContexts.erase(context_name);
  }

  void HeadEventTP::handleContextOwnershipUpdateReply( mace::set<mace::string> const& ctxNames ) {
    ADD_SELECTORS("HeadEventTP::handleContextOwnershipUpdateReply");
    ScopedLock sl(executingSyncMutex);
    for( mace::set<mace::string>::const_iterator c_iter = ctxNames.begin(); c_iter != ctxNames.end(); c_iter ++ ) {
      waitingReplyContextNames.erase(*c_iter);
      macedbg(1) << "Recv reply from " << *c_iter << Log::endl;
    }

    if( waitingReplyContextNames.size() == 0 ) {
      pthread_cond_signal( &executingSyncCond );
    }
  }

  void HeadEventTP::enqueueGlobalHeadEvent(GlobalHeadEvent* ge) {
    ADD_SELECTORS("HeadEventTP::enqueueGlobalHeadEvent");
    macedbg(1) << "Put a global head event("<< ge->ticket<<") into the queue!" << Log::endl;
    globalHeadEventQueue.push( ge );
    tryWakeupGlobal();
  }

  HeadEventTP* _inst;
  HeadEventTP* HeadEventTPInstance() {
    //ASSERT( _inst != NULL );
    return _inst;
  }

  // elasticity method
  void* HeadEventTP::startElasticityThread(void* arg) {
    struct ThreadArg* targ = ((struct ThreadArg*)arg);
    HeadEventTP* t = targ->p;
    t->elasticityrun(targ->i);
    return 0;
  }

  void HeadEventTP::elasticityrun(uint32_t n){
    ADD_SELECTORS("HeadEventTP::elasticityrun");
    ScopedLock sl(elasticityEventQueueMutex);
    while( !halted ){
      if( !hasPendingElasticityEvents() ){
        if( elasticitySleeping[n] == false ){
          elasticitySleeping[n] = true;
          elasticityIdle ++;
        }
        //macedbg(1) << "Global sleeping." << Log::endl;
        elasticityWait();
        //macedbg(1) << "Global wakeup!" << Log::endl;
        continue;
      }
      if(elasticitySleeping[n] == true){
        elasticitySleeping[n] = false;
        elasticityIdle --;
      }

      executeElasticityEventSetup();

      sl.unlock();
      executeElasticityEventProcess();

      sl.lock();
    } 
    ASSERT(pthread_cond_destroy(&signale) == 0 );
    sl.unlock();

    pthread_exit(NULL);
  }

  bool HeadEventTP::hasPendingElasticityEvents() {
    ADD_SELECTORS("HeadEventTP::hasPendingElasticityEvents");
    if( elasticityHeadEventQueue.empty() ){
      return false;
    } else {
      return true;
    }
  }

  void HeadEventTP::elasticityWait() {
    ASSERT(pthread_cond_wait(&signale, &elasticityEventQueueMutex) == 0); 
  }

  void HeadEventTP::executeElasticityEventSetup() {
    elasticityEvent = elasticityHeadEventQueue.front();
    elasticityHeadEventQueue.pop();
  }

  void HeadEventTP::executeElasticityEventProcess() {
    ADD_SELECTORS("HeadEventTP::executeElasticityEventProcess");
    if( elasticityEvent->elasticityEventType == ElasticityHeadEvent::ElasticityHeadEvent_CONTEXTS_COLOCATE ) {
      HeadContextsColocateEvent* colocate_event = static_cast<HeadContextsColocateEvent*>(elasticityEvent->helper);
      executeElasticityContextsColocation( elasticityEvent->cl, colocate_event->colocateDestContext, 
        colocate_event->colocateSrcContexts, elasticityEvent->serviceId );
    } else {  
      ASSERT(false);
    }
    
    delete elasticityEvent;
  }

  void HeadEventTP::enqueueElasticityContextsColocationRequest( AsyncEventReceiver* cl, mace::string const& colocate_dest_context, 
      mace::vector<mace::string> const& colocate_src_contexts, const uint8_t service_id) {

    HeadContextsColocateEvent* colocate_event = new HeadContextsColocateEvent( colocate_dest_context, colocate_src_contexts );
    ElasticityHeadEvent* event = new ElasticityHeadEvent( ElasticityHeadEvent::ElasticityHeadEvent_CONTEXTS_COLOCATE, 
        colocate_event, cl, service_id);
    enqueueElasticityHeadEvent( event );
  }

  void HeadEventTP::enqueueElasticityHeadEvent( ElasticityHeadEvent* elevent) {
    ScopedLock sl(elasticityEventQueueMutex);
    elasticityHeadEventQueue.push( elevent );
    tryWakeupElasticity();
  }

  void HeadEventTP::tryWakeupElasticity(){
    HeadEventTP* tp = HeadEventTPInstance();
    if( tp->elasticityIdle > 0 ){
      tp->signalElasticitySingle();
    }
  }

  void HeadEventTP::signalElasticitySingle() {
    pthread_cond_signal(&signale);
  }

  void HeadEventTP::executeElasticityContextsColocation( AsyncEventReceiver* cl, mace::string const& colocate_dest_context, 
      mace::vector<mace::string> const& colocate_src_contexts, const uint8_t service_id) {
    ADD_SELECTORS("HeadEventTP::executeElasticityContextsColocation");

    macedbg(1) << "Try to colocate " << colocate_src_contexts << " with " << colocate_dest_context << Log::endl;
    ScopedLock sl(executeGlobalEventMutex);
    if( waitingMigrationContexts.count(colocate_dest_context) > 0 ){
      return;
    }

    mace::vector<mace::string> migration_contexts;
    ContextService* _service = static_cast<ContextService*>(cl);
    const mace::ContextMapping& snapshot = _service->getContextMapping();
    const mace::MaceAddr& dest_addr = mace::ContextMapping::getNodeByContext( snapshot, colocate_dest_context );
    for( mace::vector<mace::string>::const_iterator iter = colocate_src_contexts.begin(); iter != colocate_src_contexts.end();
        iter ++ ) {
      if( *iter == colocate_dest_context ){
        continue;
      }
      if( mace::ContextMapping::getNodeByContext(snapshot, *iter) != dest_addr && waitingMigrationContexts.count(*iter) == 0 ) {
        migration_contexts.push_back( *iter );
        waitingMigrationContexts.insert( *iter );
      }
    }
    sl.unlock();

    for( uint32_t i=0; i<migration_contexts.size(); i++ ){
      macedbg(1) << "To migrate context("<< migration_contexts[i] <<") to server("<< dest_addr <<")!" << Log::endl;
      HeadEventTP::executeContextMigrationEvent( cl, service_id, migration_contexts[i], dest_addr, true );
    }
  }

  /*************************************************************************************************************/

  void prepareHalt(const mace::OrderID& exitEventId) {
    // TODO: chuangw: need to execute all remaining event requests before halting.
    HeadEventTPInstance()->prepareHalt(exitEventId);

  }
  void haltAndWait() {
    if( HeadEventTPInstance() ) {
      HeadEventTPInstance()->haltAndWaitHead();
      HeadEventTPInstance()->haltAndWaitGlobalHead();
    }

    //delete HeadEventTPInstance();
  }
  void haltAndWaitCommit() {
    // TODO: chuangw: need to execute all remaining event requests before halting.
    if( HeadEventTPInstance() )
      HeadEventTPInstance()->haltAndWaitCommit();
    delete HeadEventTPInstance();
    _inst = NULL;
  }
  void haltAndNoWaitCommit() {
    // TODO: chuangw: need to execute all remaining event requests before halting.
    if( HeadEventTPInstance() )
      HeadEventTPInstance()->haltAndNoWaitCommit();
    delete HeadEventTPInstance();
    _inst = NULL;
  }

  void init() {
    eventRequestTime.clear();
    eventStartTime.clear();
    sampleEventLatency = false;
    accumulatedLatency = 0;
    accumulatedEvents = 0;

    HeadTransportTP::init();

    while( !headEventQueue.empty() ){
      headEventQueue.pop();
    }
    while( !headCommitEventQueue.empty() ){
      headCommitEventQueue.pop();
    }
    while( !globalHeadEventQueue.empty() ) {
      globalHeadEventQueue.pop();
    }
    halting = false;
    halted = false;
    exitEventId.ctxId = 0;
    exitEventId.ticket = 0;
    haltingCommit = false;
    //TODO: initialize these two variables
    /*uint64_t HeadMigration::migrationEventID;
    mace::MaceAddr HeadMigration::newHeadAddr;*/
    //Assumed to be called from main() before stuff happens.
    minThreadSize = params::get<uint32_t>("NUM_HEAD_THREADS", 1);
    maxThreadSize = params::get<uint32_t>("MAX_HEAD_THREADS", 1);

    minGlobalThreadSize = params::get<uint32_t>("NUM_GLOBAL_HEAD_THREADS", 1);
    maxGlobalThreadSize = params::get<uint32_t>("MAX_GLOBAL_HEAD_THREADS", 1);
    _inst = new HeadEventTP(minThreadSize, maxThreadSize, minGlobalThreadSize, maxGlobalThreadSize);

    uint32_t minHeadTransportThreadSize = params::get<uint32_t>("NUM_HEAD_TRANSPORT_THREADS", 3);
    uint32_t maxHeadTransportThreadSize = params::get<uint32_t>("MAX_HEAD_TRANSPORT_THREADS", 3);
    _tinst = new HeadTransportTP(minHeadTransportThreadSize, maxHeadTransportThreadSize);
  }


//////////////////////////////////// HeadTransportTp //////////////////////////
  pthread_mutex_t HeadTransportTP::ht_lock = PTHREAD_MUTEX_INITIALIZER;

  HeadTransportTP::HeadTransportTP(uint32_t minThreadSize, uint32_t maxThreadSize ) :
    tpptr (new ThreadPoolType(*this,&HeadEventDispatch::HeadTransportTP::runDeliverCondition,&HeadEventDispatch::HeadTransportTP::runDeliverProcessUnlocked,&HeadEventDispatch::HeadTransportTP::runDeliverSetup,NULL,ThreadStructure::ASYNC_THREAD_TYPE,minThreadSize, maxThreadSize) )
    //tpptr (new ThreadPoolType(*this,&mace::HeadTransportTP::runDeliverCondition,&mace::HeadTransportTP::runDeliverProcessUnlocked,NULL,NULL,ThreadStructure::ASYNC_THREAD_TYPE,minThreadSize, maxThreadSize) )
    {
    Log::log("HeadTransportTP::constructor") << "Created threadpool for head transport with " << minThreadSize << " threads. Max: "<< maxThreadSize <<"." << Log::endl;
  }
  HeadTransportTP::~HeadTransportTP() {
    haltAndWait();
    ThreadPoolType *tp = tpptr;
    tpptr = NULL;
    delete tp;
  }
  void HeadTransportTP::init()  {
    while( !mqueue.empty() ){
      mqueue.pop();
    }
  }
  void HeadTransportTP::lock()  {
    ASSERT(pthread_mutex_lock(&ht_lock) == 0);
  } // lock

  void HeadTransportTP::unlock()  {
    ASSERT(pthread_mutex_unlock(&ht_lock) == 0);
  } // unlock
  bool HeadTransportTP::runDeliverCondition(ThreadPoolType* tp, uint threadId) {
    ScopedLock sl( ht_lock );
    return !mqueue.empty();
    
  }
  void HeadTransportTP::runDeliverSetup(ThreadPoolType* tp, uint threadId) {
    ScopedLock sl( ht_lock );
    tp->data(threadId) = mqueue.front();
    mqueue.pop();
  }
  void HeadTransportTP::runDeliverProcessUnlocked(ThreadPoolType* tp, uint threadId) {
    tp->data(threadId).fire();
  }
  void HeadTransportTP::runDeliverProcessFinish(ThreadPoolType* tp, uint threadId){
  }
  void HeadTransportTP::sendEvent(InternalMessageSender* sv, mace::MaceAddr const& dest, mace::AsyncEvent_Message* const eventObject, uint64_t instanceUniqueID){
    HeadTransportTP* instance =  HeadTransportTPInstance();
    lock();
    mqueue.push( HeadTransportQueueElement( sv, dest, eventObject, instanceUniqueID ) );
    
    unlock();
    instance->signal();
  }
  void HeadTransportTP::signal() {
    if (tpptr != NULL) {
      tpptr->signalSingle();
    }
  }

  void HeadTransportTP::haltAndWait() {
    ASSERTMSG(tpptr != NULL, "Please submit a bug report describing how this happened.  If you can submit a stack trace that would be preferable.");
    tpptr->halt();
    tpptr->waitForEmptySignal();
  }
}
template<class T> pthread_mutex_t HeadEventDispatch::ObjectPool< T >::lock = PTHREAD_MUTEX_INITIALIZER;
