// Parent class for all context classes
#ifndef CONTEXTBASECLASS_H
#define CONTEXTBASECLASS_H

#include "Serializable.h"
#include "mace-macros.h"
#include "mstring.h"
#include "mset.h"
#include "pthread.h"
#include <queue>
#include "m_map.h"

#include "ThreadStructure.h"
#include "Printable.h"
#include "Event.h"
#include "pthread.h"
#include "mace.h"
#include "HeadEventDispatch.h"
#include "SpecialMessage.h"
#include "SockUtil.h"
#include "ElasticPolicy.h"
#include "eMonitor.h"

#define  MARK_RESERVED NULL
/**
 * \file ContextBaseClass.h
 * \brief declares the base class for context classes
 */
namespace mace {
class ContextEventTP;

typedef std::map< std::pair< mace::OrderID, mace::string >, std::map< mace::string, mace::string > > snapshotStorageType;
class ContextThreadSpecific;
class ContextBaseClass;
class EventCommitQueue;
class __EventStorageStruct__;
class ContextBaseClassParams;

template<class T>
class EventQueue{
private:
  typedef mace::deque<T> QueueType;
public:
  EventQueue() {

  }

  bool empty() {
    return queue.empty();
  }
  T top() const {
    return queue.front();
  }
  T back() const {
    return queue.back();
  }
  void pop() {
    queue.pop_front();
  }
  void push( T event ){
     queue.push_back(event);
  }

  int size() {
    return queue.size();
  }

private:
  uint64_t offset;
  QueueType queue;
};

class ContextEvent: public PrintPrintable, public Serializable {
  public:
    static const uint8_t TYPE_NULL = 0;
    static const uint8_t TYPE_ASYNC_EVENT = 1;
    static const uint8_t TYPE_ROUTINE_EVENT = 2;
    static const uint8_t TYPE_BROADCAST_EVENT = 3;
    static const uint8_t TYPE_GRAP_EVENT = 4;
    static const uint8_t TYPE_START_EVENT = 5;
    static const uint8_t TYPE_COMMIT_CONTEXT = 6;    
    static const uint8_t TYPE_FIRST_EVENT = 7;
    static const uint8_t TYPE_LAST_EVENT = 8;
    static const uint8_t TYPE_BROADCAST_COMMIT = 9;
    static const uint8_t TYPE_BROADCAST_COMMIT_CONTEXT = 10;
    
    OrderID eventId;
    uint64_t executeTicket;
    
    BaseMaceService* sv;
    uint8_t type;
    InternalMessageHelperPtr param;
    mace::MaceAddr source;
    uint8_t sid;

    ContextBaseClass* contextObject;
    
  public:
    ContextEvent() : eventId( ), executeTicket(0), sv(NULL), type( TYPE_NULL ) , param(), source( SockUtil::NULL_MACEADDR ), sid(0), contextObject(NULL)  {}
    ContextEvent(const OrderID& eventId, BaseMaceService* sv, uint8_t const type, InternalMessageHelperPtr param, mace::MaceAddr const& source, const uint8_t sid, ContextBaseClass* ctxObj ) : 
        eventId( eventId ), sv(sv), type( type ), param(param), source(source), sid(sid), contextObject(ctxObj) { }
    
    ContextEvent(const OrderID& eventId, BaseMaceService* sv, uint8_t const type, InternalMessageHelperPtr param, const uint8_t sid, ContextBaseClass* ctxObj) : eventId(eventId),  
        sv(sv), type( type ), param(param), sid(sid), contextObject(ctxObj) {}
    
    ~ContextEvent() { 
      sv = NULL;
      contextObject = NULL; 
      param = NULL;
    }
    void fire();

    void setContextObject( ContextBaseClass* ctxObj ) { contextObject = ctxObj; }

    ContextEvent& operator=(const ContextEvent& orig){
        if( this == &orig ) {
          return *this;
        }
        this->eventId = orig.eventId;
        this->executeTicket = orig.executeTicket;
        this->sv = orig.sv;
        this->type = orig.type;
        this->param = orig.param;
        this->source = orig.source;
        this->sid = orig.sid;
        this->contextObject = orig.contextObject;
        return *this;
    }

    virtual void serialize(std::string& str) const;
    virtual int deserialize(std::istream & is) throw (mace::SerializationException);
    int deserializeEvent( std::istream& in );

    void print(std::ostream& out) const;
    void printNode(PrintNode& pr, const std::string& name) const;
};

class ContextCommitEvent: public PrintPrintable, public Serializable {
public:
  BaseMaceService* sv;
  OrderID eventId;
  ContextBaseClass* contextObject;
  bool isAsyncEvent;

  ContextCommitEvent(): sv(NULL), eventId(), contextObject(NULL), isAsyncEvent(false) {}
  ContextCommitEvent(BaseMaceService* sv, OrderID const& eventId, mace::ContextBaseClass* contextObject, const bool isAsyncEvent): sv(sv), eventId(eventId), 
    contextObject(contextObject), isAsyncEvent(isAsyncEvent) { }

  ~ContextCommitEvent() {
    sv = NULL;
    contextObject = NULL;
  }

  void setBaseMaceService(BaseMaceService* sv) {
    this->sv = sv;
  }

  ContextCommitEvent& operator=(const ContextCommitEvent& orig){
    ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
    this->sv = orig.sv;
    this->eventId = orig.eventId;
    this->contextObject = orig.contextObject;
    this->isAsyncEvent = orig.isAsyncEvent;
    return *this;
  }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const;
  void printNode(PrintNode& pr, const std::string& name) const;
};


class ContextThreadSpecific{
public:
    ContextThreadSpecific():
        threadCond(),
        currentMode(-1),
        myTicketNum(std::numeric_limits<uint64_t>::max()),
        snapshotVersion(0)
    {
        pthread_cond_init(  &threadCond, NULL );
    }

    ~ContextThreadSpecific(){
        pthread_cond_destroy( &threadCond );
    }

    int getCurrentMode() { return currentMode; }
    void setCurrentMode(int newMode) { currentMode = newMode; }
    const uint64_t& getSnapshotVersion() { return snapshotVersion; }
    void setSnapshotVersion(const uint64_t& ver) { snapshotVersion = ver; }
public:
    pthread_cond_t threadCond;
    int currentMode; 
    uint64_t myTicketNum;
    uint64_t snapshotVersion;
};
/**
 * ContextBaseClass defines the base for context classes
 *
 * */
template<typename T>
struct QueueComp {
  bool operator() (const T& p1, const T& p2) {
    return p1.eventId > p2.eventId;
  }
};

class LockRequest: public Serializable, public PrintPrintable {
public:
  typedef mace::vector< mace::pair<EventOperationInfo, bool> > EventOperationInfosType;

public:
  static const uint16_t INVALID = 0;
  static const uint16_t WLOCK = 1;
  static const uint16_t RLOCK = 2;
  static const uint16_t DLOCK = 6;
  static const uint16_t VWLOCK = 3;
  static const uint16_t VRLOCK = 4;
  static const uint16_t UNLOCK = 5;

public:
  uint16_t lockType;
  mace::string lockedContextName;
  mace::OrderID eventId;
  bool hasNotified;

private:
  mace::vector<mace::EventOperationInfo> eventOpInfos;

public:
  LockRequest(): lockType(LockRequest::INVALID), lockedContextName(), eventId(), hasNotified(false), eventOpInfos() { }
  LockRequest( const uint16_t& lockType, mace::string const& lockedContextName, mace::OrderID const& eventId ):
    lockType(lockType), lockedContextName(lockedContextName), eventId(eventId), hasNotified(false), eventOpInfos() { }
  ~LockRequest() { 
    eventOpInfos.clear();
  }


