#ifndef _CONTEXTSERVICE_H
#define _CONTEXTSERVICE_H
/**
 * \file ContextService.h
 * \brief declares ContextService, accessory trait classes and event messages
 */

#include "mace.h"
#include "ContextBaseClass.h"
#include "ContextDispatch.h"
#include "mvector.h"
#include "m_map.h"
#include "mstring.h"
#include "mpair.h"
#include "pthread.h"
#include "ScopedLock.h"
#include "ContextMapping.h"
#include "Message.h"
#include "Printable.h"
#include "Event.h"
#include "InternalMessage.h"
#include "NullInternalMessageProcessor.h"
#include "ScopedContextRPC.h"
#include "AsyncDispatch.h"
#include "AccessLine.h"
#include "ContextOwnership.h"
#include "ElasticPolicy.h"
#include "eMonitor.h"
#include "ControlMessageChannel.h"

/**
 * \file ContextService.h
 * \brief declares the base class for all context'ed services.
 */

#define USE_HEAD_TRANSPORT_THREAD
//#define USE_HEAD_ENQUEUE_THREAD

using mace::__asyncExtraField;
using mace::ContextMapping;
using mace::InternalMessageSender;
using mace::InternalMessageReceiver;
namespace mace{
  class __ServiceStackEvent__;
  class __ScopedTransition__;
  class __ScopedRoutine__;
  class __CheckMethod__;
  
  template <typename T> 
  class Delegator;

  class OnePhysicalNode{};
  class DistributedLogicalNode{};
  template<typename N>
  struct Locality_trait {
    bool isLocal( MaceAddr const& dest ) const {return false; }
  };
  /// specialized trait template for distributed logical node service
  template<>
  struct Locality_trait<DistributedLogicalNode> {
    bool isLocal( MaceAddr const& dest ) const {
      if ( dest == Util::getMaceAddr() ){
        return true;
      }
      return false;
    }
  };
  /// specialized trait template for one-phyiscal-node logical node service
  template<>
  struct Locality_trait<OnePhysicalNode> {
    bool isLocal( MaceAddr const& dest ){
      return true;
    }
  };

  class ContextLocatorInterface{
public:
    virtual MaceAddr const& getDestination() const = 0;
    //virtual MaceAddr const& getNextDestination() const = 0;
    //virtual bool isNextLocal() const = 0;
  };

  /**
   * \brief message object for the events that does not have implemented transition handlers
   *
   * used to waste the ticket
   */
  class NullEventMessage: public Message, public PrintPrintable{
  public:
    NullEventMessage( mace::OrderID const& eventId ): event( eventId ){}
    mace::Event event;
    static const uint8_t messageType = 255;
    static uint8_t getMsgType() { return messageType; }
    uint8_t getType() const { return NullEventMessage::getMsgType(); }

    mace::Event& getEvent() { return event; }

    std::string toString() const { 
      mace::string str;
      return str;
    }
    void print(std::ostream& __out) const { }
    size_t getSerializedSize() const { return 0; }
    void serialize(std::string& str) const { }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) { 
      return 0;
    }
    void sqlize(mace::LogNode* __node) const { }

    std::string serializeStr() const { 
      mace::string str;
      return str;
    }
    void deserializeStr(const std::string& __s) throw (mace::SerializationException) { }
  };

  class __event_MigrateContext: public Message, public PrintPrintable {
  public:
    __event_MigrateContext( mace::OrderID const& eventId, uint8_t const serviceID, mace::string const& contextName, MaceAddr const& destNode, bool const rootOnly ):
      eventId( eventId ), serviceID( serviceID ), contextName( contextName ), destNode( destNode ), rootOnly( rootOnly ){}
    const mace::OrderID eventId;
    const uint8_t serviceID;
    const mace::string contextName;
    const MaceAddr destNode;
    const bool rootOnly;
    static const uint8_t messageType = 255;
    static uint8_t getMsgType() { return messageType; }
    uint8_t getType() const { return NullEventMessage::getMsgType(); }

    std::string toString() const { 
      mace::string str;
      return str;
    }
    void print(std::ostream& __out) const { }
    size_t getSerializedSize() const { return 0; }
    void serialize(std::string& str) const { }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) { 
      return 0;
    }
    void sqlize(mace::LogNode* __node) const { }

    std::string serializeStr() const { 
      mace::string str;
      return str;
    }
    void deserializeStr(const std::string& __s) throw (mace::SerializationException) { }
  };
}





/**
 * \brief Base class for all context'ed Mace services
 *
 * Provides the APIs for processing any kinds of operation.
 */
class PthreadCondPointer {
public:
  pthread_cond_t* cond_ptr;
  
  PthreadCondPointer(): cond_ptr(NULL) { }
  PthreadCondPointer(pthread_cond_t* ptr): cond_ptr(ptr) { }

  ~PthreadCondPointer() {
    cond_ptr = NULL;
  } 

  PthreadCondPointer& operator=(const PthreadCondPointer& orig){
    ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
    this->cond_ptr = orig.cond_ptr;
    return *this;
  }

};

class ExternalCommClass {
private:
  pthread_mutex_t mutex;
  uint32_t contextId;
  uint32_t externalCommId;
  uint64_t createTicket;
  mace::string contextName;

public:
  ExternalCommClass(const uint32_t& contextId, const uint32_t& externalCommId): contextId(contextId), externalCommId(externalCommId), 
      createTicket(1) { 
    ASSERT( pthread_mutex_init(&mutex, NULL)==0 ); 

    std::ostringstream oss;
    oss << mace::ContextMapping::EXTERNAL_COMM_CONTEXT_NAME<< "[" << this->externalCommId << "]";
    contextName = oss.str();
  }
  void createEvent(BaseMaceService* sv, mace::Event& event, const mace::string& targetContextID, const mace::string& methodType, 
    const uint8_t& event_op_type, const uint8_t& instanceUniqueID);
};

class RemoteContextRuntimeInfo {
public:
  mace::string contextName;
  mace::map< mace::MaceAddr, uint32_t > connectionStrengths;

public:
  RemoteContextRuntimeInfo( mace::string const& context_name ): contextName(context_name) { }
  RemoteContextRuntimeInfo(): contextName("") { }
  ~RemoteContextRuntimeInfo() { connectionStrengths.clear(); }

public:
  uint32_t getConnectionStrength( mace::MaceAddr const& addr );
};

class ContextService : public BaseMaceService, public InternalMessageReceiver {
friend class mace::__ServiceStackEvent__;
friend class mace::__ScopedTransition__;
friend class mace::__ScopedRoutine__;
friend class mace::__CheckMethod__;
friend class mace::Event;
public:
  ContextService(InternalMessageSender* sender = new mace::NullInternalMessageProcessor(), bool enqueueService = true): 
      BaseMaceService(enqueueService), sender( sender ), isContextMigrating(false), isContextComming(false), nextExternalCommContextId(0), 
      now_serving_external_message_ticket(1) {
    ADD_SELECTORS("ContextService::constructor");
    pthread_mutex_init( &getContextObjectMutex, NULL );
    pthread_mutex_init( &getExecuteTicketMutex, NULL );
    pthread_mutex_init( &createNewContextMutex, NULL );
    pthread_mutex_init( &contextMappingUpdateMutex, NULL );
    pthread_mutex_init( &eventRequestBufferMutex, NULL );
    pthread_mutex_init( &migratingContextMutex, NULL);
    pthread_mutex_init( &serviceSharedDataMutex, NULL);
    pthread_mutex_init( &externalCommMutex, NULL );

    pthread_mutex_init( &releaseContextMappingUpdateMutex, NULL);
    pthread_mutex_init( &contextStructureUpdateMutex, NULL);

    pthread_mutex_init( &elasticityHandlerMutex, NULL );

    externalCommContextNumber = params::get<uint32_t>("NUM_EXTERNAL_COMMUNICATION_CONTEXT", 1 );
    clientFlag = params::get<uint8_t>("CLIENT_FLAG", 0 );

    receivedExternalMsgCount = 0;
    externalMsgOutputCount = 1000;

    contextMappingUpdatingFlag = false;
    contextStructureUpdatingFlag = false;

    nextExternalCommClassId = 0;

    contextInfoCollectFlag = params::get<uint16_t>("CONTEXT_INFO_COLLECT_FLAG", 0);

    uint32_t minThreadSize = params::get<uint32_t>("MIN_CONTEXT_THREADS", 2);
    uint32_t maxThreadSize = params::get<uint32_t>("MAX_CONTEXT_THREADS", 8);
    macedbg(1) << "minThreadSize=" << minThreadSize << ", maxThreadSize=" << maxThreadSize << Log::endl;
    contextEventDispatcher = new mace::ContextEventTP(  minThreadSize, maxThreadSize );

    // uint32_t channelThreadSize = params::get<uint32_t>("CONTROL_CHANNEL_THREADS", 2);
    // eventExecutionControlChannel = new EventExecutionControlMessageChannel( this, channelThreadSize );
    eventExecutionControlChannel = NULL;

    // uint32_t elasticityTimePeriod = params::get<uint32_t>("ELASTICITY_TIME_PERIOD", 0);
    // macedbg(1) << "elasticityTimePeriod=" << elasticityTimePeriod << Log::endl;
    // // int elasticityTimePeriod = 60;
    // mace::set<mace::string> context_types;
    // // context_types.insert("Router");
    // // context_types.insert("ALL");
    // // context_types.insert("ItemStockTable");
    // // context_types.insert("DistrictTable");
    // // context_types.insert("CustomerTable");
    // // context_types.insert("HistoryTable");
    // // context_types.insert("OrderTable");
    // // context_types.insert("OrderLineTable");
    // // context_types.insert("NewOrderTable");

    // AsyncEventReceiver* sv = static_cast<AsyncEventReceiver*>(this);
    // uint32_t initActiveServerPercent = params::get<uint32_t>("INIT_ACTIVE_SERVER_PERCENT", 100);

    // elasticityMonitor = new mace::eMonitor( sv, elasticityTimePeriod, context_types );
    // elasticityMonitor->setServerAddrs( mace::ContextMapping::serverAddrs, initActiveServerPercent );

    /* HaloPresenceService Contexts Interaction *********************************************/
    // mace::ElasticityRule wk_bl_rule( mace::ElasticityRule::INIT_PLACEMENT );
    // wk_bl_rule.relatedContextTypes.insert("GameSession");
    // wk_bl_rule.relatedContextTypes.insert("Player");

    // wk_bl_rule.behavior.behaviorType = mace::ElasticityBehavior::WORKLOAD_BALANCE;
    // wk_bl_rule.behavior.contextTypes.push_back("GameSession");
    // wk_bl_rule.behavior.contextTypes.push_back("Player");

    // eConfig.addElasticityRule( wk_bl_rule );


    // mace::ElasticityRule wk_bl_rule2( mace::ElasticityRule::INIT_PLACEMENT );
    // wk_bl_rule2.relatedContextTypes.insert("Router");
   
    // wk_bl_rule2.behavior.behaviorType = mace::ElasticityBehavior::WORKLOAD_BALANCE;
    // wk_bl_rule2.behavior.contextTypes.push_back("Router");
    
    // eConfig.addElasticityRule( wk_bl_rule2 );

    /* E-Store TPC-C *********************************************/
    // mace::ElasticityRule rule1( mace::ElasticityRule::INIT_PLACEMENT );
    // rule1.relatedContextTypes.insert("ANY");
    // rule1.behavior.behaviorType = mace::ElasticityBehavior::COLOCATE;
    
    // mace::ElasticityAndConditions and_cond1;
    // mace::ElasticityCondition cond1(mace::ElasticityCondition::REFERENCE);
    // cond1.contextTypes.insert("ANY");

    // and_cond1.conditions.push_back(cond1);
    // rule1.conditions.push_back(and_cond1);


    // mace::ElasticityRule rule2( mace::ElasticityRule::GLOBAL_RULE );
    // rule2.relatedContextTypes.insert("ANY");
    // rule2.behavior.behaviorType = mace::ElasticityBehavior::ISOLATE;
    // rule2.behavior.resourceType = mace::ElasticityBehavior::RES_NET;
    
    // mace::ElasticityAndConditions and_cond2;
    // mace::ElasticityCondition cond2(mace::ElasticityCondition::EVENT_ACCESS_COUNT_PERCENT);
    // cond2.contextTypes.insert("ANY");
    // cond2.compareType = mace::ElasticityCondition::COMPARE_ME;
    // cond2.threshold = 0.8;

    // and_cond2.conditions.push_back(cond2);
    // rule2.conditions.push_back(and_cond2);

    // eConfig.addElasticityRule(rule1);
    // (elasticityMonitor->eConfig)->addElasticityRule( rule2 );

    // mace::ElasticityRule ref_rule( mace::ElasticityRule::INIT_PLACEMENT );
    // ref_rule.behavior.behaviorType = mace::ElasticityBehavior::INIT_COLOCATE;
    // mace::ElasticityCondition ref_cond( mace::ElasticityCondition::REFERENCE, "GameSession", "Player" );
    // ref_rule.conditions.push_back( ref_cond );
    // elasticityConfig.addElasticityRule( ref_rule );

    // mace::ElasticityRule rd_rule( mace::ElasticityRule::INIT_PLACEMENT );
    // rd_rule.behavior.behaviorType = mace::ElasticityBehavior::RANDOM;
    // rd_rule.behavior.contextNames.insert("Player");
    // mace::ElasticityCondition rd_cond( mace::ElasticityCondition::CONTEXT_TYPE, "Player", "" );
    // rd_rule.conditions.push_back( rd_cond );
    // elasticityConfig.addElasticityRule( rd_rule );

    // mace::ElasticityRule pin_rule( mace::ElasticityRule::RUNTIME );
    // pin_rule.behavior.behaviorType = mace::ElasticityBehavior::PIN;
    // mace::ElasticityCondition pin_cond( mace::ElasticityCondition::CONTEXT_TYPE, "Cart", "" );
    // pin_rule.conditions.push_back( pin_cond );
    // elasticityConfig.addElasticityRule( pin_rule );

    // mace::ElasticityRule iso_rule( mace::ElasticityRule::GLOBAL_RULE );
    // iso_rule.behavior.behaviorType = mace::ElasticityBehavior::ISOLATE;
    
    // mace::ElasticityCondition iso_cond( mace::ElasticityCondition::MARKER_LATENCY_PERCENT);
    // iso_cond.compareType = mace::ElasticityCondition::COMPARE_ME;
    // iso_cond.marker = "PAGERANK";
    // iso_cond.threshold = 1.0;

    // mace::ElasticityAndConditions iso_and_conds;
    // iso_and_conds.conditions.push_back(iso_cond);

    // iso_rule.conditions.push_back( iso_and_conds );
    // (elasticityMonitor->eConfig)->addElasticityRule( iso_rule );

    /*** Max latency SLA rule ***/
    // mace::ElasticityCondition sla_cond(mace::ElasticityCondition::SLA_MAX_VALUE);
    // sla_cond.marker = "PAGERANK";
    // sla_cond.threshold = params::get<double>("PAGERANK_MAX_LATENCY", 0.3);

    // mace::ElasticityAndConditions sla_and_conds;
    // sla_and_conds.conditions.push_back(sla_cond);

    // mace::ElasticityRule sla_rule(mace::ElasticityRule::SLA_ACTOR_MARKER_RULE);
    // sla_rule.conditions.push_back(sla_and_conds);
    // sla_rule.relatedContextTypes.insert("GraphPartition");
    // eConfig.addElasticityRule(sla_rule);
  }

