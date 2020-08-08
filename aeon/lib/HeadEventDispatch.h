#ifndef __HEAD_EVENT_DISPATCH_h
#define __HEAD_EVENT_DISPATCH_h

#include "mace.h"
#include "mace-macros.h"
#include "ThreadPool.h"
#include "Message.h"
#include "InternalMessage.h"

#include "MaceKey.h"
#include "CircularQueueList.h"
#include <deque>
#include "InternalMessageInterface.h"

using mace::InternalMessageSender;
/**
 * \file HeadEventDispatch.h
 * \brief declares the HeadEventDispatch class 
 */
class AsyncEventReceiver;

namespace mace{
  class AgentLock;
  class __event_MigrateContext;
}
namespace HeadEventDispatch {
  typedef mace::map< mace::OrderID, uint64_t, mace::SoftState> EventRequestTSType;
  /// the timestamp where the event request is created
  extern EventRequestTSType eventRequestTime;
  /// the timestamp where the event request is processed
  extern EventRequestTSType eventStartTime;
  extern pthread_mutex_t startTimeMutex;
  extern pthread_mutex_t requestTimeMutex;

  extern pthread_mutex_t samplingMutex;
  extern bool sampleEventLatency;
  extern uint32_t accumulatedLatency;
  extern uint32_t accumulatedEvents;

  void insertEventStartTime(uint64_t eventID);
  void insertEventRequestTime(uint64_t eventID);
  void sampleLatency( bool flag );
  double getAverageLatency(  );

  void waitAfterCommit( uint64_t eventTicket );

  class GlobalHeadEventHelper {

  };

  class HeadCreateContextEvent: public GlobalHeadEventHelper {
  public:
    mace::string createContextName;
    mace::OrderID eventId;
    mace::vector< mace::pair<mace::string, mace::string> > ownershipPairs; 
    mace::map< mace::string, uint64_t> vers;
    
    HeadCreateContextEvent(): createContextName(""), eventId(), ownershipPairs(), vers() { }

    HeadCreateContextEvent(const mace::string& ctxName, const mace::OrderID& eventId, 
      const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, const mace::map< mace::string, uint64_t>& vers ): 
      createContextName(ctxName), eventId(eventId), ownershipPairs(ownershipPairs), vers(vers) { }

    ~HeadCreateContextEvent() { }
  };

  class HeadModifyOwnershipEvent: public GlobalHeadEventHelper {
  public:
    mace::string ctxName;
    mace::EventOperationInfo eop;
    mace::vector<mace::EventOperationInfo> ownershipOpInfos;

    HeadModifyOwnershipEvent( mace::EventOperationInfo const& eop, mace::string const& ctxName, mace::vector<mace::EventOperationInfo> const& ownershipOpInfos):
      ctxName( ctxName ), eop( eop ), ownershipOpInfos( ownershipOpInfos ) {}
    ~HeadModifyOwnershipEvent() {
      ownershipOpInfos.clear();
    }
  };


  class HeadContextMigrationEvent: public GlobalHeadEventHelper {
  public:
    mace::__event_MigrateContext* msg;

    HeadContextMigrationEvent(): msg(NULL) { }
    HeadContextMigrationEvent( mace::__event_MigrateContext* msg): msg(msg) { }

  };

  class HeadExitEvent: public GlobalHeadEventHelper {
  public:
    mace::OrderID exitEventId;

    HeadExitEvent(): exitEventId( ) { }
    HeadExitEvent(const mace::OrderID& exitEventId): exitEventId(exitEventId) { }
  };


  typedef GlobalHeadEventHelper* HeadGlobalEventHelperPtr;

  class GlobalHeadEvent {
  public:
    static const uint8_t GlobalHeadEvent_UNDEF = 0;
    static const uint8_t GlobalHeadEvent_CREATECONTEXT = 1;
    static const uint8_t GlobalHeadEvent_MIGRATION = 2;
    static const uint8_t GlobalHeadEvent_EXIT = 3;
    static const uint8_t GlobalHeadEvent_MODIFY_OWNERSHIP = 4;