  LockRequest& operator=(const mace::LockRequest& orig){
    ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
    this->lockType            = orig.lockType;
    this->eventId             = orig.eventId;
    this->lockedContextName   = orig.lockedContextName;
    this->eventOpInfos        = orig.eventOpInfos;
    this->hasNotified         = orig.hasNotified;
    return *this;
  }

  bool enqueueEventOperation( mace::EventOperationInfo const& eventOpInfo );
  uint32_t getOpNumber() { return eventOpInfos.size(); }
  bool unlock( mace::EventOperationInfo const& op );
  void clearOps() {  eventOpInfos.clear(); }
  mace::vector<mace::EventOperationInfo> getEventOps() const { return eventOpInfos; }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const {
    out<< "LockRequest(";
    uint16_t print_lockType = (uint16_t) lockType;
    out<< "lockType = "; mace::printItem(out, &(print_lockType)); out<< ", ";
    out<< "hasNotified = "; mace::printItem(out, &(hasNotified)); out<< ", ";
    out<< "lockedContextName = "; mace::printItem(out, &(lockedContextName)); out<< ", ";
    out<< "eventId = "; mace::printItem(out, &(eventId)); out<< ", ";
    out<< "eventOpInfos = "; mace::printItem(out, &(eventOpInfos));
    out<< ")";
  }

  void printNode(PrintNode& pr, const std::string& name) const { }
};

class DomLockRequest: public Serializable, public PrintPrintable {
public:
  uint16_t lockType;
  mace::OrderID eventId;
  mace::vector<mace::EventOperationInfo> lockOps;
  mace::set<mace::string> lockedContexts;
  bool hasNotified;

public:
  DomLockRequest(): lockType(LockRequest::INVALID), hasNotified(false) { }
  DomLockRequest( const uint16_t& lockType, mace::OrderID const& eventId ): lockType(lockType), eventId(eventId), hasNotified(false) { }
  ~DomLockRequest() { }


  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const {
    out<< "DomLockRequest(";
    uint16_t print_lockType = (uint16_t) lockType;
    out<< "lockType = "; mace::printItem(out, &(print_lockType)); out<< ", ";
    out<< "hasNotified = "; mace::printItem(out, &(hasNotified)); out<< ", ";
    out<< "lockedContexts = "; mace::printItem(out, &(lockedContexts)); out<< ", ";
    out<< "eventId = "; mace::printItem(out, &(eventId)); out<< ", ";
    out<< "lockOps = "; mace::printItem(out, &(lockOps)); 
    out<< ")";
  }

  void printNode(PrintNode& pr, const std::string& name) const { }

public:
  void addEventOp( const EventOperationInfo& op );
  void addLockedContexts( const mace::vector<mace::string>& locked_contexts );
  void addLockedContext( const mace::string& locked_context );
  bool toRemove() const;
  mace::vector<mace::string> releaseContext( const mace::string& dominator, const mace::string& srcContext,
    const mace::string& locked_context, const ContextStructure& contextStructure);
  bool lockingContext( const mace::string& ctx_name ) const;

  bool unlock( const mace::string& dominator, const mace::EventOperationInfo& eop, const ContextStructure& contextStructure, 
    mace::vector<mace::string>& releaseContexts );

  void removeNotLockedContext( const mace::set<mace::string>& dominatedContexts );
};

class DominatorRequest: public Serializable, public PrintPrintable {
public:
  static const uint16_t INVALID = 0;
  static const uint16_t CHECK_PERMISSION = 1;
  static const uint16_t UNLOCK_CONTEXT = 2;
  static const uint16_t RELEASE_CONTEXT = 3;

public:
  uint16_t lockType;
  mace::string ctxName;
  mace::OrderID eventId;
  mace::EventOperationInfo eventOpInfo;
  
public:
  DominatorRequest(): lockType(DominatorRequest::INVALID), ctxName(), eventId(), eventOpInfo() { }
  // check permission
  DominatorRequest( const uint16_t& lockType, mace::string const& ctxName, mace::EventOperationInfo const& eventOpInfo ):
    lockType(lockType), ctxName(ctxName), eventId(eventOpInfo.eventId), eventOpInfo( eventOpInfo ) { }
  // release
  DominatorRequest( const uint16_t& lockType, mace::string const& ctxName, mace::OrderID const& eventId ):
    lockType(lockType), ctxName(ctxName), eventId(eventId), eventOpInfo() { }
  ~DominatorRequest() { 
    
  }


  DominatorRequest& operator=(const mace::DominatorRequest& orig){
    ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
    this->lockType    = orig.lockType;
    this->eventId     = orig.eventId;
    this->ctxName     = orig.ctxName;
    this->eventOpInfo = orig.eventOpInfo;
    return *this;
  }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const {
    out<< "LockRequest(";
    uint16_t t = (uint16_t)lockType;
    out<< "lockType = "; mace::printItem(out, &( t ) ); out<< ", ";
    out<< "ctxName = "; mace::printItem(out, &(ctxName)); out<< ", ";
    out<< "eventId = "; mace::printItem(out, &(eventId)); out<< ", ";
    out<< "eventOpInfo = "; mace::printItem(out, &(eventOpInfo));
    out<< ")";
  }

  void printNode(PrintNode& pr, const std::string& name) const { }
};


class ContextEventDominator: public Serializable, public PrintPrintable {
public:
  typedef mace::map< mace::string, mace::vector<LockRequest> > EventLockQueue;
public:
  ContextEventDominator(): contextName(), dominateContexts(), eventOrderQueue() { ASSERT( pthread_mutex_init( &dominatorMutex, NULL) == 0 ); }
  ~ContextEventDominator() { pthread_mutex_destroy( &dominatorMutex ); }

  void initialize( const mace::string& contextName, const mace::string& domCtx, const uint64_t& ver,
   const mace::vector<mace::string>& dominateCtxs);

  mace::set<mace::string> checkEventExecutePermission(BaseMaceService* sv, mace::EventOperationInfo const& eventOpInfo);
  void enqueueEventOrderQueue( ContextStructure& ctxStructure, const mace::EventOperationInfo& eventOpInfo );
  mace::set<mace::string> getEventPermitContexts( const mace::OrderID& eventId );
  void printWaitingEventID( const mace::EventOperationInfo& eop ) const;
  
  mace::set<mace::OrderID> getCanLockEventIDs() const;
  bool checkContextInclusion( mace::string const& ctxName );
  void labelRequestNotified( const mace::OrderID& eventId );

  bool unlockContext( ContextStructure& contextStructure, mace::EventOperationInfo const& eventOpInfo, 
    mace::map<mace::string, mace::vector<mace::EventOperationInfo> >& permittedEventOps, 
    mace::map< mace::OrderID, mace::vector<mace::string> >& permittedContextNames, mace::vector<mace::string>& releaseContexts );

  bool unlockContextWithoutLock( ContextStructure& contextStructure, mace::EventOperationInfo const& eventOpInfo, 
    mace::map<mace::string, mace::vector<mace::EventOperationInfo> >& permittedEventOps, 
    mace::map< mace::OrderID, mace::vector<mace::string> >& permittedContextNames, mace::vector<mace::string>& releaseContexts );

  void unlockContextByWaitingRequests( ContextStructure& contextStructure, const mace::OrderID& eventId, 
    mace::map<mace::string, mace::vector<mace::EventOperationInfo> >& permittedEventOps, 
    mace::map< mace::OrderID, mace::vector<mace::string> >& permittedContextNames, mace::vector<mace::string>& releaseContexts );

  void addWaitingUnlockRequest( const mace::EventOperationInfo& eop );

  bool releaseContext( ContextStructure& contextStructure, mace::OrderID const& eventId, 
    mace::string const& lockedContextName, mace::string const& srcContext,
    mace::map<mace::string, mace::vector<mace::EventOperationInfo> >& permittedEventOps, 
    mace::map< mace::OrderID, mace::vector<mace::string> >& permittedContextNames,
    mace::vector<mace::string>& releaseContexts );