  ~ContextService(){

    deleteAllContextObject( );
    pthread_mutex_destroy( &getContextObjectMutex );
    pthread_mutex_destroy( &getExecuteTicketMutex );
    pthread_mutex_destroy( &createNewContextMutex );
    pthread_mutex_destroy( &contextMappingUpdateMutex );
    pthread_mutex_destroy( &eventRequestBufferMutex );
    pthread_mutex_destroy( &migratingContextMutex);
    pthread_mutex_destroy( &serviceSharedDataMutex );
    pthread_mutex_destroy( &externalCommMutex );

    pthread_mutex_destroy( &releaseContextMappingUpdateMutex );
    pthread_mutex_destroy( &contextStructureUpdateMutex );

    pthread_mutex_destroy( &elasticityHandlerMutex );
  }
protected:
  /// utility functions that can be used in user code.
  void migrateContext( mace::string const& paramid );

  /** interface for create context objects. The services are required to implement this interface.
   * 
   * @param contextTypeName the type name of the context
   * @return the context object corresponding to the type name
   *
   * */
  virtual mace::ContextBaseClass* createContextObject( mace::string const& contextTypeName ) = 0;
  // functions that are used by the code generated from perl compiler

  /**
   * Migrate context
   * If the context does not exist yet, ignore the request, but store the mapping so that when the context is created, it is created at the destination node.
   *
   * @param serviceID the numerical ID of the target service
   * @param contextID the numerical ID of the target context
   * @param destNode the destination node where the context will be migrated
   * @param rootOnly whether or not to migrate the subcontexts as well.
   * */
  void requestContextMigrationCommon(const uint8_t serviceID, const mace::string& contextID, const MaceAddr& destNode, const bool rootOnly);
  /**
   * initialize context mapping. This is supposed to be called in service constructor.
   *
   * @param servContext the mapping of physical nodes to list of contexts
   * */
  void loadContextMapping(const mace::map<mace::MaceAddr ,mace::list<mace::string > >& servContext);
  /**
   * send a message to request a ticket number for a new subevent
   * @param extra Extra object that contains the target context
   * @param event the event object
   * @param msg serialized event request 
   * */
  //void requestRouteEvent ( __asyncExtraField& extra, mace::Event& event, mace::Serializable& msg ) const;
  /**
   * set up thread stack and event environment before starting a routine/downcall/upcall 
   *
   * @param event the event object
   * */
  void __beginRemoteMethod( mace::Event const& event ) const;
  /**
   * clean up thread stack and event environment after starting a routine/downcall/upcall 
   *
   * @param src the source physical node of this call
   * @param returnValueStr the serialized return value
   * */
  void __finishRemoteMethodReturn(  mace::MaceAddr const& src, mace::string const& returnValueStr ) const;
  /**
   * For the transitions that are not implemented, it will not use the ticket, so waste the ticket.
   * */
  void wasteTicket( void ) const;
  void notifyHeadExit();
  /** push new sub event requests into the current Event structure so that when the current event commits, it knows to create these sub events.
   * @param reqObject the object that represents the even trequest
   * */
  void addEventRequest( mace::AsyncEvent_Message* reqObject);

  void executeEventBroadcastRequest( mace::AsyncEvent_Message* reqObj );
  void newBroadcastEventID();

  void addTimerEventRequest( mace::AsyncEvent_Message* reqObject){
    mace::InternalMessage msg( mace::new_event_request, reqObject, instanceUniqueID );
    mace::MaceAddr const& dest = contextMapping.getHead();

    ADD_SELECTORS("ContextService::addTimerEventRequest");
    if( isLocal( dest ) ){
      handleInternalMessages ( msg, Util::getMaceAddr() );
    }else{
      sender->sendInternalMessage( dest, msg );
    }
  }
  void addTransportEventRequest( mace::AsyncEvent_Message* reqObject, mace::MaceKey const& src);
  /**
   * defer an upcall transition that does not return value if it enters application.
   *
   * @param upcall the pointer to the upcall transition serialization message
   * */
  void deferApplicationUpcall( mace::ApplicationUpcall_Message* upcall ){
    ThreadStructure::myEvent().deferApplicationUpcalls( instanceUniqueID, upcall);
  }
  /**
   * An application upcall transition that returns a value. The upcall can not be deferred so the 
   * runtime communicate with the logical node head. The runtime waits until the previous event commits
   * and then execute the upcall transition, and returns the value.
   *
   * @param upcall the pointer to the upcall serialized object
   * */
  template< typename T>
  T returnApplicationUpcall( mace::ApplicationUpcall_Message* upcall ) const
  {
    T ret;
    if( isLocal( contextMapping.getHead() ) ){
      mace::string returnValue;
      ContextService *self = const_cast<ContextService *>( this );
      self->processLocalRPCApplicationUpcall( upcall, returnValue );

      mace::deserializeStr<T>( returnValue, &ret );
    }else{
      mace::InternalMessage im( upcall, instanceUniqueID );
      mace::ScopedContextRPC rpc;
      forwardInternalMessage( contextMapping.getHead(), im );
      rpc.get( ret );
      rpc.get( ThreadStructure::myEvent() );
    }
    return ret;
  }

  /**
   * A downcall/upcall/routine method whose context is at a remote physical node calls this API to
   * start the method and return the value. This API is a RPC.
   *
   * @param message the serialized method call.
   * @param cm 
   *
   * */
  template< typename T>
  T returnRemoteRoutine( mace::Message* const message, mace::ContextLocatorInterface const& cm ) const{
    // WC: use a template delgator, otherwise gcc complains:
    // explicit specialization in non-namespace scope 'class ContextService'
    // One workaround solution proposed in this link does not work 100%
    // http://stackoverflow.com/questions/3052579/explicit-specialization-in-non-namespace-scope
    //
    // The solution proposed (template delgator function) does not work. gcc compiles the code,
    // but either can not find the symbol when linking, or finds duplicate symbols.
    //
    // I had to use a template delgator class, which works just fine.
    mace::Delegator< T > d( sender, message, cm, this, instanceUniqueID );
    return d.getValue();
  }

private:
  /**
   * send an event. If the destination is the local physical node, push into the async dispatch queue. Otherwise send via transport service.
   * @param dest the destination physical node MaceKey
   * @param eventObject the pointer to the event object being sent
   * */
  void forwardEvent( mace::MaceAddr const& dest, mace::AsyncEvent_Message* const eventObject, const uint32_t contextID ){
    ADD_SELECTORS("ContextService::forwardEvent");
    if( isLocal( dest ) ){
#ifdef USE_HEAD_ENQUEUE_THREAD
#else
      const mace::Event& event = eventObject->getEvent();
      mace::InternalMessageID msgId( Util::getMaceAddr(), event.target_ctx_name, 0);
      mace::InternalMessage msg(eventObject, msgId, instanceUniqueID);
      handleEventMessage( eventObject, msg, Util::getMaceAddr(), contextID );
#endif
    }else{
#ifdef USE_HEAD_TRANSPORT_THREAD
      forwardHeadTransportThread( dest, eventObject );
#else
      mace::InternalMessage msg( eventObject, instanceUniqueID );
      sender->sendInternalMessage( dest, msg );
      msg.unlinkHelper();
      delete eventObject;
#endif
    }
  }
  /** get the context object pointer by its canonical name 
   * @param contextName canonical name of the context
   * @return the pointer to the context object
   * */