  public:
    uint8_t globalEventType;
    HeadGlobalEventHelperPtr helper;
    AsyncEventReceiver* cl;
    uint64_t ticket;

    
    GlobalHeadEvent(): globalEventType(GlobalHeadEvent_UNDEF), helper(NULL), cl(NULL), ticket(0) { }
    GlobalHeadEvent(const uint8_t type, HeadGlobalEventHelperPtr p, AsyncEventReceiver* cl, const uint64_t ticket): globalEventType(type), 
        helper(p), cl(cl), ticket(ticket) {}

    ~GlobalHeadEvent() {
      delete helper;
      helper = NULL;
    }
    
  };


  /******************************************** Elasticity Event *****************************************************************/
class ElasticityHeadEventHelper { };

class HeadContextsColocateEvent: public ElasticityHeadEventHelper {
public:
  mace::string colocateDestContext;
  mace::vector<mace::string> colocateSrcContexts;
    
  HeadContextsColocateEvent(const mace::string& colocate_dest_context, const mace::vector<mace::string>& colocate_src_contexts ): 
    colocateDestContext(colocate_dest_context), colocateSrcContexts(colocate_src_contexts) { }
  ~HeadContextsColocateEvent() { }
};

typedef ElasticityHeadEventHelper* HeadElasticityEventHelperPtr;

class ElasticityHeadEvent {
public:
  static const uint8_t ElasticityHeadEvent_CONTEXTS_COLOCATE = 1;
  
public:
  uint8_t elasticityEventType;
  HeadElasticityEventHelperPtr helper;
  AsyncEventReceiver* cl;
  uint8_t serviceId;

  ElasticityHeadEvent( const uint8_t type, HeadElasticityEventHelperPtr helper, AsyncEventReceiver* cl, const uint8_t service_id ): 
    elasticityEventType( type ), helper(helper), cl( cl ), serviceId( service_id ) { }
  ~ElasticityHeadEvent() { 
    delete helper;
    helper = NULL;
  }
    
};


  class HeadMigration {
public:
    static void setState( const uint16_t newState ){
      ScopedLock sl( lock );
      state = newState;
    }
    static const uint16_t getState( ){
      ScopedLock sl( lock );
      return state;
    }
    static void setMigrationEventID( const mace::OrderID eventId ){
      ScopedLock sl( lock );
      migrationEventId = eventId;
    }
    static const mace::OrderID& getMigrationEventID( ){
      ScopedLock sl( lock );
      return migrationEventId;
    }
    static void setNewHead( const MaceAddr& head ){
      ScopedLock sl( lock );
      newHeadAddr = head;
    }
    static const MaceAddr getNewHead( ){
      ScopedLock sl( lock );
      return newHeadAddr;
    }
public:
    static const uint16_t HEAD_STATE_NORMAL = 0;
    static const uint16_t HEAD_STATE_MIGRATING = 1;
    static const uint16_t HEAD_STATE_MIGRATED = 2;
private:
    static pthread_mutex_t lock;
    static uint16_t state;
    static mace::OrderID migrationEventId;
    static mace::MaceAddr newHeadAddr;

  };

  typedef void (AsyncEventReceiver::*eventfunc)( mace::RequestType );
  class HeadEvent {
    public:
      AsyncEventReceiver* cl;
      eventfunc func;
      mace::RequestType param;
      mace::OrderID eventId;
      mace::string contextName;
      
    public:
      HeadEvent() : cl(NULL), func(NULL), param(NULL), eventId(), contextName() { }
      HeadEvent(AsyncEventReceiver* cl, eventfunc func, 
#ifdef EVENTREQUEST_USE_SHARED_PTR
  mace::RequestType&
#else
  mace::RequestType
#endif
 param, mace::OrderID& eventId, mace::string const& contextName ) : 
      cl(cl), func(func), param(param), eventId(eventId), contextName(contextName){ }

      void fire() {
        ThreadStructure::setEventID( eventId );
        (cl->*func)(param);
      }

      HeadEvent& operator=(const HeadEvent& orig){
        ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
        this->cl = orig.cl;
        this->func = orig.func;
        this->param = orig.param;
        this->eventId = orig.eventId;
        this->contextName = orig.contextName;
        return *this;
      }