  mace::vector<DominatorRequest> checkWaitingDominatorRequests( mace::OrderID const& eventId );
  uint32_t getLockedContextsNumber( mace::OrderID const& eventId );

  void checkEventOrderQueue( ContextStructure& ctxStructure, const mace::OrderID& eventId, const uint16_t& lock_type, 
    const mace::set<mace::string>& locked_contexts, mace::map<mace::string, mace::vector<mace::EventOperationInfo> >& permittedEventOps,
    mace::map< mace::OrderID, mace::vector<mace::string> >& permittedContextNames, mace::vector< mace::string >& releaseContexts );

  void addLockedContexts( const mace::OrderID& eventId, const mace::vector<mace::string>& locked_contexts );
  void addLockedContext( const mace::OrderID& eventId, const mace::string& ctx_name );

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const {
    out<< "ContextEventDominator(";
    out<< "contextName = "; mace::printItem(out, &(contextName)); out<< ", ";
    out<< "dominateContexts = "; mace::printItem(out, &(dominateContexts)); out<< ", ";
    out<< "eventOrderQueue = "; mace::printItem(out, &(eventOrderQueue));
    out<< ")";
  }

  void printNode(PrintNode& pr, const std::string& name) const { }

  ContextEventDominator& operator=(const mace::ContextEventDominator& orig){
    ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
    this->contextName         = orig.contextName;
    this->preDominator        = orig.preDominator;
    this->curDominator        = orig.curDominator;
    this->version             = orig.version;
    this->dominateContexts    = orig.dominateContexts;
    this->eventOrderQueue     = orig.eventOrderQueue;
    this->domLockRequestQueue = orig.domLockRequestQueue;
    return *this;
  }

  // dominator update
  bool updateDominator( ContextStructure& ctxStructure, mace::vector<mace::EventOperationInfo>& feops );
  // bool updateEventQueue(BaseMaceService* sv, mace::string const& src_contextName, mace::ContextEventDominator::EventLockQueue const& lockQueues,
  //   mace::vector<DominatorRequest>& dominatorRequests );
  // void clearWaitingDominatorRequests() { waitingDominatorRequests.clear(); }
  void addUpdateSourceContext( const mace::string& ctx_name );
  void addUpdateWaitingContext( const mace::string& ctx_name );
  void setWaitingUnlockRequests( const mace::vector<mace::EventOperationInfo>& eops );
  void removeUpdateWaitingContext( const mace::string& ctx_name ) { ScopedLock sl(dominatorMutex); updateWaitingContexts.erase(ctx_name); }
  bool isUpdateWaitingForReply();
  void clearUpdateSourceContexts();
  mace::set<mace::string> getUpdateSourceContexts() { ScopedLock sl(dominatorMutex); return updateSourceContexts; }

  mace::string getPreDominator() const { return preDominator; }
  mace::string getCurDominator() const { return curDominator; }
  void addReplyEventOp( const mace::EventOperationInfo& reply_eop );
  void addReplyEventOps( const mace::vector<mace::EventOperationInfo>& reply_eops );
  void clearUpdateReplyEventOps() { ScopedLock sl(dominatorMutex); updateReplyEventOps.clear(); }
  mace::vector<mace::EventOperationInfo> getUpdateReplyEventOps() { ScopedLock sl(dominatorMutex); return updateReplyEventOps; }

private:
  pthread_mutex_t dominatorMutex;
  mace::string contextName;
  
  // dominator update
  mace::string preDominator;
  mace::string curDominator;
  uint64_t version;
  mace::set<mace::string> updateWaitingContexts;
  mace::set<mace::string> updateSourceContexts;
  mace::vector<mace::EventOperationInfo> updateReplyEventOps;
  
  // events lock order
  mace::vector<mace::string> dominateContexts;
  EventLockQueue eventOrderQueue; 
  mace::vector<DomLockRequest> domLockRequestQueue;
  mace::vector<mace::EventOperationInfo> waitingUnlockRequests;
  
private:
  bool checkContextInclusionNoLock( mace::string const& ctxName ) const;
  // bool checkAndUpdateEventQueues(ContextStructure& contextStructure);
  // void adjustEventQueue();
  void printEventQueues();
};

class ContextEventOrder: public Serializable, public PrintPrintable {
private:
  pthread_mutex_t eventOrderMutex;
  mace::string contextName;
  
  //Record <execute ticket, execute event> map
  mace::map<uint64_t, OrderID> executeTicketEventMap;
  //Record <execute event. execute ticket> map
  mace::map<OrderID, uint64_t> executeEventTicketMap;
  uint64_t executeTicketNumber;

public:
  ContextEventOrder(): executeTicketNumber(1) {
    ADD_SELECTORS("ContextEventOrder::constructor");
    ASSERT( pthread_mutex_init( &eventOrderMutex, NULL) == 0 );
    macedbg(1) << "Initialize eventOrderMutex = " << &eventOrderMutex << Log::endl; 
  }

  ~ContextEventOrder() {
    ADD_SELECTORS("ContextEventOrder::destructor");
    macedbg(1) << "Destroy ContextEventOrder=" << this << Log::endl;
    pthread_mutex_destroy( &eventOrderMutex );
  }

  void initialize() {
    ADD_SELECTORS("ContextEventOrder::initialize");
    ASSERT( pthread_mutex_init( &eventOrderMutex, NULL) == 0 );
    macedbg(1) << "Initialize eventOrderMutex = " << &eventOrderMutex << Log::endl;
  }

  ContextEventOrder& operator=(const mace::ContextEventOrder& orig){
    ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
    this->contextName                   = orig.contextName;
    this->executeTicketEventMap         = orig.executeTicketEventMap;
    this->executeEventTicketMap         = orig.executeEventTicketMap;
    this->executeTicketNumber           = orig.executeTicketNumber;
    return *this;
  }

  void setContextName(const mace::string& ctxName) { contextName = ctxName; }
  
  void clear();

  uint64_t addExecuteEvent(mace::OrderID const& eventId);
  mace::vector<OrderID> commitEvent(OrderID const& eventId);
  void removeEventExecuteTicket(OrderID const& eventId);
  OrderID getExecuteEventOrderID( const uint64_t ticket );
  uint64_t getExecuteEventTicket( OrderID const& eventId );

  uint64_t getExecuteTicketNumber() const { return executeTicketNumber; }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const {
    out<< "ContextEventOrder(";
    out<< "contextName = "; mace::printItem(out, &(contextName)); out<< ", ";
    out<< "executeTicketEventMap = "; mace::printItem(out, &(executeTicketEventMap)); out<< ", ";
    out<< "executeEventTicketMap = "; mace::printItem(out, &(executeEventTicketMap)); out<<", ";
    out<< "executeTicketNumber = "; mace::printItem(out, &(executeTicketNumber));
    out<< ")";
  }

  void printNode(PrintNode& pr, const std::string& name) const { }
};

class ContextBaseClass: public Serializable, public PrintPrintable{
public:
    typedef mace::hash_map<ContextBaseClass*, ContextThreadSpecific*, SoftState> ThreadSpecificMapType;
    typedef HeadEventDispatch::HeadEvent RQType;
    //typedef std::priority_queue<RQType, std::vector<RQType>, QueueComp<HeadEventDispatch::HeadEvent> > EventRequestQueueType;
    typedef std::queue<RQType> EventRequestQueueType;

    struct QueueComp {
      bool operator()( const ContextEvent& e1, const ContextEvent& e2) {
        return e1.executeTicket > e2.executeTicket;
      }
    };
    typedef std::priority_queue< ContextEvent, std::vector<ContextEvent>, QueueComp > ContextEventQueueType;
    

private:
friend class ContextThreadSpecific;
friend class ContextLock;
friend class ContextEventTP;
public:
    typedef std::pair<uint64_t, pthread_cond_t*> QueueItemType;
    static const uint8_t HEAD = 0;
    static const uint8_t CONTEXT = 1;