  void checkContextCreation( mace::string const& contextName ) const {
    ADD_SELECTORS("ContextService::checkContextCreation");
    ScopedLock sl(getContextObjectMutex);
    mace::hash_map< mace::string, mace::ContextBaseClass*, mace::SoftState >::const_iterator cpIt = ctxobjNameMap.find( contextName );
    if( cpIt == ctxobjNameMap.end() ){
      macedbg(1)<<"context "<< contextName << " is not found!" << Log::endl;
      
      pthread_cond_t cond;
      pthread_cond_init( &cond, NULL );
      contextWaitingThreads2[ contextName ].insert( &cond );
      pthread_cond_wait( &cond, &getContextObjectMutex );
      pthread_cond_destroy( &cond );
      cpIt = ctxobjNameMap.find( contextName );
      ASSERT( cpIt != ctxobjNameMap.end() );
    }

    return;
  }

  bool handleMessageForNullContext(MaceAddr const& src, mace::InternalMessage const& message) {
    ADD_SELECTORS("ContextService::handleMessageForNullContext");
    const mace::string& ctxName = message.getTargetContextName();
    ScopedLock sl(getContextObjectMutex);
    macedbg(1) << "Check context: " << ctxName << Log::endl;
    mace::hash_map< mace::string, mace::ContextBaseClass*, mace::SoftState >::const_iterator cpIt = ctxobjNameMap.find( ctxName );
    if( cpIt == ctxobjNameMap.end() ) {
      const ContextMapping& ctxMapping = contextMapping.getLatestContextMapping();
      if( !ContextMapping::hasContext2(ctxMapping, ctxName) || ContextMapping::getNodeByContext(ctxMapping, ctxName) == Util::getMaceAddr() ) {
        macedbg(1) << "Context("<< ctxName <<") is not found!" << Log::endl;

        pthread_cond_t cond;
        pthread_cond_init( &cond, NULL );
        contextWaitingThreads2[ ctxName ].insert( &cond );
        pthread_cond_wait( &cond, &getContextObjectMutex );
        pthread_cond_destroy( &cond );
        cpIt = ctxobjNameMap.find( ctxName );
        ASSERT( cpIt != ctxobjNameMap.end() );
        return false;
      } else {
        sl.unlock();
        macedbg(1) << "Forward the message for context("<< ctxName <<") to new node!" << Log::endl;
        forwardInternalMessageToNewAddress(src, message);
        return true;
      }
    } else {
      return false;
    }

  }

  void scaleTo(const uint32_t& n) {
    mace::map<mace::string, mace::MaceAddr> newMapping = contextMapping.scaleTo(n);

    for( mace::map<mace::string, mace::MaceAddr>::iterator iter=newMapping.begin(); iter!=newMapping.end(); iter++ ){
      MaceAddr dest = iter->second;
      mace::string contextName = iter->first;
      uint8_t service = 0;
      requestContextMigrationCommon(service, contextName, dest , true);
    }
  }
  
  void waitForContextObjectCreate( const mace::string& contextName, ScopedLock& release_lock );

  void waitForExecuteTicket( const mace::OrderID& eventId ) const {
    ScopedLock sl(getExecuteTicketMutex);
    pthread_cond_t cond;
    pthread_cond_init( &cond, NULL );
    contextWaitingThreads3[ eventId ].insert( &cond );
    pthread_cond_wait( &cond, &getExecuteTicketMutex );
    pthread_cond_destroy( &cond );
  }

  void waitForContextMappingUpdate( const uint64_t expectVer, ScopedLock& release_lock) const {
    ADD_SELECTORS("ContextService::waitForContextMappingUpdate");
    pthread_cond_t cond;
    pthread_cond_init( &cond, NULL );
    contextMappingUpdateWaitingThreads[expectVer].insert(&cond);
    release_lock.unlock();
    macedbg(1) << "Wait for ContextMapping of " << expectVer << Log::endl;
    //pthread_cond_wait( &cond, &contextMappingUpdateMutex );
    pthread_cond_wait( &cond, &releaseContextMappingUpdateMutex );
    macedbg(1) << "Wakeup for ContextMapping of " << expectVer << Log::endl;
    pthread_cond_destroy( &cond );
  }

  /** associate context object pointer to the numerical id and canonical name. Used by services to implement createContext() interface
   *
   * @param obj the pointer to the context object
   * @param contextID the numerical context ID
   * @param contextName the canonical context name
   * */
  void setContextObject( mace::ContextBaseClass* obj, uint32_t const contextID, mace::string const& contextName ){
    ADD_SELECTORS("setContextObject");
    macedbg(1) << "Try to set context=" << contextName << Log::endl;
    ASSERT( ctxobjNameMap.find( contextName ) == ctxobjNameMap.end() );
    //ASSERT( ctxobjIDMap.find( contextID ) == ctxobjIDMap.end() );
    ASSERT( ctxobjIDMap.size() <= contextID || ctxobjIDMap[ contextID ] == NULL );

    ctxobjNameMap[ contextName ] = obj;

    if( ctxobjIDMap.size() <= contextID ){
      ctxobjIDMap.resize( contextID+1 );
    }
    ctxobjIDMap[ contextID ] = obj;
  }
  /**
   * called to resume the execution of an event that makes an upcall transition to the application
   * @param src the source physical node of the upcall
   * @param returnValueStr serialized return value
   * */
  void __appUpcallReturn( mace::MaceKey const& src, mace::string const& returnValueStr) const;
  /**
   * a downcall/upcall transition when it first enters a context and that it's an internal transition that does not generate  a new context,
   * must make sure it downgrades those contexts that it will not access
   * */
  void enterInnerService (mace::string const& targetContextID ) const;
  /// acquire context locks
  void acquireContextLocks(uint32_t const  targetContextID, mace::vector<uint32_t> const & snapshotContextIDs) const ;
  /// initialize events
  //mace::MaceAddr const&  asyncHead( mace::Event& event,  mace::__asyncExtraField const& extra, int8_t const eventType, uint32_t& contextId);
  

  /// internal message handler
  void handleInternalMessages( mace::InternalMessage const& message, MaceAddr const& src, uint64_t size=0  );

  void addTimerEvent( mace::AsyncEvent_Message* reqObject);
  void snapshot(const uint64_t& ver) const {} // no op
  void snapshotRelease(const uint64_t& ver) const {} // no op
  
  void createEventFromRemoteContext(const mace::string& ctxName);

  /**
   * wake up the threads that waits for the context to be created.
   *
   * @param contextID the numerical ID of the context
   * */
  void wakeupWaitingThreads(uint64_t contextID) const{
    std::map< uint64_t, std::set< pthread_cond_t* > >::iterator condSetIt = contextWaitingThreads.find( contextID );
    if( condSetIt != contextWaitingThreads.end() ){
      /*for( std::set< pthread_cond_t* >::iterator condIt = condSetIt->second.begin(); condIt != condSetIt->second.end(); condIt++ ){
        pthread_cond_signal( *condIt );
      }*/
      std::for_each( condSetIt->second.begin(), condSetIt->second.end(), pthread_cond_signal );
      contextWaitingThreads.erase( condSetIt );
    }
  }
  /**
   * wake up the threads that waits for the context to be created.
   *
   * @param contextName the canonical name of the context
   * */
  void wakeupWaitingThreads(mace::string const& contextName) const{
    ADD_SELECTORS("ContextService::wakeupWaitingThreads");
    ScopedLock sl(releaseContextMappingUpdateMutex);
    macedbg(1) << "Wakeup waiting threads for context: " << contextName << Log::endl;
    std::map< mace::string, std::set< pthread_cond_t* > >::iterator condSetIt = contextWaitingThreads2.find( contextName );
    if( condSetIt != contextWaitingThreads2.end() ){
      std::for_each( condSetIt->second.begin(), condSetIt->second.end(), pthread_cond_signal );
      contextWaitingThreads2.erase( condSetIt );
    }
  }

  void wakeupWaitingThreads(mace::OrderID const& eventId) const{
    ADD_SELECTORS("ContextService::wakeupWaitingThreads#3");
    std::map< mace::OrderID, std::set< pthread_cond_t* > >::iterator condSetIt = contextWaitingThreads3.find( eventId );
    if( condSetIt != contextWaitingThreads3.end() ){
      std::for_each( condSetIt->second.begin(), condSetIt->second.end(), pthread_cond_signal );
      contextWaitingThreads3.erase( condSetIt );
    }
  }

  void wakeupContextMappingUpdateThreads( const uint64_t ver ) {
    ADD_SELECTORS("ContextService::wakeupContextMappingUpdateThreads");
    std::vector<uint64_t> toDelete;
    
    ScopedLock sl(releaseContextMappingUpdateMutex);
    // ScopedLock sl(contextMappingUpdateMutex);
    macedbg(1) << "To wakeup threads waiting for (or less) version=" << ver << Log::endl;
    std::map< uint64_t, std::set< pthread_cond_t* > >::iterator condSetIt = contextMappingUpdateWaitingThreads.begin();
    for(; condSetIt != contextMappingUpdateWaitingThreads.end(); condSetIt ++ ) {
      if( condSetIt->first <= ver ) {
        macedbg(1) << "Wakeup "<< condSetIt->second.size()<<" threads waiting for version " << condSetIt->first << Log::endl; 
        std::for_each(condSetIt->second.begin(), condSetIt->second.end(), pthread_cond_signal);
        toDelete.push_back(condSetIt->first);
      }
    }

    if( !toDelete.empty() ) {
      for( uint32_t i=0; i<toDelete.size(); i++ ) {
        contextMappingUpdateWaitingThreads.erase(toDelete[i]);
      }
    }
  }

  // void wakeupContextStructureUpdateThreads() const;

  bool handleEventMessage( mace::AsyncEvent_Message* m, mace::InternalMessage const& msg, mace::MaceAddr const& src, 
    const uint32_t targetContextID=0, const uint64_t msg_size=0 );
  void handleRoutineMessage( mace::Routine_Message* m, mace::MaceAddr const& source, mace::InternalMessage const& message );
  /**
   * initialize an event and send it to the start context 
   *
   * @param msgObject the pointer to the event object 
   * */
  void createEvent(mace::AsyncEvent_Message* msgObject){
    if( mace::Event::isExit ){
      wasteTicket();
      return;
    }
    //mace::Event& event = msgObject->getEvent();
    //mace::__asyncExtraField & extra = msgObject->getExtra();
    uint32_t contextID;
    const MaceAddr& destAddr = asyncHead( msgObject, contextID );
    forwardEvent( destAddr, msgObject, contextID );
  }

  void processRPCApplicationUpcall( mace::ApplicationUpcall_Message* msg, MaceAddr const& src);
  void processLocalRPCApplicationUpcall( mace::ApplicationUpcall_Message* msg, mace::string& returnValue );

  void __beginTransition( const uint32_t targetContextID, mace::vector<uint32_t> const& snapshotContextIDs, bool isRelease, 
    bool newExecuteTicket  ) const;
  void __beginMethod( const uint32_t targetContextID, mace::vector<uint32_t> const& snapshotContextIDs, bool isRelease, bool newExecuteTicket ) const;

  void __finishTransition(mace::ContextBaseClass* oldContext) const;
  void __finishMethod(mace::ContextBaseClass* oldContext) const;
  void __finishBroadcastTransition() const;
  