			mace::ContextBaseClass* getCtxObj();

      ~HeadEvent() {
        param = NULL;
      }
  };


  void init();
  void prepareHalt(const mace::OrderID& _exitEventId);
  void haltAndWait();
  void haltAndWaitCommit();
  void haltAndNoWaitCommit();

  class HeadEventTP;
	struct ThreadArg {
    HeadEventTP* p;
    uint32_t i;
  };

  /**
   * The thread pool of the head node for creating events
   *
   * The thread pool is of dynamic size between minThreadSize and maxThreadSize. This object is similar to ThreadPool.
   * 
   * */
  class HeadEventTP{
  friend class mace::AgentLock;
  public:
  //typedef mace::ThreadPool<HeadEventTP, HeadEvent> ThreadPoolType;
  private: 
    uint32_t idle; ///< number of idle threads
    bool* sleeping; ///< whether the thread is sleeping or not
    ThreadArg* args; ///< argument to the thread
    bool busyCommit; ///< whether or not the commit thread is sleeping
    const uint32_t minThreadSize; ///< minimum number of threads
    const uint32_t maxThreadSize; ///< max number of threads
    static pthread_t* headThread; ///< pthread_t array of threads
    static pthread_t headCommitThread; ///< pthread_t array of commit threads
    HeadEvent data; ///< record of an event request
    mace::Event* committingEvent; ///< pointer to the currently commiting event
    pthread_cond_t signalv; ///< conditional variable for head thread
    pthread_cond_t signalc; ///< conditional variable for commit thread

    static pthread_mutex_t executingSyncMutex;
    static pthread_cond_t executingSyncCond;

		//bsang
		uint32_t globalIdle;
		const uint32_t minGlobalThreadSize;
		const uint32_t maxGlobalThreadSize;
		GlobalHeadEvent* globalEvent;
		pthread_cond_t signalg;
		ThreadArg* globalArgs;
		static pthread_t* globalHeadThread;
		bool* globalSleeping;

    static uint64_t now_serving_ticket;
    static uint64_t next_ticket;

    static mace::map<uint32_t, mace::string> migratingContexts;
    static mace::map<uint32_t, mace::string> migratingContextsCopy;
    static mace::map< mace::MaceAddr, mace::set<mace::string> > migratingContextsOrigAddrs;
    static mace::MaceAddr destAddr;
    static mace::set<mace::string> waitingMigrationContexts;

    static mace::map<mace::MaceAddr, mace::string> externalCommContextMap;

    static mace::set<mace::string> waitingReplyContextNames;

    // elasticity variable
    uint32_t elasticityIdle;
    const uint32_t minElasticityThreadSize;
    const uint32_t maxElasticityThreadSize;
    ElasticityHeadEvent* elasticityEvent;
    pthread_cond_t signale;
    ThreadArg* elasticityArgs;
    static pthread_t* elasticityHeadThread;
    bool* elasticitySleeping;

    static void doExecuteEvent(HeadEvent const& thisev);
    static void tryWakeup();
    static void tryWakeupGlobal(); 

    static bool migrating_waiting_flag; 
    static mace::set<uint64_t> waitingCommitEventIds;

    static bool migrating_flag;
    
    // debug
    static uint64_t req_start_time;
    static uint8_t req_type;

    static uint64_t total_latency;
    static uint64_t create_latency;
    static uint64_t ownership_latency;
    static uint64_t migrating_latency;

    static uint64_t total_req_count; 
    static uint64_t create_req_count;
    static uint64_t ownership_req_count;
    static uint64_t migrating_req_count;

    static uint64_t GLOBAL_EVENT_OUTPUT;

    static void collectGlobalEventInfo();

  public:
    /**
     * constructor
     * @param minThreadSize minimum number of threads
     * @param maxThreadSize maximum number of threads
     * */
    HeadEventTP( const uint32_t minThreadSize, const uint32_t maxThreadSize, const uint32_t minGlobalThreadSize, 
        const uint32_t maxGlobalThreadSize);

    /**
     * destructor
     * */
    ~HeadEventTP() ;