    static pthread_once_t global_keyOnce;
    static pthread_mutex_t eventCommitMutex;
    static pthread_mutex_t eventSnapshotMutex;
    static std::map< mace::OrderID, pthread_cond_t* > eventCommitConds;
    static std::map< mace::OrderID, pthread_cond_t* > eventSnapshotConds;
    static snapshotStorageType eventSnapshotStorage;

    mace::string contextName; ///< The canonical name of the context
    mace::string contextTypeName;
    const int contextType;
    uint8_t serviceId; ///< The service in which the context belongs to
    uint32_t contextId; ///< The numerical ID of the context
   
		//bsang
    pthread_mutex_t createEventTicketMutex;
    pthread_mutex_t executeEventTicketMutex;
    
    pthread_mutex_t executeEventMutex;
    pthread_mutex_t createEventMutex;
    pthread_mutex_t commitEventMutex;
    
    pthread_mutex_t contextMigratingMutex;
    pthread_cond_t contextMigratingCond;

    pthread_mutex_t timeMutex;
    pthread_mutex_t executeTimeMutex;

    pthread_mutex_t eventExecutingSyncMutex;
    std::map< mace::EventOperationInfo, pthread_cond_t* > eventExecutingSyncConds;

    pthread_mutex_t contextEventOrderMutex;
    pthread_mutex_t releaseLockMutex;
    pthread_mutex_t contextDominatorMutex;
    
		EventRequestQueueType createEventQueue;
				
    typedef mace::map<OrderID, mace::vector<__EventStorageStruct__> > EventStorageType;
    EventStorageType waitingEvents;

    typedef std::map<uint64_t, mace::ContextCommitEvent*> CommitEventQueueType;
    CommitEventQueueType executeCommitEventQueue;
    
    mace::deque< mace::ContextEvent > executeEventQueue;

    uint64_t create_now_committing_ticket;
    OrderID execute_now_committing_eventId;
    uint64_t execute_now_committing_ticket;

    OrderID now_serving_eventId;
    uint64_t now_serving_execute_ticket;
    bool execute_serving_flag;

    uint64_t lastWrite;

    ContextEventOrder contextEventOrder;
    ContextEventDominator dominator;

    // Elasticity variables
    mace::ContextRuntimeInfo runtimeInfo;
    mace::ElasticityConfiguration eConfig;

    uint64_t createTicketNumber;
    uint64_t executeTicketNumber;

    mace::map< uint64_t, bool > executedEventsCommitFlag;

    uint64_t now_serving_create_ticket;
    bool createWaitingFlag;

    std::map<uint64_t, pthread_cond_t*> createWaitingThread;
    uint64_t handlingMessageNumber;
    bool isWaitingForHandlingMessages;
    uint64_t handlingCreateEventNumber;
    bool isWaitingForHandlingCreateEvents;

    uint64_t now_max_execute_ticket;

    mace::map< mace::OrderID, EventExecutionInfo > eventExecutionInfos;

		mace::set<mace::OrderID> readerEvents;
    mace::set<mace::OrderID> writerEvents;

    mace::EventOperationInfo currOwnershipEventOp;

    //Migration
    bool migrationEventWaiting;
    mace::set<uint64_t> skipCreateTickets;
    
    //Debug information
    mace::map<mace::OrderID, uint64_t> eventCreateStartTime;
    uint64_t committedEventCount;
    uint64_t totalEventTime;

    mace::map<mace::OrderID, uint64_t> eventExecuteStartTime;
    uint64_t finishedEventCount;
    uint64_t totalEventExecuteTime;

    mace::vector<mace::OrderID> deletedEventIds;
    mace::vector<mace::OrderID> releaseLockEventIds;
    mace::vector<mace::OrderID> committedEventIds;

public:
    /**
     * constructor
     *
     * */
    ContextBaseClass(const mace::string& contextName="(unnamed)", const uint64_t createTicketNumber = 0, const uint64_t executeTicketNumber = 0, const uint64_t create_now_committting_ticket = 0, 
      const mace::OrderID& execute_now_committing_eventId = mace::OrderID(), const uint64_t execute_now_committing_ticket = 0, const mace::OrderID& now_serving_eventId = mace::OrderID(),
      const uint64_t now_serving_execute_ticket = 0, const bool execute_serving_flag = false, const uint64_t lastWrite = 0, const uint8_t serviceId = 0, const uint32_t contextId = 0, const uint8_t contextType = CONTEXT);
    /**
     * destructor
     * */
    virtual ~ContextBaseClass();
    virtual void print(std::ostream& out) const;
    virtual void printNode(PrintNode& pr, const std::string& name) const;
    virtual void serialize(std::string& str) const{ }
    virtual int deserialize(std::istream & is) throw (mace::SerializationException){
        return 0;
    }

		void enqueueCommitEventQueue(BaseMaceService* sv, const mace::OrderID& eventId, const bool isAsyncEvent);
    void commitEvent(const mace::OrderID& eventId);
    
    void enqueueContextEvent(BaseMaceService* sv, const OrderID& eventId);
    void enqueueContextEvent(const BaseMaceService* sv, mace::Event& event);
        
    void createEvent(BaseMaceService* sv, mace::OrderID& myEventId, mace::Event& event, const mace::string& targetContextName, 
      const mace::string& methodType, const int8_t eventType, const uint8_t& eventOpType);
    bool waitingForExecution(const BaseMaceService* sv, mace::Event& event);
    void putBackEventObject(mace::Event* event);
    uint64_t requireExecuteTicket( mace::OrderID const& eventId);

    void prepareHalt();
    void resumeExecution();

    void enqueueSubEvents(mace::OrderID const& eventId);
    void enqueueDeferredMessages(mace::OrderID const& eventId);

    void markExecuteEventCommitted(const mace::OrderID& eventId);

    void sendCommitDoneMsg(const mace::Event& event);

    void signalCreateThread();

    uint64_t getContextMappingVersion() const;
    uint64_t getContextStructureVersion() const;

    void applyOwnershipOperations( const BaseMaceService* sv, mace::EventOperationInfo const& eop, 
      mace::vector<mace::EventOperationInfo> const& ownershipOpInfos );
    
    void handleOwnershipOperationsReply( mace::EventOperationInfo const& eop );

    void handleOwnershipOperations( BaseMaceService* sv, const mace::EventOperationInfo& eop, const mace::string& src_ctxName, 
      const mace::vector<mace::EventOperationInfo>& ownershipOpInfos );

    void handleContextDAGReply( BaseMaceService* sv, const mace::set<mace::string>& contexts, 
      const mace::vector<mace::pair<mace::string, mace::string > >& ownershipPairs, const mace::map<mace::string, uint64_t>& vers );

    void executeOwnershipOperations( BaseMaceService* sv, const mace::EventOperationInfo& eop, const mace::string& src_ctxName, 
      const mace::vector<mace::EventOperationInfo>& ownershipOpInfos );

    mace::EventOperationInfo getNewContextOwnershipOp( mace::OrderID const& eventId, mace::string const& parentContextName,
      mace::string const& childContextName );

    void addOwnershipOperations( const BaseMaceService* sv, mace::EventOperationInfo const& eop, 
      mace::vector<mace::EventOperationInfo> const& ownershipOpInfos );

    void insertEventCreateStartTime( const mace::OrderID& eventId) {
      ScopedLock sl(timeMutex);
      eventCreateStartTime[eventId] = TimeUtil::timeu();
    }