  void acquireContextLocksCommon(uint32_t const targetContextID, mace::vector<uint32_t> const& snapshotContextIDs, mace::map< MaceAddr, mace::vector< uint32_t > >& ancestorContextNodes) const;
  void downgradeContext( mace::string const& contextName ); // WC: not used?
  void nullEventHead( void *p );
  void doDeleteContext( mace::string const& contextName  );
  void deleteContext( mace::string const& contextName );
  void copyContextData(mace::ContextBaseClass* thisContext, mace::string& s ) const;
  void eraseContextData(mace::ContextBaseClass* thisContext);
  void downgradeEventContext( );
  void deleteAllContextObject( ) {
    ADD_SELECTORS("ContextService::deleteAllContextObject");
    ScopedLock sl(getContextObjectMutex);

    /*for( mace::hash_map< uint32_t, mace::ContextBaseClass*, mace::SoftState >::iterator cpIt = ctxobjIDMap.begin(); cpIt != ctxobjIDMap.end(); cpIt++ ){
      delete cpIt->second;
    }*/
    for( mace::vector< mace::ContextBaseClass*, mace::SoftState >::size_type i=1; i < ctxobjIDMap.size(); i++ ){
      delete ctxobjIDMap[ i ];
    }

    ctxobjIDMap.clear();

    ctxobjNameMap.clear();
  }
  void sendAsyncSnapshot( __asyncExtraField const& extra, mace::string const& thisContextID, mace::ContextBaseClass* const& thisContext );
  
  void getContextSnapshot( mace::vector<uint32_t> const& snapshotContextID ) const {
    uint32_t nsnapshot = snapshotContextID.size();
    uint32_t receivedSnapshots = 0;
    while( receivedSnapshots < nsnapshot ){
      //uint32_t recvContextID;
      //mace::string recvContextSnapshot;
      //mace::ContextSnapshot::receive(recvContextID, recvContextSnapshot);
      //mace::ContextBaseClass * contextObject = getContextObjByID( recvContextID );
      //mace::deserialize( recvContextSnapshot, contextObject);
      receivedSnapshots++;
    }

  }
  
    
  void send__event_commit( MaceAddr const& destNode, mace::Event const& event, mace::string const& ctxName );
  void const_send__event_commit( MaceAddr const& dest, mace::Event const& event, mace::string const& ctxName ) const;
  void send__event_snapshot( MaceAddr const& dest, mace::Event const& event, mace::string const& targetContextID, mace::string const& snapshotContextID, mace::string const& snapshot );
  void send__event_create_response( MaceAddr const& dest, mace::Event const& event, uint32_t const& counter, MaceAddr const& targetAddress);
  void const_send__event_create( MaceAddr const& dest, __asyncExtraField const& extra, uint64_t const& counter, uint32_t const& ctxId ) const;
  void send__event_downgrade_context( MaceAddr const& dest, uint32_t const contextID, mace::OrderID const& eventID, bool const isresponse );

  //bsang messages sending
	void send__event_AllocateContextObjReqMsg(mace::string const& ctx_name, mace::OrderID const& eventId,
    const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, const mace::map< mace::string, uint64_t>& vers );
  void send__event_contextMappingUpdateReqMsg(const uint64_t expectVer);
  void send__event_contextStructureUpdateReqMsg(const uint64_t expectVer);
	void send__event_AllocateContextObjectResponseMsg(mace::string const& ctx_name, mace::OrderID const& eventId, bool const& isCreateContextEvent);
  void send__event_ContextStructureUpdate( mace::set< mace::pair<mace::string, mace::string> > const& ownerships, const uint64_t& version, const mace::MaceAddr& src);
  // void send__event_AddNewOwnership( mace::set< mace::pair<mace::string, mace::string> > const& ownerships );
  void send__event_contextMappingUpdateSuggest( const mace::MaceAddr& dest, const uint64_t ver ) const;
  void send__event_migrate_context(mace::MaceAddr const& newNode, mace::string const& contextName, uint64_t const delay );
  void send__event_migrate_param(mace::string const& paramid );
  void send__event_routine_return( mace::MaceAddr const& src, mace::string const& returnValueStr ) const;
  void send__event_RemoveContextObject( mace::OrderID const& eventID, mace::ContextMapping const& ctxmapCopy, MaceAddr const& dest, uint32_t contextID );
  void send__event_delete_context( mace::string const& contextName );

  void send__event_asyncEvent( mace::MaceAddr const& dest, mace::AsyncEvent_Message* const eventObject, mace::string const& ctxName );

  void send__event_externalCommControlMsg( mace::MaceAddr const& dest, const uint8_t& control_type, const uint32_t& externalCommId, 
    const uint32_t& externalCommContextId);

  void send__event_commitContextsMsg( MaceAddr const& destNode, mace::vector< mace::string > const& cctxNames, 
    mace::string const& src_contextName, mace::OrderID const& eventId) const;
  

  void remoteAllocateGlobalContext( mace::string const& globalContextID, std::pair< mace::MaceAddr, uint32_t > const& newMappingReturn, const mace::ContextMapping* ctxmapCopy ){
    mace::map< uint32_t, mace::string > contextSet;
    contextSet[ newMappingReturn.second ] =  globalContextID ;

    mace::vector< mace::pair<mace::string, mace::string> > ownershipPairs;
    mace::map< mace::string, uint64_t > vers;
    mace::InternalMessage msg( mace::AllocateContextObject, newMappingReturn.first, contextSet, 
      ThreadStructure::myEventID(), *ctxmapCopy, 0, contextMapping.getCurrentVersion(), ownershipPairs, vers );
    sender->sendInternalMessage( newMappingReturn.first ,  msg );
  }

  void trytoCreateNewContextObject( mace::string const& ctx_name, mace::OrderID const& eventId, 
    const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, const mace::map< mace::string, uint64_t>& vers );

  void notifyNewEvent( mace::Event & event,  const uint8_t serviceID ) ;
  void notifyNewContext( mace::Event & event, const uint8_t serviceID );
 
  void handle__event_AllocateContextObject( MaceAddr const& src, MaceAddr const& destNode, 
      mace::map< uint32_t, mace::string > const& ContextID, mace::OrderID const& eventID, mace::ContextMapping const& ctxMapping, 
      int8_t const& eventType, const uint64_t& current_version, 
      const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, 
      const mace::map<mace::string, uint64_t>& vers );

	void handle__event_AllocateContextObjReq(mace::string const& ctxName, mace::OrderID const& eventId, 
    const mace::MaceAddr& src, const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, 
    const mace::map< mace::string, uint64_t>& vers);

	void handle__event_AllocateContextObjectResponse(mace::string const& ctx_name, mace::OrderID const& eventId, bool const& isCreateContextEvent);
  void handle__event_UpdateContextMapping(mace::ContextMapping const& contextMapping, mace::string const& contextName);
	
	void handle__event_ContextMigrationRequest( MaceAddr const& src, MaceAddr const& dest, mace::Event const& event, uint64_t const& prevContextMapVersion, 
    mace::set< uint32_t > const& migrateContextIds, mace::ContextMapping const& ctxMapping );
  
  void handle__event_create( MaceAddr const& src, __asyncExtraField const& extra, uint64_t const& counter, uint32_t const& ctxID );
  void handle__event_commit( mace::Event& event, mace::InternalMessage const& message, mace::MaceAddr const& src ) const;
  
  void handle__event_commit_contexts( mace::vector< mace::string > const& ctxNames, mace::string const& targetContextName,
      mace::OrderID const& eventId, mace::MaceAddr const& src, mace::MaceAddr const& orig_src );
  void handle__event_create_response( mace::Event const& event, uint32_t const& counter, MaceAddr const& targetAddress);
  void handle__event_enter_context( mace::Event const& event, mace::vector< uint32_t > const& contextIDs );
  void handle__event_exit_committed( );
  void handle__event_create_head( __asyncExtraField const& extra, uint64_t const& counter, MaceAddr const& src);
  void handle__event_snapshot( mace::Event const& event, mace::string const& ctxID, mace::string const& snapshotContextID, mace::string const& snapshot);
  void handle__event_downgrade_context( uint32_t const& contextID, mace::OrderID const& eventID, bool const& isresponse );
  void handle__event_routine_return( mace::string const& returnValue, mace::Event const& event);
  void handle__event_appupcall_return( mace::string const& returnValue, mace::Event const& event);
  void handle__event_new_head_ready(  MaceAddr const& src);
  void handle__event_evict( MaceAddr const& src );
  void handle__event_migrate_context( mace::MaceAddr const& newNode, mace::string const& contextName, uint64_t const delay );
  void handle__event_migrate_param( mace::string const& paramid );
  void handle__event_RemoveContextObject( mace::OrderID const eventID, mace::ContextMapping const& ctxmapCopy, MaceAddr const& dest, uint32_t const & contextID );
  void handle__event_delete_context( mace::string const& contextName );
  
  void handle__event_CommitDone( mace::string const& target_ctx_name, mace::OrderID const& eventId, mace::set<mace::string> const& coaccess_contexts );
  void handle__event_contextmapping_update_req( const uint64_t& expectVer, const mace::MaceAddr& src);

  // Ownership operation
  void handle__ownership_modifyOwnership( const mace::string& dominator, const mace::EventOperationInfo& eop, const mace::string& ctxName, 
    const mace::vector<mace::EventOperationInfo>& ownershipOpInfos );
  
  void handle__ownership_updateOwnership( const mace::vector< mace::pair<mace::string, mace::string> >& ownerships, 
    const mace::map< mace::string, uint64_t>& vers);

  void handle__ownership_contextDAGRequest( const mace::MaceAddr& srcAddr, const mace::string& src_context, 
    const mace::set<mace::string>& contexts ) const;

  void handle__ownership_contextDAGReply( const mace::string& dom_context, const mace::set<mace::string>& contexts, 
    const mace::vector<mace::pair<mace::string, mace::string > >& ownershipPairs, const mace::map<mace::string, uint64_t>& vers );

  void handle__ownership_updateDominators( const mace::vector< mace::pair<mace::string, mace::string> >& ownerships, 
    const mace::map<mace::string, uint64_t>& vers, const mace::string& src_context, const mace::set<mace::string>& update_doms, 
    const mace::vector<mace::EventOperationInfo>& eops );

  void handle__ownership_updateReply( const mace::string& dest_contextName, const mace::set<mace::string>& src_contextNames,
    const mace::vector<mace::EventOperationInfo>& eops );
  
  void handle__event_contextmapping_update_suggest( const uint64_t ver);
  void handle__event_ExternalCommControl( const mace::ExternalCommControl_Message* msg);
  
  void handle__event_NewContextCreation( const mace::string& create_ctx_name, const mace::string& created_ctx_type, const mace::EventOperationInfo& op_info);


  static void waitExit(){
    ScopedLock sl( waitExitMutex );
    pthread_cond_wait( &waitExitCond, &waitExitMutex );
  }

  static void proceedExit(){
    ScopedLock sl( waitExitMutex );
    pthread_cond_signal( &waitExitCond );
  }

	mace::string getStartContextName(const mace::string& ctx_name);
  mace::vector<mace::string> getAccessContexts(const mace::string& ctx_name);
  
  
// protected:
public:
  mace::ContextMapping contextMapping;
private:
  mutable pthread_mutex_t getContextObjectMutex;
  mutable pthread_mutex_t createNewContextMutex;
  mutable pthread_mutex_t getExecuteTicketMutex;
  mutable pthread_mutex_t contextMappingUpdateMutex;
  