    /**
     * wait on the conditional variable
     * */
    void wait();

		void globalWait();
    /**
     * (event commit thread) wait on the conditional variable
     * */
    void commitWait() ;

    /**
     * signal one head thread
     * */
    void signalSingle() ;
    void signalGlobalSingle();
    /**
     * signal all head threads
     * */
    void signalAll() ;
    void signalGlobalAll();
    /**
     * signal commit thread
     * */
    void signalCommitThread() ;
    /**
     * cond function
     * @return TRUE if the next event is ready to initialize
     * */
    bool hasPendingEvents();

		bool hasPendingGlobalEvents();
    /**
     * cond function
     * @return TRUE if the next event is ready to commit
     * */
    bool hasUncommittedEvents();
    //static bool nextToCommit( uint64_t eventID);
    // setup
    /**
     * set up the thread structure to initialize the event
     * */
    void executeEventSetup();

		void executeGlobalEventSetup();
    /**
     * set up the thread structure to commit the event
     * */
    void commitEventSetup( );
    // process
    /**
     * execute the callback function of the event request
     * */
    void executeEventProcess();

		void executeGlobalEventProcess();
    void executeGlobalContextCreateEventProcess();
    void executeGlobalModifyOwnershipEventProcess();
    /**
     * execute the callback function of the commit request
     * */
    void commitEventProcess() ;
    // finish
    /**
     * finish initializing the event 
     * */
    void executeEventFinish();
    /**
     * finish commit request
     * */
    void commitEventFinish() ;

    /**
     * this is where the head thread starts
     * */
    static void* startThread(void* arg) ;
    /**
     * this is where the commit thread starts
     * */
    static void* startCommitThread(void* arg) ;

		/**  
		 * this is where the head global thread starts
		 * */
		static void* startGlobalThread(void* arg);

    /**
     * execution of the head thread
     * @param n the id of the thread
     * */
    void run(uint32_t n);

		void globalrun(uint32_t n);
    /**
     * execution of the commit thread
     * */
    void runCommit();

    /**
     * signal the head thread to stop and wait for its termination
     * */
    void haltAndWait();
    void haltAndWaitHead();
    /**
     * signal the head thread to stop and return immediately
     * @param exitTicket the ticket id of exit event
     * */
    void prepareHalt(const mace::OrderID& _exitEventId);
    /**
     * signal the commit thread to stop and wait for its termination
     * */
    void haltAndWaitCommit();
    void haltAndNoWaitCommit();
    void haltAndWaitGlobalHead();
    /**
     * put an event request in the head queue
     * @param sv the service that starts the request
     * @param func the callback function
     * @param p the request object
     * @param useTicket whether or not to use the current ticket as the ID of the new event.
     * */
    static void executeEvent(AsyncEventReceiver* sv, eventfunc func, mace::Message* p, bool useTicket);
    //static void executeExternalMessage( AsyncEventReceiver* sv, eventfunc func, mace::Message* p, mace::MaceAddr const& src);
    /**
     * Call by Event::commit() to put all sub event requests in the head queue
     * */
    static void executeEvent(eventfunc func, mace::Event::EventRequestType subevents, bool useTicket);

		static void executeContextCreateEvent(AsyncEventReceiver* cl, const mace::string& ctxName, const mace::OrderID& eventId, 
      const mace::MaceAddr& src, const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, 
      const mace::map< mace::string, uint64_t>& vers);
    
    static void executeContextMigrationEvent(AsyncEventReceiver* cl, uint8_t const serviceID, mace::string const& contextName, MaceAddr const& destNode, bool const rootOnly);
    static void executeModifyOwnershipEvent(AsyncEventReceiver* cl, mace::EventOperationInfo const& eop, mace::string const& ctxName,
      mace::vector<mace::EventOperationInfo> const& ownershipOpInfos);
    static void enqueueGlobalHeadEvent(GlobalHeadEvent* ge);

    static void signalMigratingContextThread(const uint64_t ticket);

    static void handleContextOwnershipUpdateReply( mace::set<mace::string> const& ctxNames );