    void insertEventCreateEndTime( const mace::OrderID& eventId ) {
      ADD_SELECTORS("ContextBaseClass::insertEventCreateEndTime");
      ScopedLock sl(timeMutex);
      mace::map< mace::OrderID, uint64_t>::const_iterator cIter = eventCreateStartTime.find(eventId);
      if( cIter != eventCreateStartTime.end() ) {
        committedEventCount ++;
        uint64_t end_time = TimeUtil::timeu();
        totalEventTime += end_time - cIter->second;
        
        if( committedEventCount % 1000 == 0 ) {
          uint64_t avg = (uint64_t)( totalEventTime / 1000 );
          totalEventTime = 0;
          macedbg(1) << "Context("<< contextName <<") avg_event_time=" << avg << " committed_event_count=" << committedEventCount << Log::endl;
        }

        eventCreateStartTime.erase(cIter);
      }
    }

    void insertEventExecuteStartTime( const mace::OrderID& eventId) {
      ScopedLock sl(executeTimeMutex);
      eventExecuteStartTime[eventId] = TimeUtil::timeu();
    }

    void insertEventExecuteEndTime( const mace::OrderID& eventId ) {
      ADD_SELECTORS("ContextBaseClass::insertEventExecuteEndTime");
      ScopedLock sl(executeTimeMutex);
      mace::map< mace::OrderID, uint64_t>::const_iterator cIter = eventExecuteStartTime.find(eventId);
      if( cIter != eventExecuteStartTime.end() ) {
        finishedEventCount ++;
        uint64_t end_time = TimeUtil::timeu();
        totalEventExecuteTime += end_time - cIter->second;
        if( finishedEventCount % 1000 == 0 ) {
          uint64_t avg = (uint64_t)( totalEventExecuteTime / 1000 );
          totalEventExecuteTime = 0;
          macedbg(1) << "Context("<< contextName <<") avg_event_execute_time=" << avg << " finished_event_count=" << finishedEventCount << Log::endl;
        }

        eventExecuteStartTime.erase(cIter);
      }
    }

    uint64_t getExecuteEventTicket( const OrderID& eventId ) {
      return contextEventOrder.getExecuteEventTicket(eventId);
    }

		bool setFinishedCtxEventId(const uint32_t ctxId, const uint64_t eventId) {
			return true;
		}

    mace::Event* getEventObject() {
      mace::Event* eventPtr = new Event();
      return eventPtr;
    }

    void tryCreateWakeup();

    /**
     * returns the canonical name of the context
     * */
    mace::string const& getName() const{
      return contextName;
    }

    void setContextName(const mace::string& ctxName) {
      contextName = ctxName;
    }

    mace::string const& getTypeName() const {
      return contextTypeName;
    }
    /**
     * returns the numerical ID of the context
     * */
    uint32_t getID() const{
      return contextId;
    }

    /** 
     * create a read only snapshot of the context
     * public interface of snapshot() 
     *
     * @param ver snapshot version
     * */
    void snapshot(const uint64_t& ver) const{
        ContextBaseClass* _ctx = new ContextBaseClass(*this);
        snapshot( ver, _ctx );
    }

    void snapshot( std::string& str ) const;

    void resumeParams(BaseMaceService* sv, const ContextBaseClassParams * params);
    /**
     * Each context is responsible for releasing its own snapshot
     * 
     * \param ver Ticket number of the snapshot 
     * */
    virtual void snapshotRelease(const uint64_t& ver) const{ }
    /**
     * Each context takes its own snapshot
     * 
     * \param ver Ticket number of the snapshot 
     * */
    void snapshot(const uint64_t& ver, ContextBaseClass* _ctx) const{
      ADD_SELECTORS("ContextBaseClass::snapshot");
      macedbg(1) << "Snapshotting version " << ver << " for this " << this << " value " << _ctx << Log::endl;
      ASSERT( versionMap.empty() || versionMap.back().first < ver );
      versionMap.push_back( std::make_pair(ver, _ctx) );
    }
    /**
     * return a snapshot of the current event version
     * */
    virtual const ContextBaseClass& getSnapshot() const{
      return *this;
    }
    /**
     * insert a read-only snapshot into the context
     *
     * @param ver version number
     * @param snapshot snapshot object
     * */
    virtual void setSnapshot(const uint64_t ver, const mace::string& snapshot){
        std::istringstream in(snapshot);
        mace::ContextBaseClass *obj = new mace::ContextBaseClass(this->contextName, 1 );
        mace::deserialize(in, obj );
        versionMap.push_back( std::make_pair( ver, obj  ) );
    }
    /**
     * returns the now_serving number
     * now_serving number is the ticket of the next to run event
     * */
    OrderID const& getNowServing() const{
      return now_serving_eventId;
    }

    // chuangw: XXX: need to move init() to ContextBaseClass,
    // since every variables used are references to ContextBaseClass
    ContextThreadSpecific* init();
    
    int getCurrentMode() { 
      const OrderID& myEventId = ThreadStructure::myEventID();
      const uint64_t executeTicket = getExecuteEventTicket(myEventId);
      ASSERT( executeTicket > 0 );
      
      if( uncommittedEvents.first != executeTicket ){
        return -1;
      }
      return uncommittedEvents.second;
    }
    
    const uint64_t& getSnapshotVersion() { return init()->getSnapshotVersion(); }
    void setCurrentMode(int newMode) { init()->currentMode = newMode; }
    void setSnapshotVersion(const uint64_t& ver) { init()->snapshotVersion = ver; }
    /**
     * push an event into the context execution queue
     *
     * */
    void enqueueEvent(BaseMaceService* sv, AsyncEvent_Message* const p);
    bool enqueueEvent(const BaseMaceService* sv, mace::Event& event);

    void enqueueRoutine(BaseMaceService* sv, Routine_Message* const p, mace::MaceAddr const& source);

    void commitContext(BaseMaceService* sv, mace::OrderID const& eventId);

    void initialize(const mace::string& contextName, const OrderID& now_serving_eventId, const uint8_t serviceId, 
      const uint32_t contextId, const ContextEventOrder& contextEventOrder, const uint64_t create_now_committing_ticket, const OrderID& execute_now_committing_eventId,
      const uint64_t execute_now_committing_ticket );

    void initialize2(const mace::string& contextName, const uint8_t serviceId, const uint32_t contextId, 
      const uint64_t create_now_committing_ticket, const uint64_t execute_now_committing_ticket, 
      ContextStructure& ctxStructure  );

    void lock(  );
    void downgrade( int8_t requestedMode );
    void unlock(  );
    void nullTicket();
    /**
     * when context is removed, eliminate thread specfici memory associated with this context
     * */
    static void releaseThreadSpecificMemory(){
      // delete thread specific memories
      pthread_once( & mace::ContextBaseClass::global_keyOnce, mace::ContextBaseClass::createKeyOncePerThread );
      ThreadSpecificMapType* t = (ThreadSpecificMapType *)pthread_getspecific(global_pkey);
      if( t == 0 ){
        //chuangw: this can happen if init() is never called by this thread;
      }else{
        ThreadSpecificMapType::iterator ctIterator;
        for( ctIterator = t->begin(); ctIterator != t->end(); ctIterator++){
          delete ctIterator->second;
        }
      }
      delete t;
    }
    static void createKeyOncePerThread();
    static mace::string getTypeName( mace::string const& fullName ){
      mace::string s;

      size_t pos = fullName.find_last_of("." );

      if( pos == mace::string::npos ){
        s = fullName;
      }else{
        s = fullName.substr( pos+1 );
      }

      size_t bracket_pos = s.find_first_of("[" );
      if( bracket_pos == mace::string::npos ){
        return s;
      }

      s = s.substr( 0, bracket_pos );

      return s;
    }

    void enqueueCreateEvent(AsyncEventReceiver* sv, HeadEventDispatch::eventfunc func, mace::Message* p, bool useTicket);
    OrderID newCreateTicket();