  mutable pthread_mutex_t releaseContextMappingUpdateMutex;
  mutable pthread_mutex_t contextStructureUpdateMutex;

  mutable pthread_mutex_t migratingContextMutex;

  mutable pthread_mutex_t serviceSharedDataMutex;

  mutable InternalMessageSender* sender;
  //mace::hash_map< uint32_t, mace::ContextBaseClass*, mace::SoftState > ctxobjIDMap;
  // TODO: use std::auto_ptr
  mace::vector< mace::ContextBaseClass*, mace::SoftState > ctxobjIDMap;
  mace::hash_map< mace::string, mace::ContextBaseClass*, mace::SoftState > ctxobjNameMap;
  static std::map< uint64_t, std::set< pthread_cond_t* > > contextWaitingThreads;
  static std::map< mace::string, std::set< pthread_cond_t* > > contextWaitingThreads2;
  static std::map< mace::OrderID, std::set< pthread_cond_t* > > contextWaitingThreads3;
  static std::map< uint64_t, std::set< pthread_cond_t* > > contextMappingUpdateWaitingThreads;
  static std::map< uint64_t, std::vector< PthreadCondPointer > > contextStructureUpdateWaitingThreads;
  static std::map< uint64_t, pthread_cond_t* > externalMsgWaitingThread;
  static pthread_mutex_t waitExitMutex;
  static pthread_cond_t waitExitCond;
  mutable pthread_mutex_t eventRequestBufferMutex;
  mace::ContextEventRecord contextEventRecord;
  mutable std::map< uint32_t, std::pair<mace::string*, mace::string > > unfinishedEventRequest;
  // TODO: make ContextService a templated class and the template parameter of the trait class uses the template parameter of ContextService
  mace::Locality_trait< mace::DistributedLogicalNode > nodeLocality;

  //bsang variables
  mace::ContextEventTP *contextEventDispatcher;
  EventExecutionControlMessageChannel* eventExecutionControlChannel;

  mace::map<mace::string, uint32_t> contextIds;
  mace::set<mace::string> existingContextNames;

  bool contextMappingUpdatingFlag;
  bool contextStructureUpdatingFlag;

  // For migration
  std::map<mace::string, std::set<mace::InternalMessage*> > holdingMessageForMigration;
  mace::set< mace::string > migratingContextNames;
  bool isContextMigrating;

  std::map< mace::string, std::set<mace::InternalMessage*> > holdingMessageForCommingContexts;
  mace::set< mace::string > commingContexts;
  mace::map< uint32_t, mace::string > commingContextsMap;
  bool isContextComming;

  mace::map< mace::string, mace::vector<mace::MaceAddr> > processContextCreate;

  typedef std::map< uint32_t, std::map< mace::OrderID, pthread_cond_t> > SyncWaitCondType;
  SyncWaitCondType syncEventWaitConds;

  // For external communication
  uint32_t externalCommContextNumber;
  std::map<uint32_t, ExternalCommClass*> externalCommClassMap;
  mace::vector<uint32_t> externalCommClasses;
  uint32_t nextExternalCommClassId;
  
  mace::map<mace::string, mace::MaceAddr> headClientExCommContextMap;
  
  mutable pthread_mutex_t externalCommMutex;
  uint32_t nextExternalCommContextId;

  uint64_t now_serving_external_message_ticket;
  std::map<uint64_t, pthread_cond_t*> externalMsgWaitThread;

  // Elasticity variables
  mutable pthread_mutex_t elasticityHandlerMutex;
  std::map<mace::string, RemoteContextRuntimeInfo> remoteContextRuntimeInfos;
  std::map<uint32_t, pthread_cond_t*> elasticityHandlerConds;
  mace::eMonitor* elasticityMonitor;

  // For client optimization
  uint8_t clientFlag;

  // For debug
  uint64_t receivedExternalMsgCount;
  uint64_t externalMsgOutputCount;

public:
  uint16_t contextInfoCollectFlag;
  ContextStructure contextStructure;
  mace::ElasticityConfiguration eConfig;
  
  void executeRoutineGrap( mace::Routine_Message* routineobject, mace::MaceAddr const& source );
  void executeStartEvent( mace::AsyncEvent_Message* eventObject );
  void executeCommitContext( mace::commit_single_context_Message* const msg );
  void executeBroadcastCommitContext( mace::commit_single_context_Message* const msg );

  void __beginCommitContext( const uint32_t targetContextID ) const;
  void __finishCommitContext(mace::ContextBaseClass* oldContext) const;

  void forwardHeadTransportThread( mace::MaceAddr const& dest, mace::AsyncEvent_Message* const eventObject );

  void createGlobalContextObject();

  void checkAndUpdateContextMapping(const uint64_t contextMappingVer);
  void checkAndUpdateContextStructure(const uint64_t contextStructureVer);
  void handle__event_MigrateContext( void *p );
  void downgradeBroadcastEvent( mace::string const& ctxName, mace::OrderID const& eventId, mace::OrderID const& bEventId, 
    mace::map<mace::string, mace::set<mace::string> > const& cpRelations, mace::set<mace::string> const& targetContextNames ) const;

  void send__event_commit_interface( MaceAddr const& destNode, mace::Event const& event, mace::string const& ctxName ){
    send__event_commit(destNode, event, ctxName);
  }

  void send__event_CommitDoneMsg(mace::string const& create_ctx_name, mace::string const& target_ctx_name, mace::OrderID const& eventId, 
    mace::set<mace::string> const& coaccess_contexts);

  void send__ownership_ownershipOperations( const mace::string& dominator, const mace::EventOperationInfo& eop, const mace::string& contextName, 
    const mace::vector<mace::EventOperationInfo>& ownershipOpInfos) const;
  
  void send__event_updateOwnership(const mace::MaceAddr& destAddr, mace::set<mace::string> const& ctxNames, 
    mace::vector< mace::pair<mace::string, mace::string> > const& ownerships, const uint64_t ver) const;

  void send__ownership_updateOwnershipReply( const mace::string& destContext, const mace::set<mace::string>& srcContexts,
    const mace::vector<mace::EventOperationInfo>& reply_eops ) const;
  
  void send__event_modifyOwnershipReply( mace::MaceAddr const& destAddr, mace::string const& ctxName, mace::EventOperationInfo const& eop ) const;
  
  void send__ownership_updateOwnershipAndDominators(const mace::MaceAddr& destAddr, const mace::string& src_contextName,
    const mace::set<mace::string>& update_doms, const mace::vector<mace::EventOperationInfo>& forward_eops,
    const mace::vector< mace::pair<mace::string, mace::string> >& ownerships, const mace::map<mace::string, uint64_t>& vers ) const;
  
  void send__ownership_contextDAGRequest( const mace::string& src_contextName, const mace::MaceAddr& destAddr, 
    const mace::set<mace::string>& context_set ) const;

  void send__ownership_contextDAGReply( const mace::MaceAddr& destAddr, const mace::string& dest_contextName,
    const mace::set<mace::string>& contextSet, const mace::vector< mace::pair<mace::string, mace::string> >& ownerships,
    const mace::map<mace::string, uint64_t>& versions ) const;
  
  void send__event_createNewContext( mace::string const& src_contextName, mace::string const& contextTypeName, 
    mace::EventOperationInfo const& eventOpInfo ) const;
  void send__event_createNewContextReply( mace::string const& dest_contextName, mace::EventOperationInfo const& eventOpInfo, const uint32_t& newContextId) const;
  void send__event_enqueueSubEvent( mace::EventOperationInfo const& eventOpInfo, mace::string const& dest_contextName, mace::string const& src_contextName,
    mace::EventRequestWrapper const& eventRequest) const;
  void send__event_enqueueSubEventReply( mace::EventOperationInfo const& eventOpInfo, mace::string const& dest_contextName, 
    mace::string const& src_contextName ) const;
  void send__event_enqueueExternalMessage( mace::EventOperationInfo const& eventOpInfo, mace::string const& dest_contextName, mace::string const& src_contextName,
    mace::EventMessageRecord const& msg) const;
  void send__event_enqueueExternalMessageReply( mace::EventOperationInfo const& eventOpInfo, mace::string const& dest_contextName, 
    mace::string const& src_contextName ) const;
  
  mace::set<mace::string> send__event_requireEventExecutePermission( mace::string const& dominator, mace::EventOperationInfo const& eventOpInfo );
  void send__event_enqueueLockRequests( mace::string const& dominator, mace::string const& requireContextName, 
    mace::vector<mace::EventOperationInfo> const& eventOpInfos);
  void send__event_getReadyToCommit( mace::OrderID const& eventId, mace::string const& notifyContext, mace::string const& src_contextName ) const;
  void send__event_notifyReadyToCommit( const MaceAddr& destAddr, const mace::OrderID& eventId, const mace::string& src_contextName,
    mace::vector<mace::string> const& notifyContexts, mace::vector<mace::string> const& executedContexts) const;
  void send__event_EventOperationInfo( const uint8_t type, mace::string const& dest_contextName, mace::string const& src_contextName, 
    mace::EventOperationInfo const& opInfo, mace::OrderID const& eventId ) const;

  
  void send__event_enqueueOwnershipOps( mace::EventOperationInfo const& eventOpInfo, mace::string const& dest_contextName, 
    mace::string const& src_contextName, mace::vector<mace::EventOperationInfo> const& ownershipOpInfos) const;
  void send__event_enqueueOwnershipOpsReply(mace::EventOperationInfo const& eventOpInfo, mace::string const& dest_contextName, 
    mace::string const& src_contextName) const;

  void send__event_AllocateContextObjectMsg( mace::OrderID const& eventID, mace::ContextMapping const& ctxmapCopy, 
    MaceAddr const newHead, mace::map< uint32_t, mace::string > const& contextSet, int8_t const eventType, 
    const uint64_t& version, const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, 
    const mace::map<mace::string, uint64_t>& vers );

  mace::ContextMapping& getContextMapping() {
    return contextMapping;
  } 

  const mace::ContextMapping& getSnapshot() {
    return contextMapping.getSnapshot();
  }

  const mace::ContextMapping& getLatestContextMapping() {
    return contextMapping.getLatestContextMapping();
  }

  void setContextMappingUpdateFlag(bool flag){
    contextMapping.setUpdateFlag(flag);
  }

  void handleInternalMessagesWrapper( void* __param  ){
    ADD_SELECTORS("ContextService::handleInternalMessagesWrapper");
    mace::InternalMessage* __msg = static_cast<mace::InternalMessage* >(__param);
    __msg->getMessageType();
    handleInternalMessages( *__msg, Util::getMaceAddr() );
    delete __msg;
  }

  mace::MaceAddr const& asyncHead( mace::Message* msg, uint32_t& contextId);
  mace::MaceAddr const& asyncHead( mace::OrderID& eventId, mace::string const& targetContextName, int8_t const eventType, uint32_t& contextId);
  mace::MaceAddr const& asyncHead( const mace::OrderID& eventId, const mace::string& targetContextName, 
    const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, const mace::map< mace::string, uint64_t>& vers );
  void broadcastHead( mace::Message* msg );

  mace::MaceAddr getExternalCommContextAddr(const MaceKey& src, const mace::string& identifier);
  void checkAndWaitExternalMessageHandle();
  void signalExternalMessageThread();
  uint32_t getExternalCommContextID(const uint32_t& externalCommId) { return 100000+externalCommId; }