    /**
     * put an event in the commit queue
     * @param event the event object
     * */
    static void commitEvent(const mace::Event& event);

    static void commitGlobalEvent( const uint64_t ticket);

    static void accumulateEventLifeTIme(mace::Event const& event);
    static void accumulateEventRequestCommitTime(mace::Event const& event);

    // migration method
    static void commitMigrationEvent( const BaseMaceService* sv, mace::OrderID const& eventId, const uint32_t ctxId );
    
    static void setMigratingContexts( BaseMaceService* sv, mace::OrderID const& eventId, mace::map< uint32_t, mace::string > const& my_migratingContexts, 
      mace::map<mace::MaceAddr, mace::set<mace::string> > const& origAddrs, mace::MaceAddr const& destNode );
    static void waitingForMigrationContextMappingUpdate( BaseMaceService* sv, mace::map< mace::MaceAddr, mace::set<uint32_t> > const& origContextIdAddrs, mace::MaceAddr const& destNode,
      mace::Event const& event, const uint64_t preVersion, mace::ContextMapping const& ctxMapping, mace::OrderID const& eventId );

    static void deleteMigrationContext( const mace::string& context_name );

    // elasticity method
    static void* startElasticityThread(void* arg);
    void elasticityrun(uint32_t n);
    bool hasPendingElasticityEvents();
    void elasticityWait();
    void executeElasticityEventSetup();
    void executeElasticityEventProcess();
    static void enqueueElasticityContextsColocationRequest( AsyncEventReceiver* cl, mace::string const& colocate_dest_context, 
      mace::vector<mace::string> const& colocate_src_contexts, const uint8_t service_id);
    static void enqueueElasticityHeadEvent( ElasticityHeadEvent* elevent);
    void executeElasticityContextsColocation( AsyncEventReceiver* cl, mace::string const& colocate_dest_context, 
      mace::vector<mace::string> const& colocate_src_contexts, const uint8_t service_id);
    static void tryWakeupElasticity();
    void signalElasticitySingle();

  };
  HeadEventTP* HeadEventTPInstance() ;

  class HeadTransportQueueElement {
    private: 
      InternalMessageSender* cl;
      mace::MaceAddr dest;
      mace::AsyncEvent_Message* eventObject;
      uint64_t instanceUniqueID;

    public:
      HeadTransportQueueElement() : cl(NULL), eventObject(NULL), instanceUniqueID(0) {}
      HeadTransportQueueElement(InternalMessageSender* cl, mace::MaceAddr const& dest, mace::AsyncEvent_Message* const eventObject, uint64_t instanceUniqueID) : cl(cl), dest(dest), eventObject(eventObject), instanceUniqueID(instanceUniqueID) {}
      void fire() {
        ADD_SELECTORS("HeadTransportQueueElement::fire");
        mace::InternalMessageID msgId;
        mace::InternalMessage msg( eventObject, msgId, instanceUniqueID );
        cl->sendInternalMessage(dest, msg);
        msg.unlinkHelper();
        delete eventObject;
      }
  };
  typedef std::queue< HeadTransportQueueElement > MessageQueue;
  class HeadTransportTP {
    typedef mace::ThreadPool<HeadTransportTP, HeadTransportQueueElement> ThreadPoolType;
    private:
      static pthread_mutex_t ht_lock;
      static MessageQueue mqueue;
      ThreadPoolType *tpptr;
      bool runDeliverCondition(ThreadPoolType* tp, uint threadId);
      void runDeliverSetup(ThreadPoolType* tp, uint threadId);
      void runDeliverProcessUnlocked(ThreadPoolType* tp, uint threadId);
      void runDeliverProcessFinish(ThreadPoolType* tp, uint threadId);
    public:
      HeadTransportTP(uint32_t minThreadSize, uint32_t maxThreadSize  );
      ~HeadTransportTP();

      void signal();

      void haltAndWait();
      static void lock(); // lock

      static void unlock(); // unlock
      static void sendEvent(InternalMessageSender* sv, mace::MaceAddr const& dest, mace::AsyncEvent_Message* const eventObject, uint64_t instanceUniqueID);

      static void init();
  };

	

}

#endif