    void increaseHandlingMessageNumber();
    void decreaseHandlingMessageNumber();
    void waitingForMessagesDone();
    void waitingForCreateEventsDone();

    void enqueueExecuteContextEvent( BaseMaceService* sv, ContextEvent& ce);
    void enqueueExecuteContextEventToHead( BaseMaceService* sv, ContextEvent& ce);

    // Event execution permission
    void waitForEventExecutePermission(BaseMaceService *sv, mace::EventOperationInfo const& eventOpInfo );
    void releaseContext( BaseMaceService* sv, const mace::OrderID& eventId, const mace::string& lockedContextName, const mace::string& srcContext,
      const mace::vector<mace::EventOperationInfo>& localLockRequests, const mace::vector<mace::string>& lockedContexts );

    void unlockContext( BaseMaceService* sv, mace::EventOperationInfo const& eventOpInfo,
      mace::vector<mace::EventOperationInfo> const& localLockRequests, mace::vector<mace::string> const& lockedContexts);

    void handleEventOperationDone( BaseMaceService const* sv, mace::EventOperationInfo const& eop );
    void addEventToContext( mace::OrderID const& eventId, mace::string const& toContext );
    void addEventFromContext( mace::OrderID const& eventId, mace::string const& fromContext );
    void addChildEventOp( const mace::EventOperationInfo& eventOpInfo );
    mace::set<mace::string> getEventToContextNames( mace::OrderID const& eventId );
    void notifyReadyToCommit( BaseMaceService* sv, mace::OrderID const& eventId, mace::string const& toContext, mace::vector<mace::string> const& executedContextNames );
    void getReadyToCommit( BaseMaceService* sv, mace::OrderID const& eventId );

    mace::vector<mace::EventOperationInfo> updateDominator( BaseMaceService* sv, const mace::string& src_ctx_name, 
      const mace::vector<mace::EventOperationInfo>& recv_eops);

    void handleOwnershipUpdateDominatorReply( BaseMaceService* sv, const mace::set<mace::string>& src_contextNames,
      const mace::vector<mace::EventOperationInfo>& eops);
    
    // void updateEventQueue(BaseMaceService* sv, mace::string const& src_contextName, mace::ContextEventDominator::EventLockQueue const& lockQueues );
    mace::vector< mace::EventOperationInfo > getLocalLockRequests( mace::OrderID const& eventId );
    mace::vector< mace::EventOperationInfo > getAllLocalLockRequests();
    mace::vector< mace::string > getLockedChildren( mace::OrderID const& eventId );
    void clearLocalLockRequests( mace::OrderID const& eventId );
    void clearLockedChildren( mace::OrderID const& eventId );
    void handleEventExecutePermission(BaseMaceService* sv, const mace::vector<mace::EventOperationInfo>& eops, 
      const mace::vector<mace::string>& permitContexts);

    bool checkEventExecutePermission(BaseMaceService* sv, mace::EventOperationInfo const& eventOpInfo, const bool& add_permit_contexts, 
      mace::set<mace::string>& permittedContextNames );
    
    void addEventPermitContexts( mace::OrderID const& eventId, mace::vector<mace::string> const& permitContexts);
    void addEventPermitContexts( mace::OrderID const& eventId, mace::set<mace::string> const& permitContexts);
    void addEventPermitContext( mace::OrderID const& eventId, mace::string const& permitContext);
    mace::vector<mace::string> getEventPermitContexts( const mace::OrderID& eventId );
    bool checkEventExecutePermitCache( const mace::OrderID& eventId, const mace::string& ctxName );
    void enqueueLocalLockRequest( const EventOperationInfo& eventOpInfo, const mace::string& ctx_name );
    void forwardWaitingBroadcastRequset(BaseMaceService* sv, const mace::EventOperationInfo& eventOpInfo);
    void setCurrentEventOp( const mace::EventOperationInfo& eop );

    bool checkBroadcastRequestExecutePermission( BaseMaceService* sv, const mace::EventOperationInfo& eventOpInfo, 
      mace::AsyncEvent_Message* reqObj);

    void initializeDominator( const mace::string& contextName, const mace::string& domCtx, const uint64_t& ver,
      const mace::vector<mace::string>& dominateCtxs );

    void handleWaitingDominatorRequests(BaseMaceService* sv, mace::vector<DominatorRequest> const& dominatorRequests);
    void enqueueLockRequests(BaseMaceService* sv, mace::vector<mace::EventOperationInfo> const& eops );
    void clearEventPermitCache();
    void clearEventPermitCacheNoLock();
    void notifyReleasedContexts(BaseMaceService* sv, mace::OrderID const& eventId, mace::vector<mace::string> const& releaseContexts);
    void notifyNextExecutionEvents(BaseMaceService* sv, 
      const mace::map< mace::string, mace::vector<mace::EventOperationInfo> >& permittedEventOps, 
      const mace::map< mace::OrderID, mace::vector<mace::string> >& permittedContextNames );
    bool localUnlockContext( mace::EventOperationInfo const& eventOpInfo,
      mace::vector<mace::EventOperationInfo> const& local_lock_requests, mace::vector<mace::string> const& local_require_contexts);
    mace::vector<mace::string> getLocalUnlockedContexts( mace::OrderID const& eventId, mace::vector<mace::string> const& vlockContexts );
    // void getLocalLockRequests( mace::OrderID const& eventId, mace::vector<mace::EventOperationInfo>& local_lock_requests, 
    //   mace::vector<mace::string>& local_require_contexts );
    
    void enqueueSubEvent( OrderID const& eventId, mace::EventRequestWrapper const& eventRequest);
    mace::vector< mace::EventRequestWrapper > getSubEvents(mace::OrderID const& eventId);
    mace::vector< mace::EventMessageRecord > getExternalMessages(mace::OrderID const& eventId);
    uint64_t getNextOperationTicket(OrderID const& eventId);
    void enqueueEventOperation( OrderID const& eventId, EventOperationInfo const& opInfo );

    void enqueueExternalMessage(OrderID const& eventId, EventMessageRecord const& msg);

    mace::vector<EventOperationInfo> extractOwnershipOpInfos( OrderID const& eventId );
    void enqueueOwnershipOpInfo( mace::OrderID const& eventId, mace::EventOperationInfo const& op );
    bool checkParentChildRelation(mace::OrderID& eventId, mace::string const& parentContextName, mace::string const& childContextName );
   
    void deleteEventExecutionInfo( OrderID const& eventId );

    mace::map<mace::OrderID, EventExecutionInfo>::iterator getEventExecutionInfo( const mace::OrderID& eventId);

    bool enqueueReadyExecuteEventQueue();
    bool enqueueReadyCreateEventQueue();
    bool enqueueReadyCommitEventQueue();

    void enqueueReadyExecuteEventQueueWithLock();
    void enqueueReadyCreateEventQueueWithLock();

    uint32_t createNewContext(BaseMaceService* sv, mace::string const& contextTypeName, mace::EventOperationInfo& eop);
    void handleCreateNewContextReply(mace::EventOperationInfo const& eventOpInfo, const uint32_t& newContextId);
    void enqueueSubEvent(BaseMaceService* sv, mace::EventOperationInfo const& eventOpInfo, mace::string const& targetContextName, 
        mace::EventRequestWrapper const& reqObject);
    void handleEnqueueSubEventReply(mace::EventOperationInfo const& eventOpInfo);
    void handleEnqueueExternalMessageReply(mace::EventOperationInfo const& eventOpInfo);
    void handleEnqueueOwnershipOpsReply(mace::EventOperationInfo const& eventOpInfo);
    void handleEventCommitDone( BaseMaceService* sv, mace::OrderID const& eventId, mace::set<mace::string> const& coaccess_contexts );