  void enqueueReadyCreateEventQueue( const HeadEventDispatch::HeadEvent& event ) { contextEventDispatcher->enqueueReadyCreateEvent(event); }
  void enqueueReadyExecuteEventQueue( const mace::ContextEvent& event ) { contextEventDispatcher->enqueueReadyExecuteEvent(event); }
  void enqueueReadyCommitEventQueue( const mace::ContextCommitEvent& event ) { contextEventDispatcher->enqueueReadyCommitEvent(event); }

  void signalSharedCreateThread() { contextEventDispatcher->signalSharedCreateThread(); }
  void signalSharedExecuteThread() { contextEventDispatcher->signalSharedExecuteThread(); }
  void signalSharedCommitThread() { contextEventDispatcher->signalSharedCommitThread(); }

  // Ownership methods
  void createNewOwnership(mace::string const& pContextName, mace::string const& cContextName );
  void removeOwnership(mace::string const& pContextName, mace::string const& cContextName);
  void modifyOwnership( const uint8_t opType, mace::string const& parentContextName, mace::string const& childContextName );

  uint32_t createNewContext(mace::string const& contextTypeName);
  mace::string getParentContextName(mace::string const& ctxName) const { return contextStructure.getParentContextName(ctxName); }

  void addNewContextName( const mace::string& ctx_name );

  // Locking methods
  void send__event_replyEventExecutePermission( mace::string const& dest_contextName, mace::string const& src_contextName, 
    mace::OrderID const& eventId, mace::vector<mace::string> const& permittedContextNames, 
    mace::vector<mace::EventOperationInfo> const& eventOpInfos ) ;
  void send__event_releaseContext( mace::string const& dominator, mace::OrderID const& eventId, 
    mace::string const& releaseContext, mace::vector<mace::EventOperationInfo> const& localLockRequests,
    mace::vector< mace::string > const& lockedContexts, const mace::string& src_contextName );
  void send__event_unlockContext( mace::string const& destContext, mace::EventOperationInfo const& eop, 
    mace::vector<mace::EventOperationInfo> const& localLockRequests, mace::vector<mace::string> const& lockedContexts, 
    const mace::string& src_contextName );
  void send__event_releaseLockOnContext( mace::string const& dest_contextName, mace::string const& src_contextName, mace::OrderID const& eventId);
  
  ExternalCommClass* getExternalCommClass();
  bool checkParentChildRelation(mace::string const& p, mace::string const& c) const { return contextStructure.checkParentChildRelation(p, c); }

  // void getUpdatedContextStructure(const uint64_t expectVer);

  mace::ContextBaseClass* getContextObjByID( uint32_t const contextID ) const{
    ADD_SELECTORS("ContextService::getContextObjByID");
    //macedbg(1) << "ContextId = " << contextID << Log::endl;
    ScopedLock sl(getContextObjectMutex);
    //mace::hash_map< uint32_t, mace::ContextBaseClass*, mace::SoftState >::const_iterator cpIt = ctxobjIDMap.find( contextID );
    //if( cpIt == ctxobjIDMap.end() ){
    if( contextID >= ctxobjIDMap.size() || ctxobjIDMap[ contextID ] == NULL ){
      return NULL;
      /*
      macedbg(1)<<"context ID "<< contextID << " not found! wait ...";
      pthread_cond_t cond;
      pthread_cond_init( &cond, NULL );
      contextWaitingThreads[ contextID ].insert( &cond );
      pthread_cond_wait( &cond, &getContextObjectMutex );
      pthread_cond_destroy( &cond );
      */
    }
    //return cpIt->second;
    return ctxobjIDMap[ contextID ];
  }

   bool isLocal( mace::MaceAddr const& dest ) const{
    return nodeLocality.isLocal( dest );
  }

  bool addNewCreateContext(mace::string const& ctx_name, const mace::MaceAddr& src);
  bool hasNewCreateContext(const mace::string& ctx_name);
  void notifyContextMappingUpdate(mace::string const& ctx_name); 
  void notifyContextMappingUpdate(mace::string const& ctx_name, const mace::MaceAddr& src);

  // void send__event_AllocateContextObjectMsg_Interface( mace::OrderID const& eventID, mace::ContextMapping const& ctxmapCopy, 
  //     MaceAddr const newHead, mace::map< uint32_t, mace::string > const& contextSet, int8_t const eventType, const uint64_t& version,
  //      ){
  //   send__event_AllocateContextObjectMsg( eventID, ctxmapCopy, newHead, contextSet, eventType, version );
  // }

  void createContextEntry( const mace::string& contextName, const uint32_t contextId, const mace::OrderID& firstEventId ) {
    contextEventRecord.createContextEntry(contextName, contextId, firstEventId);
  }

  mace::ContextBaseClass* createContextObjectWrapper( mace::OrderID const& eventID, mace::string const& contextFullName, const uint32_t contextID,
      const uint64_t& current_version, const bool migrationFlag ){
    // TODO: check if the full name is valid
    ADD_SELECTORS("ContextService::createContextObjectWrapper");
    macedbg(1) << "Create context object for: " << contextFullName << " its contextId=" << contextID << " mapping_version=" << current_version << Log::endl;
    ScopedLock sl(getContextObjectMutex);
    
    mace::string contextTypeName = mace::ContextBaseClass::getTypeName( contextFullName );
    mace::ContextBaseClass* newContext = createContextObject( contextTypeName );

    newContext->initialize2(  contextFullName, instanceUniqueID, contextID, 1, 1, this->contextStructure );

    // const mace::vector<mace::ElasticityRule>& rules = eConfig.getElasticityRules();
    // for( mace::vector<mace::ElasticityRule>::const_iterator iter = rules.begin(); iter != rules.end(); iter ++ ){
    //   if( (*iter).relatedContextType(contextTypeName) ){
    //     (newContext->eConfig).addElasticityRule(*iter);
    //     macedbg(1) << "Adding a elasticity rule for context " << contextFullName << Log::endl;
    //   }
    // }
    
    setContextObject( newContext, contextID, contextFullName );
    
    sl.unlock();
    wakeupWaitingThreads(contextFullName);
     
    return newContext;
  }

  /// send internal message either locally with async dispatch thread, or remotely with transport thread
  void forwardInternalMessage( MaceAddr const& dest, mace::InternalMessage const& msg ) const{
    ADD_SELECTORS("ContextService::forwardInternalMessage");
    if( isLocal( dest ) ){
      ContextService *self = const_cast<ContextService *>( this );
      // macedbg(1)<<"Enqueue a message into async dispatch queue. " <<Log::endl;
      
      AsyncDispatch::enqueueEvent(self,(AsyncDispatch::asyncfunc)&ContextService::handleInternalMessagesWrapper,(void*)new mace::InternalMessage( msg ) );
      msg.unlinkHelper();
    }else{
      sender->sendInternalMessage( dest, msg );
    }
  }

  // Forward message to the right address
  void forwardInternalMessageToNewAddress( mace::MaceAddr const& immediate_src, mace::InternalMessage const& msg ) const {
    ADD_SELECTORS("ContextService::forwardInternalMessageToNewAddress");
    const mace::string& targetContextName = msg.getTargetContextName();
    const MaceAddr& dest = mace::ContextMapping::getNodeByContext(contextMapping, targetContextName);
    ASSERTMSG( !isLocal(dest), "The context is on the same node!" );
    macedbg(1) << "Forward message("<< msg <<") to context("<< targetContextName<<") from "<< immediate_src <<" to " << dest << Log::endl;
    sender->sendInternalMessage( dest, msg );

    send__event_contextMappingUpdateSuggest(immediate_src, contextMapping.getCurrentVersion() );
  }

  bool handleMessageToMigratingAndNullContext( mace::ContextBaseClass* ctxObj, mace::InternalMessage const& message, MaceAddr const& src ){
    ADD_SELECTORS("ContextService::handleMessageToMigratingAndNullContext");
    ScopedLock sl(migratingContextMutex);
    receivedExternalMsgCount ++;
    if( isContextMigrating && checkMigratingContext(message) ) {
      receivedExternalMsgCount --;
      return false;
    }
    if( isContextComming && checkCommingContext(message) ) {
      receivedExternalMsgCount --;
      return false;
    }
    ctxObj = getContextObjByName(message.getTargetContextName());
    if( ctxObj == NULL ) {
      if( handleMessageForNullContext(src, message) ) {
        receivedExternalMsgCount --;
        return false;
      } else {
        ctxObj = getContextObjByName(message.getTargetContextName());
        ASSERT(ctxObj != NULL );
      }
    }
    if( isContextMigrating ){
      macedbg(1) << "MessageType=" << (uint16_t)message.getMessageType() << " context=" << ctxObj->contextName << Log::endl;
    }
    ctxObj->increaseHandlingMessageNumber();
    receivedExternalMsgCount --;
    return true;
  }

  void snapshotContext( const mace::string& fileName ) const;

  const ContextMapping& getLatestContextMapping() const {
    return contextMapping.getLatestContextMapping();
  }

  mace::ContextBaseClass* getContextObjByName( mace::string const& contextName ) const{
    ADD_SELECTORS("ContextService::getContextObjByName");
    //macedbg(1) << "Enter getContextObjByName" << Log::endl;
    ScopedLock sl(getContextObjectMutex);
    //macedbg(1) << "After acquire mutex getContextObjectMutex" << Log::endl;
    mace::hash_map< mace::string, mace::ContextBaseClass*, mace::SoftState >::const_iterator cpIt = ctxobjNameMap.find( contextName );
    if( cpIt == ctxobjNameMap.end() ){
      //macedbg(1)<<"context name "<< contextName << " is not found!";
      return NULL;
    }
    return cpIt->second;
  }

  void getUpdatedContextMapping(const uint64_t ver );

  const mace::MaceAddr& getNodeByContext(const mace::string & contextName) {
    const ContextMapping& ctxMapping = this->getLatestContextMapping();
    if( !ctxMapping.hasContext(contextName)){
      this->getUpdatedContextMapping(0);
    }

    const ContextMapping& latestCtxMapping = this->getLatestContextMapping();
    return mace::ContextMapping::getNodeByContext(latestCtxMapping, contextName);
  }

  // Elasticity methods
  uint32_t getConnectionStrength( const uint32_t handler_thread_id, mace::string const& context_name, mace::MaceAddr const& addr );
  uint32_t getRemoteConnectionStrength( const uint32_t handler_thread_id, mace::string const& context_name, mace::MaceAddr const& addr );

  void send__elasticity_ContextNodeStrengthRequest( const uint32_t& handler_thread_id, mace::string const& context_name, 
    mace::MaceAddr const& addr) const;
  void send__elasticity_ContextNodeStrengthReply( mace::MaceAddr const& dest, const uint32_t& handler_thread_id, mace::string const& context_name, 
    mace::MaceAddr const& addr, const uint64_t& value ) const;
  void send__elasticity_contextsColocateRequest( mace::string const& colocate_dest_context, 
    mace::vector<mace::string> const& colocate_src_contexts ) const;
  void send__elasticity_informationReport( const mace::ServerRuntimeInfo& server_info ) const;
  void send__elasticity_serverInfoRequest( const uint32_t& handler_thread_id ) const;
  void send__elasticity_serverInfoReply( const mace::MaceAddr& dest, const uint32_t& handler_thread_id, 
    const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info, const uint64_t& eop_type, const mace::MaceAddr& addr ) const;
  void send__elasticity_contextMigrationQuery( const mace::MaceAddr& dest, const mace::vector< mace::MigrationContextInfo>& m_contexts_info ) const;
  void send__elasticity_contextMigrationQueryReply( const mace::MaceAddr& dest, const mace::vector<mace::string>& accept_m_ctxs) const;