    void setEventExecutionInfo( mace::OrderID const& eventId, mace::string const& createContextName, mace::string const& targetContextName, const uint8_t event_op_type);
  
    void releaseLock(BaseMaceService* sv, mace::OrderID const& eventId );
    bool checkEventIsLockContext( mace::OrderID const& eventId );

    void setEventExecutionInfo(mace::OrderID const& eventId, mace::EventExecutionInfo const& info ) { eventExecutionInfos[eventId] = info; }
    // const mace::EventSkipRecord& getPreEventInfosStorage(const uint8_t serviceId, OrderID const& eventId);

    void addReaderEvent(mace::OrderID const& eventId);
    void addWriterEvent(mace::OrderID const& eventId);

    void removeReaderEvent(mace::OrderID const& eventId);
    void removeWriterEvent(mace::OrderID const& eventId);

    bool checkReEnterEvent(mace::OrderID const& eventId) const;
    void notifyExecutedContexts(mace::OrderID const& eventId);
    
    bool deferExternalMessage( EventOperationInfo const& eventOpInfo, mace::string const& targetContextName, uint8_t instanceUniqueID, MaceKey const& dest,  std::string const&  message, 
        registration_uid_t const rid );

    void setMigratingFlag(const bool flag);

    // Elasticity methods
    mace::ElasticityBehaviorAction* proposeAndCheckMigrationAction(const mace::ContextMapping& snapshot,
      const mace::string& marker, const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info,
      mace::map< mace::MaceAddr, double >& servers_cpu_usage, 
      mace::map< mace::MaceAddr, double >& servers_cpu_time, mace::map< mace::MaceAddr, mace::set<mace::string> >& migration_ctx_sets);
    uint64_t getCPUTime() { return runtimeInfo.getCPUTime(); }
    void markMigrationTicket( const uint64_t& ticket );

    void markStartTimestamp( mace::string const& marker ) { runtimeInfo.markStartTimestamp(marker); }
    void markEndTimestamp( mace::string const& marker ) { runtimeInfo.markEndTimestamp(marker); }
    mace::ElasticityBehaviorAction* getImprovePerformanceAction( mace::ContextMapping const& snapshot, 
      mace::map<mace::MaceAddr, mace::ServerRuntimeInfo> const& servers_info, mace::string const& marker,
      const mace::map< mace::MaceAddr, mace::set<mace::string> >& migration_ctx_sets, 
      const mace::map< mace::MaceAddr, double >& servers_cpu_usage, const mace::map< mace::MaceAddr, double >& servers_cpu_time );
    bool isImprovePerformance( const mace::MaceAddr& dest_node, const mace::ContextMapping& snapshot,
      const mace::map< mace::MaceAddr, mace::set<mace::string> >& migration_ctx_sets, 
      const mace::map< mace::MaceAddr, double >& servers_cpu_usage, const mace::map< mace::MaceAddr, double >& servers_cpu_time,
      const mace::string& marker );
    uint64_t getCurrentLatency( const mace::string& marker ) { return runtimeInfo.getMarkerTotalLatency(marker); }
    double getMarkerAvgLatency( const mace::string& marker ) { return runtimeInfo.getMarkerAvgLatency(marker); }
    mace::map< mace::string, uint64_t > getContextsInterCount() { return runtimeInfo.getContextInteractionCount(); }
    mace::map< mace::string, uint64_t > getFromAccessCountByTypes(const mace::set<mace::string>& context_types) { return runtimeInfo.getFromAccessCountByTypes(context_types); }
    mace::map< mace::string, uint64_t > getToAccessCountByTypes(const mace::set<mace::string>& context_types) { return runtimeInfo.getToAccessCountByTypes(context_types); }
    mace::map< mace::string, uint64_t > getEstimateContextsInterSize() { return runtimeInfo.getEstimateContextsInteractionSize(); }
    uint64_t getMarkerCount( const mace::string& marker ) { return runtimeInfo.getMarkerCount(marker); }

private:
    
    struct CondQueueComp{
      bool operator()( const std::pair<uint64_t, pthread_cond_t*>& p1, const std::pair<uint64_t, pthread_cond_t*>& p2 ){
        return p1.first > p2.first;
      }
    };
    struct BypassSorter{
      // The bypass range shouldn't intersect
      bool operator()(const std::pair<uint64_t,uint64_t>& p1, const std::pair<uint64_t,uint64_t>& p2){
        return (p1.first<p2.first);
      }
    };

    typedef std::priority_queue< std::pair<uint64_t, pthread_cond_t*>, std::vector<std::pair<uint64_t, pthread_cond_t*> >, CondQueueComp > CondQueue;
    typedef std::set< std::pair< uint64_t, uint64_t >, BypassSorter > BypassQueueType ;

    pthread_key_t pkey;
    pthread_once_t keyOnce;
public:
    int numReaders;
    int numWriters;
private:
    CondQueue conditionVariables;
    CondQueue commitConditionVariables;
    pthread_mutex_t _context_ticketbooth; 
    BypassQueueType bypassQueue;
    BypassQueueType commitBypassQueue;
    mace::pair<uint64_t, int8_t> uncommittedEvents;
    
    static pthread_key_t global_pkey;
    
protected:
    typedef std::deque<std::pair<uint64_t, const ContextBaseClass* > > VersionContextMap;
    mutable VersionContextMap versionMap;
};

class __EventStorageStruct__: public Serializable, public PrintPrintable {
public:
  InternalMessageHelperPtr msg;
  uint8_t contextEventType;
  uint8_t messageType;
  uint8_t sid;
  EventOperationInfo eventOpInfo;

  __EventStorageStruct__(): msg( NULL ), contextEventType(ContextEvent::TYPE_NULL), messageType(mace::InternalMessage::UNKNOWN), sid(0) { }
  __EventStorageStruct__( const InternalMessageHelperPtr msg, const uint8_t contextEventType, const uint8_t messageType, const uint8_t sid, 
    const mace::EventOperationInfo& eventOpInfo ): msg(msg), contextEventType(contextEventType),
    messageType(messageType), sid(sid), eventOpInfo(eventOpInfo) { } 

  ~__EventStorageStruct__() { 
    msg = NULL;
  }

  __EventStorageStruct__& operator=(const __EventStorageStruct__& orig){
    ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
    this->msg = orig.msg;
    this->contextEventType = orig.contextEventType;
    this->messageType = orig.messageType;
    this->sid = orig.sid;
    this->eventOpInfo = orig.eventOpInfo;
    
    return *this;
  }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);

  void print(std::ostream& out) const {
    out<< "__EventStorageStruct__(";
    out<< "contextEventType = "; mace::printItem(out, &(contextEventType)); out<<", ";
    out<< "msg = "; mace::printItem(out, msg); //out<<", ";
      //out<< "contextName = "; mace::printItem(out, &(ctxName)); 
    out<< ")";
  }

  void printNode(PrintNode& pr, const std::string& name) const {
    mace::PrintNode printer(name, "__EventStorageStruct__" );
  
    mace::printItem( printer, "contextEventType", &contextEventType );
    mace::printItem( printer, "msg", msg );
      //mace::printItem( printer, "ctxName", &ctxName );
    pr.addChild( printer );
  }

  int deserializeEvent( std::istream& in ){
    BaseMaceService* serviceInstance = BaseMaceService::getInstance( sid );
    mace::Message* ptr;
    int count = serviceInstance->deserializeMethod( in, ptr );
    msg = InternalMessageHelperPtr( static_cast< InternalMessageHelperPtr >( ptr ) );
    return count;
  }
  
  int deserializeRoutine( std::istream& in ){
    BaseMaceService* serviceInstance = BaseMaceService::getInstance( sid );
    mace::Message* ptr;
    int count = serviceInstance->deserializeMethod( in, ptr );
    msg = InternalMessageHelperPtr( static_cast< InternalMessageHelperPtr >( ptr ) );
    return count;
  }

};