  uint64_t getContextCPUTime( const mace::string& ctx_name );
  // mace::map< mace::MaceAddr, double > getServersCPUUsage();
  double getContextCPUUsage( const mace::string& ctx_name );

  void wakeupHandlerThreadForContextStrength( const uint32_t& handler_thread_id, mace::string const& context_name, 
    mace::MaceAddr const& addr, const uint64_t& value );
  void wakeupHandlerThreadForServersInfo( const uint32_t& handler_thread_id, 
    const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info );

  void clearRemoteContextRuntimeInfos();

  void getContextObjectsByTypes( std::vector<mace::ContextBaseClass*>& contextObjs, const mace::set<mace::string>& context_types );

  void colocateContexts( const mace::string& ctx_name1, const mace::string& ctx_name2 );
  mace::set<uint32_t> getLocalContextIDs( const mace::string& context_type );

  void collectContextsInfos(const uint64_t& r) { elasticityMonitor->collectContextsInfos(r); }
  void processContextElasticity(const mace::string& marker, const bool& migrate_flag) { elasticityMonitor->processContextElasticity(marker, migrate_flag); }
  void updateCPUInfo() { elasticityMonitor->updateCPUInfo(); }

  void waitForServerInformationUpdate(const uint32_t handler_thread_id, const mace::ServerRuntimeInfo& server_info);
  void handleServerInformationReport( mace::MaceAddr const& src, const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info);

  void markStartTimestamp(const mace::string& marker);
  void markEndTimestamp( const mace::string& marker );
  double getMarkerAvgLatency( const mace::string& marker );

  void inactivateContext( const mace::string& context_name);
  void activateContext( const mace::string& context_name);

  // Migration methods
  void send__migration_contextMigrationRequest( const mace::string& context_name, const mace::MaceAddr& dest_node ) const;
  void send__event_MigrationControlMsg(mace::MaceAddr const& dest, const uint8_t control_type, const uint64_t ticket, 
    mace::map<uint32_t, mace::string> const& migrate_contexts, mace::ContextMapping const& ctxMapping) const;
  void send__ContextMigration_UpdateContextMapping( mace::MaceAddr const& dest, mace::map<uint32_t, mace::string> const& migrate_contexts, 
    mace::ContextMapping const& ctxMapping, mace::MaceAddr const& srcAddr ) const;
  void send__ContextMigration_Done( mace::MaceAddr const& dest, mace::map<uint32_t, mace::string> const& migrate_contexts, 
    mace::ContextMapping const& ctxMapping) const;
  void send__event_ContextMigrationRequest( MaceAddr const& destNode, MaceAddr const& dest, mace::Event const& event, 
    uint64_t const& prevContextMapVersion, mace::set< uint32_t > const& migrateContextIds, mace::ContextMapping const& ctxMapping );
  void send__event_TransferContext( MaceAddr const& dest, const mace::ContextBaseClassParams* ctxParams, mace::string const& checkpoint, 
    mace::OrderID const eventId );
  void send__commit_context_migration( mace::OrderID const& eventId, mace::string const& contextName, mace::MaceAddr const& srcAddr, 
    mace::MaceAddr const& destAddr ) const;
  

  void handleContextMigrationRequest( MaceAddr const& dest, mace::string const& contextName, const uint32_t& contextId );
  void handle__event_MigrationControl( const mace::MigrationControl_Message* msg);
  void handle__event_TransferContext( MaceAddr const& src, const mace::ContextBaseClassParams* ctxParams, mace::string const& checkpoint, 
    mace::OrderID const& eventId );
  void addMigratingContextName( mace::string const& ctx_name );
  void releaseBlockedMessageForMigration( mace::set<mace::string> const& migrate_contexts);
  bool checkMigratingContext(mace::InternalMessage const& message);
  bool checkCommingContext(mace::InternalMessage const& message);

};

namespace mace{
class __EventStructure__ {
public:
  mace::Event* event;
  mace::__asyncExtraField extra;

  __EventStructure__(): event(NULL), extra() {}

  __EventStructure__(mace::Event* e, mace::__asyncExtraField& extra): event(e), extra(extra) {}
};

class __ServiceStackEvent__ {
  private:
  ContextService* sv;
      public:
    __ServiceStackEvent__(const int8_t eventType, ContextService* service, const mace::string& targetContextName) : sv(service) {
      ADD_SELECTORS("__ServiceStackEvent__::constructor");
      macedbg(1) << "Enter __ServiceStackEvent__, targetContextName: "<< targetContextName << Log::endl;
      bool newEventCondition;
      switch( eventType ){
        case mace::Event::STARTEVENT:
          //macedbg(1) << "It's the start event!" << Log::endl;
          newEventCondition = ThreadStructure::isFirstMaceInit();
          break;
        case mace::Event::ENDEVENT:
          newEventCondition = ThreadStructure::isFirstMaceExit();
          break;
        default:
          //newEventCondition = ThreadStructure::isOuterMostTransition();
          newEventCondition = ThreadStructure::isApplicationDowncall();
      }
      if( newEventCondition && !mace::Event::isExit ){
        //macedbg(1) << "Enter newEventCondition" << Log::endl;
        const mace::ContextMapping& snapshotMapping = sv->getContextMapping().getSnapshot();
         
        mace::string globalContext = mace::ContextMapping::GLOBAL_CONTEXT_NAME;
        uint32_t ctxId = mace::ContextMapping::hasContext2(snapshotMapping, globalContext);
        if( ctxId <= 0 ){
          macedbg(1) << "The global context is not created. We should create this context at first." << Log::endl;
          mace::OrderID eventId(0, 0);
          sv->asyncHead( eventId, globalContext, eventType, ctxId );
        }
        ASSERTMSG(ctxId > 0, "ContextID must be valid!");
        macedbg(1) << "The contextId = "<< ctxId << Log::endl;
        mace::ContextBaseClass* ctxObj = sv->getContextObjByName(globalContext);     
        ASSERTMSG(ctxObj != NULL, "Fail to get global context object on this node!");  
        mace::Event event;
        mace::OrderID eventId = ctxObj->newCreateTicket();
        ctxObj->createEvent(service, eventId, event, targetContextName, "", eventType, mace::Event::EVENT_OP_OWNERSHIP);
        ThreadStructure::setEvent(event);
      }
      sv->enterInnerService(targetContextName);
    }
    //~__ServiceStackEvent__() { }
};
class __ScopedTransition__ {
public:
  static const uint8_t TYPE_ASYNC_EVENT = 0;
  static const uint8_t TYPE_BROADCAST_EVENT = 1;
  protected:
    ContextService const* sv;
    mace::ContextBaseClass *oldContextObject;
    uint8_t type;
  public:
    __ScopedTransition__( ContextService const* service, uint32_t const& targetContextID, bool isRelease = true, bool newExecuteTicket = false, mace::vector<uint32_t> const& snapshotContextIDs = mace::vector<uint32_t>() ) 
      : sv(service), oldContextObject( ThreadStructure::myContext() ), type(__ScopedTransition__::TYPE_ASYNC_EVENT) {
      ADD_SELECTORS("__ScopedTransition__::constructor#1");
      // macedbg(1) << "Enter here!" <<Log::endl;
      sv->__beginTransition( targetContextID, snapshotContextIDs, isRelease, true );
    }
    __ScopedTransition__( ContextService const* service, __asyncExtraField const& extra, const uint8_t t = __ScopedTransition__::TYPE_ASYNC_EVENT ) 
      : sv(service),  oldContextObject( ThreadStructure::myContext() ), type(t) {
      ADD_SELECTORS("__ScopedTransition__::constructor#2");
      // macedbg(1) << "Event("<< ThreadStructure::myEventID()<<")'s __ScopedTransition__ type = " << (uint32_t)t << Log::endl;
      bool isRelease = false;
      if(t == __ScopedTransition__::TYPE_ASYNC_EVENT) {
        isRelease = true;
      }
      const mace::ContextMapping& snapshotMapping = sv->contextMapping.getLatestContextMapping();
      const uint32_t targetContextID = snapshotMapping.findIDByName( extra.targetContextID );
      mace::vector<uint32_t> snapshotContextIDs;
      for_each( extra.snapshotContextIDs.begin(), extra.snapshotContextIDs.begin(), mace::addSnapshotContextID( snapshotMapping, snapshotContextIDs  ) );
      sv->__beginTransition( targetContextID, snapshotContextIDs, isRelease, false );
    }
    ~__ScopedTransition__() {
      ADD_SELECTORS("__ScopedTransition__::destory");
      if( type == __ScopedTransition__::TYPE_ASYNC_EVENT ) {
        //macedbg(1) << "Event("<< ThreadStructure::myEventID() <<") will invoke __finishTransition" << Log::endl;
        sv->__finishTransition( oldContextObject );
      } else if( type == __ScopedTransition__::TYPE_BROADCAST_EVENT ) {
        //macedbg(1) << "Event("<< ThreadStructure::myEventID() <<") will invoke __finishBroadcastTransition" << Log::endl;
        sv->__finishBroadcastTransition();
      }
      
    }
};
class __ScopedRoutine__ {
  protected:
    ContextService const* sv;
    mace::ContextBaseClass *oldContextObject;
    bool isRelease;
    mace::EventOperationInfo oldEventOpInfo;
  public:
  __ScopedRoutine__( ContextService const* service, uint32_t const& targetContextID, const bool isRelease = true, const bool newExecuteTicket = false, mace::vector<uint32_t> const& snapshotContextIDs = mace::vector<uint32_t>() ) 
    : sv(service), oldContextObject( ThreadStructure::myContext() ), isRelease(isRelease) {
    oldEventOpInfo = ThreadStructure::myEvent().eventOpInfo;
    sv->__beginMethod( targetContextID, snapshotContextIDs, isRelease, newExecuteTicket );
  }
  ~__ScopedRoutine__() {
    mace::Event& event = ThreadStructure::myEvent();
    mace::ContextBaseClass* ctxObj = ThreadStructure::myContext();

    ContextService* _service = const_cast<ContextService*>(sv);
    if( _service->contextInfoCollectFlag > 0 ){
      (ctxObj->runtimeInfo).stopEvent(event.eventId);
    }
    
    mace::vector<mace::EventOperationInfo> ownershipOpInfos = ctxObj->extractOwnershipOpInfos(event.eventId);
    if( ownershipOpInfos.size() > 0 ) {
      mace::EventOperationInfo& eop = event.eventOpInfo;
      ctxObj->applyOwnershipOperations( _service, eop, ownershipOpInfos);
      for( uint32_t i=0; i<ownershipOpInfos.size(); i++ ) {
        eop.newCreateContexts.erase( ownershipOpInfos[i].toContextName );
      }
      oldEventOpInfo.newCreateContexts = eop.newCreateContexts;
    }

    oldEventOpInfo.contextDAGVersions = event.eventOpInfo.contextDAGVersions;
    event.eventOpInfo = oldEventOpInfo;
    
    mace::vector<mace::string> locked_contexts;
    mace::vector<mace::EventOperationInfo> local_lock_requests;
    ContextStructure& contextStructure = _service->contextStructure;
    if( !contextStructure.isUpperBoundContext(ctxObj->contextName) ) {
       local_lock_requests = ctxObj->getLocalLockRequests( event.eventId );
       locked_contexts = ctxObj->getLockedChildren( event.eventId );

       ctxObj->clearLocalLockRequests( event.eventId );
       ctxObj->clearLockedChildren( event.eventId );
    }
    event.eventOrderInfo.setLocalLockRequests( local_lock_requests, locked_contexts );

    sv->__finishMethod( oldContextObject );
  }
};

class __CheckMethod__: public ContextLocatorInterface {
protected:
  uint32_t targetContextID;
  mace::vector< uint32_t > snapshotContextIDs;
  MaceAddr destAddr;
  bool local;
  bool passCheck;
    
  static mace::vector< mace::string > nullNames;

public:
  __CheckMethod__( ContextService const* _service, const int8_t eventType, mace::string const& targetContextName,
      mace::vector< mace::string > const& snapshotContextNames = nullNames  ): local(false), passCheck(true) {
    ADD_SELECTORS("__CheckMethod__::constructor#1");
    macedbg(1) << "Enter __CheckMethod__ constructor for async and broadcast" << Log::endl;
    mace::string target_ctx_name;
    if(targetContextName.empty()) {
      target_ctx_name = mace::ContextMapping::GLOBAL_CONTEXT_NAME;
    } else {
      target_ctx_name = targetContextName;
    }

    ContextService* service = const_cast<ContextService*>(_service);
    __ServiceStackEvent__ _sse( eventType, service, target_ctx_name );
    mace::string methodName = "";
    checkExecution( service, target_ctx_name, snapshotContextNames, methodName );
  }

  // For routine invocation
  __CheckMethod__( ContextService const* _service, mace::string const& targetContextName, mace::string const& methodName,
      mace::vector< mace::string > const& snapshotContextNames = nullNames ): local(false) {
    ADD_SELECTORS("__CheckMethod__::constructor#2");
    //macedbg(1) << "Enter __CheckMethod__ constructor for routine" << Log::endl;
    mace::string target_ctx_name;
    if(targetContextName.empty()) {
      target_ctx_name = mace::ContextMapping::GLOBAL_CONTEXT_NAME;
    } else {
      target_ctx_name = targetContextName;
    }
    ContextService* service = const_cast<ContextService*>(_service);
    checkExecution( service, target_ctx_name, snapshotContextNames, methodName );
  }
  bool isLocal() const{
    return local;
  }

  MaceAddr const& getDestination() const{
    return destAddr;
  }

  uint32_t const getTargetContextID() const{
    return targetContextID;
  }

  mace::vector< uint32_t > const& getSnapshotContextIDs() const{
    return snapshotContextIDs;
  }
protected:

  void checkExecution( ContextService* service, mace::string const& targetContextName, mace::vector< mace::string > const& snapshotContextNames,
      mace::string const& methodName ){
    ADD_SELECTORS("__CheckMethod__::checkExecution");
    macedbg(1) << "targetContextName: "<< targetContextName << Log::endl;
    const mace::ContextMapping& currentMapping = service->contextMapping.getLatestContextMapping();
    if(targetContextName == mace::ContextMapping::GLOBAL_CONTEXT_NAME && currentMapping.getHead() == Util::getMaceAddr()) { // downcall transition from service
      local = true;
      destAddr = Util::getMaceAddr();
      targetContextID = currentMapping.findIDByName( targetContextName );
    } else {
      // Routine invocation preparation
      mace::Event& event = ThreadStructure::myEvent();
      ContextBaseClass* myContextObj = ThreadStructure::myContext();

      if( service->contextInfoCollectFlag > 0 ){
        (myContextObj->runtimeInfo).stopEvent(event.eventId);
        (myContextObj->runtimeInfo).addCalleeContext(targetContextName, methodName);
      }

      mace::vector<mace::EventOperationInfo> ownershipOpInfos = myContextObj->extractOwnershipOpInfos(event.eventId);
      if( ownershipOpInfos.size() > 0 ) {
        mace::EventOperationInfo& eop = event.eventOpInfo;
        myContextObj->applyOwnershipOperations( service, eop, ownershipOpInfos);
        for( uint32_t i=0; i<ownershipOpInfos.size(); i++ ) {
          eop.newCreateContexts.erase( ownershipOpInfos[i].toContextName );
        }
      }
      
      const ContextStructure& contextStructure = service->contextStructure;
      
      if( !contextStructure.checkParentChildRelation(myContextObj->contextName, targetContextName) ) {
        maceerr << "Context("<< myContextObj->contextName <<") is not Context("<< targetContextName <<")'s parent!!" << Log::endl;
        ASSERT(false);
      }

      // keep a copy of eventOpInfo before routine invocation
      event.eventOpInfoCopy = event.eventOpInfo;

      event.eventOpInfo.fromContextName = myContextObj->contextName;
      event.eventOpInfo.toContextName = targetContextName;
      event.eventOpInfo.opType = mace::EventOperationInfo::ROUTINE_OP;
      event.eventOpInfo.ticket = myContextObj->getNextOperationTicket(event.eventId);
      event.eventOpInfo.methodName = methodName;
      event.eventOpInfo.addAccessedContext( targetContextName );
      event.eventOpInfo.requireContextName = myContextObj->contextName;
      event.eventOpInfo.permitContexts = myContextObj->getEventPermitContexts(event.eventId);

      myContextObj->addEventToContext( event.eventId, targetContextName );
      myContextObj->waitForEventExecutePermission(service, event.eventOpInfo);
      
      macedbg(1) << "Launch routine: " << event.eventOpInfo << Log::endl;
      destAddr = service->asyncHead(event.eventId, targetContextName, mace::Event::ALLOCATE_CTX_OBJECT, targetContextID);
    }
  }
};

class __CheckRoutine__: public __CheckMethod__{
private:
  __ScopedRoutine__* sr;
public:
  
  __CheckRoutine__( ContextService const* service, mace::string const& targetContextName, mace::string const& methodName,
      mace::vector< mace::string > const& snapshotContextNames = nullNames ): 
      __CheckMethod__( service, targetContextName, methodName, snapshotContextNames ), sr(NULL) {
    if( isLocal() ){
      sr = new __ScopedRoutine__( service, targetContextID, true, false, snapshotContextIDs );
    }
  }
  ~__CheckRoutine__(){ delete sr; }
};
class __CheckTransition__: public __CheckMethod__{
private:
  __ScopedTransition__* sr;
public:
  
  __CheckTransition__( ContextService const* service, const int8_t eventType, mace::string const& targetContextName, 
      mace::vector< mace::string > const& snapshotContextNames = nullNames  ): 
      __CheckMethod__( service, eventType, targetContextName, snapshotContextNames ), sr( NULL ){
    ADD_SELECTORS("__CheckTransition__::constructor");
    macedbg(1) << "Enter __CheckTransition__" << Log::endl;
    if( isLocal() ){
      sr = new __ScopedTransition__( service, targetContextID, true, true, snapshotContextIDs );
    }
  }
  ~__CheckTransition__(){ delete sr; }
};

template <typename T> 
class Delegator{
private:
  T returnValue;
public:
  Delegator(  InternalMessageSender* sender, Message* const message, ContextLocatorInterface const& cm, const ContextService* service, const uint8_t sid ){
    ADD_SELECTORS("Delegator::constructor1");
    ScopedContextRPC rpc;
    Routine_Message* routine_msg = static_cast<Routine_Message*>(message);
    const mace::Event& event = routine_msg->getEvent();
    const mace::string toContextName = event.eventOpInfo.toContextName;
    macedbg(1) << "Event("<< event.eventId <<") make a routine call to context("<< toContextName <<")!" << Log::endl;
    mace::InternalMessageID msgId( Util::getMaceAddr(), toContextName, 0 );
    InternalMessage im(routine_msg, msgId, sid);
    service->forwardInternalMessage(cm.getDestination(), im);


    rpc.get( returnValue );
    
    mace::EventOperationInfo eventOpInfoCopy = ThreadStructure::myEvent().eventOpInfoCopy;

    mace::Event e2;
    rpc.get( e2 );
    // restore the eventOpInfo before routine invocation
    mace::Event& e = ThreadStructure::myEvent();

    ContextBaseClass* ctxObj = ThreadStructure::myContext();
    ContextService* _non_service = const_cast<ContextService*>(service);
    mace::EventOperationInfo& eop = e.eventOpInfo;
    
    ctxObj->unlockContext( _non_service, eop, e2.eventOrderInfo.localLockRequests, e2.eventOrderInfo.lockedContexts );

    e.eventOpInfo = eventOpInfoCopy;
    e.eventOpInfo.newCreateContexts = e2.eventOpInfo.newCreateContexts;
    e.eventOpInfo.contextDAGVersions = e2.eventOpInfo.contextDAGVersions;
    macedbg(1) << "Event("<< e.eventId <<") receive return value from context("<< toContextName <<")!" << Log::endl;

    if( service->contextInfoCollectFlag > 0 ){
        (ctxObj->runtimeInfo).runEvent(e);
    }
  }

  T getValue() const { return returnValue; }
};
template <> 
class Delegator<void>{
public:
  Delegator( InternalMessageSender* sender, Message* const message, ContextLocatorInterface const& cm, const ContextService* service, const uint8_t sid ){
    ADD_SELECTORS("Delegator::constructor2");
    ScopedContextRPC rpc;
    Routine_Message* routine_msg = static_cast<Routine_Message*>(message);
    const mace::Event& event = routine_msg->getEvent();
    const mace::string toContextName = event.eventOpInfo.toContextName;
    mace::InternalMessageID msgId( Util::getMaceAddr(), event.eventOpInfo.toContextName, 0 );
    macedbg(1) << "Event("<< event.eventId <<") op("<< event.eventOpInfo <<") make a routine call to context("<< toContextName <<")!" << Log::endl;


    InternalMessage im(routine_msg, msgId, sid);
    service->forwardInternalMessage(cm.getDestination(), im);
    
    mace::EventOperationInfo eventOpInfoCopy = ThreadStructure::myEvent().eventOpInfoCopy;

    mace::Event e2;
    rpc.get( e2 );
    // restore the eventOpInfo before routine invocation
    mace::Event& e = ThreadStructure::myEvent();

    ContextBaseClass* ctxObj = ThreadStructure::myContext();
    ContextService* _non_service = const_cast<ContextService*>(service);
    mace::EventOperationInfo& eop = e.eventOpInfo;
    ctxObj->unlockContext( _non_service, eop, e2.eventOrderInfo.localLockRequests, e2.eventOrderInfo.lockedContexts );

    macedbg(1) << "Event("<< e2.eventId <<") op("<< e2.eventOpInfo <<") receive return value from context("<< toContextName <<")!" << Log::endl;

    e.eventOpInfo = eventOpInfoCopy;
    e.eventOpInfo.newCreateContexts = e2.eventOpInfo.newCreateContexts;
    e.eventOpInfo.contextDAGVersions = e2.eventOpInfo.contextDAGVersions;

    

    if( service->contextInfoCollectFlag > 0 ){
        (ctxObj->runtimeInfo).runEvent(e);
    }
    return;
  }
  void getValue() const{ return; }
};


}

#endif