class __CreateEventStorage__: public Serializable, public PrintPrintable {
public:
  mace::InternalMessageHelperPtr msg;
  mace::OrderID eventId;
  uint8_t sid;

  __CreateEventStorage__(): msg(NULL), eventId( ), sid(0) { }
  __CreateEventStorage__(const mace::InternalMessageHelperPtr msg, mace::OrderID const& eventId, const uint8_t sid ) { 
    this->msg = const_cast<mace::InternalMessageHelperPtr>(msg);
    this->eventId = eventId;
    this->sid = sid;
  }

  ~__CreateEventStorage__() { 
    msg = NULL;
  } 

  void print(std::ostream& out) const {
    out << "__CreateEventStorage__(";
    out << "msg = "; mace::printItem(out, msg); out << ",";
    out << "eventId = "; mace::printItem(out, &eventId);
    out << ")";  
  }

  void printNode(PrintNode& pr, const std::string& name) const {
     
  }

  void serialize(std::string& str) const{
    mace::serialize( str, &sid );
    //ASSERTMSG(msg != NULL, "__CreateEventStorage__::msg is NULL!");
    mace::serialize( str, msg );
    mace::serialize( str, &eventId );
  }

  int deserialize(std::istream & is) throw (mace::SerializationException){
    int serializedByteSize = 0;
    serializedByteSize += mace::deserialize(is, &sid);

    BaseMaceService* serviceInstance = BaseMaceService::getInstance( sid );
    mace::Message* ptr;
    serializedByteSize += serviceInstance->deserializeMethod( is, ptr );
    msg = InternalMessageHelperPtr( static_cast< InternalMessageHelperPtr >( ptr ) ) ;

    serializedByteSize += mace::deserialize( is, &eventId );
    return serializedByteSize;
  }

  __CreateEventStorage__& operator=(const __CreateEventStorage__& orig){
    ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
    msg = orig.msg;
    eventId = orig.eventId;
    sid = orig.sid;

    return *this;
  }
};

class ContextBaseClassParams: public Serializable, public PrintPrintable {
public:
  ContextBaseClassParams() { }
  ~ContextBaseClassParams() { }
  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);
  void initialize( ContextBaseClass* ctxObj);

  void print(std::ostream& out) const {
    out<< "ContextBaseClassParams(";
    out<< "contextName = "; mace::printItem(out, &(contextName)); out<<", ";
    out<< "contextTypeName = "; mace::printItem(out, &(contextTypeName)); out<<", ";
    out<< "serviceId = "; mace::printItem(out, &(serviceId)); out<<", ";
    out<< "contextId = "; mace::printItem(out, &(contextId)); out<<", ";
    out<< "createEventQueue = "; mace::printItem(out, &(createEventQueue)); out<<", ";
    out<< "waitingEvents = "; mace::printItem(out, &(waitingEvents)); out<<", ";
    out<< "executeCommitEventQueue = "; mace::printItem(out, &(executeCommitEventQueue)); out<<", ";
    out<< "executeEventQueue = "; mace::printItem(out, &(executeEventQueue)); out<<", ";
    out<< "create_now_committing_ticket = "; mace::printItem(out, &(create_now_committing_ticket)); out<<", ";
    out<< "execute_now_committing_eventId = "; mace::printItem(out, &(execute_now_committing_eventId)); out<<", ";
    out<< "execute_now_committing_ticket = "; mace::printItem(out, &(execute_now_committing_ticket)); out<<", ";
    out<< "now_serving_eventId = "; mace::printItem(out, &(now_serving_eventId)); out<<", ";
    out<< "now_serving_execute_ticket = "; mace::printItem(out, &(now_serving_execute_ticket)); out<<", ";
    out<< "lastWrite = "; mace::printItem(out, &(lastWrite)); out<<", ";
    out<< "contextEventOrder = "; mace::printItem(out, &(contextEventOrder)); out<<", ";
    out<< "dominator = "; mace::printItem(out, &(dominator)); out<<", ";
    out<< "createTicketNumber = "; mace::printItem(out, &(createTicketNumber)); out<<", ";
    out<< "executeTicketNumber = "; mace::printItem(out, &(executeTicketNumber)); out<<", ";
    out<< "now_serving_create_ticket = "; mace::printItem(out, &(now_serving_create_ticket)); out<<", ";
    out<< "now_max_execute_ticket = "; mace::printItem(out, &(now_max_execute_ticket)); out<<", ";
    out<< "eventExecutionInfos = "; mace::printItem(out, &(eventExecutionInfos) );
    out<< ")";
  }

  void printNode(PrintNode& pr, const std::string& name) const {
     
  }

  ContextBaseClassParams& operator=(const ContextBaseClassParams& orig){
    ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
    this->contextName = orig.contextName; ///< The canonical name of the context
    this->contextTypeName = orig.contextTypeName;
    this->serviceId = orig.serviceId; ///< The service in which the context belongs to
    this->contextId = orig.contextId;
    this->createEventQueue = orig.createEventQueue;
    this->waitingEvents = orig.waitingEvents;
    this->executeCommitEventQueue = orig.executeCommitEventQueue;
        
    this->executeEventQueue = orig.executeEventQueue;
    this->create_now_committing_ticket = orig.create_now_committing_ticket;
    this->execute_now_committing_eventId = orig.execute_now_committing_eventId;
    this->execute_now_committing_ticket = orig.execute_now_committing_ticket;

    this->now_serving_eventId = orig.now_serving_eventId;
    this->now_serving_execute_ticket = orig.now_serving_execute_ticket;
    
    this->lastWrite = orig.lastWrite;

    this->contextEventOrder = orig.contextEventOrder;
    this->dominator = orig.dominator;

    this->runtimeInfo =orig.runtimeInfo;

    this->createTicketNumber = orig.createTicketNumber;
    this->executeTicketNumber = orig.executeTicketNumber;
    
    this->executedEventsCommitFlag = orig.executedEventsCommitFlag;
    this->now_serving_create_ticket = orig.now_serving_create_ticket;
    this->now_max_execute_ticket = orig.now_max_execute_ticket;
    
    this->eventExecutionInfos = orig.eventExecutionInfos;
    this->readerEvents = orig.readerEvents;
    this->writerEvents = orig.writerEvents;

    this->migrationEventWaiting = orig.migrationEventWaiting;
    this->skipCreateTickets = orig.skipCreateTickets;
    return *this;
  }

public:
  mace::string contextName; ///< The canonical name of the context
  mace::string contextTypeName;
  
  uint8_t serviceId; ///< The service in which the context belongs to
  uint32_t contextId;
  
  mace::vector< __CreateEventStorage__ > createEventQueue;
  ContextBaseClass::EventStorageType waitingEvents;

  mace::map<uint64_t, mace::Event> executeCommitEventQueue;
  
  mace::deque<ContextEvent> executeEventQueue;

  uint64_t create_now_committing_ticket;
  OrderID execute_now_committing_eventId;
  uint64_t execute_now_committing_ticket;

  OrderID now_serving_eventId;
  uint64_t now_serving_execute_ticket;
  
  uint64_t lastWrite;

  ContextEventOrder contextEventOrder;
  ContextEventDominator dominator;

  mace::ContextRuntimeInfo runtimeInfo;

  uint64_t createTicketNumber;
  uint64_t executeTicketNumber;

  mace::map< uint64_t, bool > executedEventsCommitFlag;

  uint64_t now_serving_create_ticket;
  uint64_t now_max_execute_ticket;
  
  mace::map< OrderID, EventExecutionInfo > eventExecutionInfos;

  mace::set<OrderID> readerEvents;
  mace::set<OrderID> writerEvents;

  bool migrationEventWaiting;
  mace::set<uint64_t> skipCreateTickets;
};



};
#endif
