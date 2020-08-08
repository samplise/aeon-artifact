/**
 * chuangw: 02/04/2012
 *
 * A high-level event is a conceptual execution of event. The old event model in Mace says the event starts when (1) transport layer processes a received message, (2) Timer goes off, or (3) Asynchronous event handler processes an asynchronous message. An event ends when the event handler finishes the processing. 
 *
 * In Full Context model, events are created by the head of virtual node. The head assigns a globally unique event id to the event, and routes the event to the physical node based on the context of the event. 
 * The reason for having a high-level event in addition to the existing low-level event is that a high-level event is composed of several low-level events. Low level events starts and ends on the same physical machine, whereas the conceptual, high-level event, because it can transition to different contexts during the execution, it is likely the execution of the event spans across many physical nodes.
 *
 * */
#ifndef _MACE_HIGHLEVELEVENT_H
#define _MACE_HIGHLEVELEVENT_H
// include system library header
#include <pthread.h>
// include mace library header
#include "mace-macros.h"
#include "Serializable.h"
#include "ScopedLock.h"
#include "mlist.h"
#include "mvector.h"
#include "Serializable.h"
#include "Printable.h"
#include "Message.h"
#include "Accumulator.h"
#include <boost/shared_ptr.hpp>

// class ContextStructure;

namespace mace{

/**
 * This class should only be created by head node.
 *
 * It creates a globally unique event id (because it's only created by head node)
 * 
 * This event class object is supposed to be carried around by messages generated by async/sync/timer calls
 * */
class OrderID: public Serializable, public PrintPrintable  {
public:
    uint32_t ctxId;
    uint64_t ticket;
    //mace::string ctxName;

    OrderID(): ctxId(0), ticket(0) {}
    OrderID(uint32_t ctxId, uint64_t ticket): 
      ctxId(ctxId), 
      ticket(ticket) {}

    bool compatible(const OrderID& orderId) {
      if(this->ctxId == orderId.ctxId){
        return true;
      }else {
        return false;
      }
    }

    OrderID& operator=(const OrderID& orig){
        ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
        this->ctxId = orig.ctxId;
        this->ticket = orig.ticket;
        return *this;
    }


    bool operator<(const OrderID& o2) const {
      if(this->ctxId < o2.ctxId ){
        return true;
      } else if (o2.ctxId == this->ctxId){
        if( this->ticket < o2.ticket ) {
          return true;
        } else {
          return false;
        }
        
      }else {
        return false;
      }
    }

    bool operator>(const OrderID& o2) const {
      if( this->ctxId > o2.ctxId ){
        return true;
      } else if (o2.ctxId == this->ctxId){
        if( this->ticket > o2.ticket ) {
          return true;
        } else {
          return false;
        }
        
      }else {
        return false;
      }
    }

    bool operator<=(const OrderID& o2) const {
      if(this->ctxId <= o2.ctxId ){
        return true;
      } else if (o2.ctxId == this->ctxId){
        if( this->ticket <= o2.ticket ) {
          return true;
        } else {
          return false;
        }
        
      }else {
        return false;
      }
    }

    bool operator>=(const OrderID& o2) const {
      if( this->ctxId >= o2.ctxId ){
        return true;
      } else if (o2.ctxId == this->ctxId){
        if( this->ticket >= o2.ticket ) {
          return true;
        } else {
          return false;
        }
        
      }else {
        return false;
      }
    }

    bool operator == (const OrderID& o2) const {
      if(this->ctxId == o2.ctxId && this->ticket == o2.ticket) {
        return true;
      } else {
        return false;
      }
    }

    bool operator != (const OrderID& o2) const {
      if(this->ctxId != o2.ctxId || this->ticket != o2.ticket) {
        return true;
      } else {
        return false;
      }
    }

    virtual void serialize(std::string& str) const{
        mace::serialize( str, &ctxId );
        mace::serialize( str, &ticket );
        //mace::serialize( str, &ctxName );
    }

    virtual int deserialize(std::istream & is) throw (mace::SerializationException){
        int serializedByteSize = 0;
        serializedByteSize += mace::deserialize( is, &ctxId );
        serializedByteSize += mace::deserialize( is, &ticket );
        //serializedByteSize += mace::deserialize( is, &ctxName );
        return serializedByteSize;
    }

    void print(std::ostream& out) const {
      out<< "OrderID(";
      out<< "contextId = "; mace::printItem(out, &(ctxId)); out<<", ";
      out<< "ticket = "; mace::printItem(out, &(ticket)); //out<<", ";
      //out<< "contextName = "; mace::printItem(out, &(ctxName)); 
      out<< ")";
    }

    void printNode(PrintNode& pr, const std::string& name) const {
      mace::PrintNode printer(name, "OrderID" );
  
      mace::printItem( printer, "ctxId", &ctxId );
      mace::printItem( printer, "ticket", &ticket );
      //mace::printItem( printer, "ctxName", &ctxName );
      pr.addChild( printer );
    }
};

class EventOwnershipOpInfo: public Serializable, public PrintPrintable {
public:
  static const uint8_t NONE_OP = 0;
  static const uint8_t ADD_OP = 1;
  static const uint8_t DELETE_OP = 2;
  
  uint8_t opType;
  mace::pair<mace::string, mace::string> ownershipContextPair;
  
  EventOwnershipOpInfo() { 
    opType = NONE_OP; 
  }

  EventOwnershipOpInfo(const uint8_t op_type, mace::string const& parentContextName, mace::string const& childContextName ) {
    opType = op_type;
    ownershipContextPair.first = parentContextName;
    ownershipContextPair.second = childContextName;
  }

  virtual void serialize(std::string& str) const{
    mace::serialize( str, &opType );
    mace::serialize( str, &ownershipContextPair );
  }

  virtual int deserialize(std::istream & is) throw (mace::SerializationException){
    int serializedByteSize = 0;
    serializedByteSize += mace::deserialize( is, &opType );
    serializedByteSize += mace::deserialize( is, &ownershipContextPair );
    return serializedByteSize;
  }

  void print(std::ostream& out) const { }

  void printNode(PrintNode& pr, const std::string& name) const { }

  EventOwnershipOpInfo& operator=(EventOwnershipOpInfo const& orig) {
    ASSERTMSG( this != &orig, "Self copy is forbidden!");
    this->opType = orig.opType;
    this->ownershipContextPair = orig.ownershipContextPair;
  }

};

class EventBroadcastInfo: public Serializable, public PrintPrintable {
private:
  OrderID broadcastId;
  OrderID preBroadcastId;

  mace::map<uint32_t, uint64_t> broadcastIDRecord;
    
public:
  mace::OrderID newBroadcastEventID( const uint32_t contextId );

  void setBroadcastEventID(OrderID const& id ) { broadcastId = id; }
  mace::OrderID getBroadcastEventID() { return broadcastId; }
  void setPreBroadcastEventID( OrderID const& preId ) { preBroadcastId = preId; }
  OrderID getPreBroadcastEventID() const { return preBroadcastId; }

  EventBroadcastInfo(): broadcastId(), preBroadcastId(), broadcastIDRecord() { }
  ~EventBroadcastInfo() {
    broadcastIDRecord.clear();
  }

  EventBroadcastInfo& operator=(const EventBroadcastInfo& orig){
    ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
    this->broadcastId = orig.broadcastId;
    this->preBroadcastId = orig.preBroadcastId;
    this->broadcastIDRecord = orig.broadcastIDRecord;
    return *this;
  }

  virtual void serialize(std::string& str) const{
    mace::serialize( str, &broadcastId );
    mace::serialize( str, &preBroadcastId );
    mace::serialize( str, &broadcastIDRecord );
  }

  virtual int deserialize(std::istream & is) throw (mace::SerializationException){
    int serializedByteSize = 0;
    serializedByteSize += mace::deserialize( is, &broadcastId );
    serializedByteSize += mace::deserialize( is, &preBroadcastId );
    serializedByteSize += mace::deserialize( is, &broadcastIDRecord );
    return serializedByteSize;
  }

  void print(std::ostream& out) const {
    out<< "EventBroadcastInfo(";
    out<< "broadcastId = "; mace::printItem(out, &(broadcastId)); out<<", ";
    out<< "preBroadcastId = "; mace::printItem(out, &(preBroadcastId)); out<<", ";
    out<< "broadcastIDRecord = "; mace::printItem(out, &(broadcastIDRecord)); 
    out<< ")";
  }

  void printNode(PrintNode& pr, const std::string& name) const {
    mace::PrintNode printer(name, "EventBroadcastInfo" );
  
    mace::printItem( printer, "broadcastId", &broadcastId );
    mace::printItem( printer, "preBroadcastId", &preBroadcastId );
    mace::printItem( printer, "broadcastIDRecord", &broadcastIDRecord );
    pr.addChild( printer );
  }
};


class EventMessageRecord: public PrintPrintable, public Serializable {
public:
  uint8_t sid;
  MaceKey dest;
  mace::string message;
  registration_uid_t rid;
  EventMessageRecord(  ){ }
  EventMessageRecord( uint8_t sid, MaceKey dest, mace::string message, registration_uid_t rid ):
    sid( sid ), dest( dest ), message( message ), rid (rid){}
  void print(std::ostream& out) const {
    out<< "EventMessageRecord(";
    out<< "sid="; mace::printItem(out, &(sid) ); out<<", ";
    out<< "dest="; mace::printItem(out, &(dest) ); out<<", ";
    out<< "message="; mace::printItem(out, &(message) ); out<<", ";
    out<< "rid="; mace::printItem(out, &(rid) );
    out<< ")";
  }
  void printNode(PrintNode& pr, const std::string& name) const {
    mace::PrintNode printer(name, "EventMessageRecord" );
    mace::printItem( printer, "sid", &sid );
    mace::printItem( printer, "dest", &dest );
    mace::printItem( printer, "message", &message );
    mace::printItem( printer, "rid", &rid );
    pr.addChild( printer );
  }
  virtual void serialize(std::string& str) const{
      mace::serialize( str, &sid );
      mace::serialize( str, &dest );
      mace::serialize( str, &message   );
      mace::serialize( str, &rid   );
  }
  virtual int deserialize(std::istream & is) throw (mace::SerializationException){
      int serializedByteSize = 0;
      serializedByteSize += mace::deserialize( is, &sid );
      serializedByteSize += mace::deserialize( is, &dest );
      serializedByteSize += mace::deserialize( is, &message   );
      serializedByteSize += mace::deserialize( is, &rid   );
      return serializedByteSize;
  }
};
#ifdef EVENTREQUEST_USE_SHARED_PTR
  typedef boost::shared_ptr<mace::Message> RequestType ;
#else
  typedef mace::Message* RequestType;
#endif
//#define EVENTREQUEST_USE_SHARED_PTR
class EventRequestWrapper: public PrintPrintable, public Serializable {
public:

  uint8_t sid;
  RequestType request;

  EventRequestWrapper(  ): sid( 0 ), request(){ }
  EventRequestWrapper( EventRequestWrapper const& right );
  EventRequestWrapper( uint8_t sid, mace::Message* request ):
    sid( sid ), request( request ){}
  ~EventRequestWrapper();
  mace::EventRequestWrapper & operator=( mace::EventRequestWrapper const& right );
  void print(std::ostream& out) const ;
  void printNode(PrintNode& pr, const std::string& name) const ;
  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);
};

class EventUpcallWrapper: public PrintPrintable, public Serializable {
public:

  uint8_t sid;
  mace::Message* upcall;

  EventUpcallWrapper(  ): sid( 0 ), upcall(){ }
  EventUpcallWrapper( EventUpcallWrapper const& right );
  EventUpcallWrapper( uint8_t sid, mace::Message* upcall ):
    sid( sid ), upcall( upcall ){}
  ~EventUpcallWrapper();
  mace::EventUpcallWrapper & operator=( mace::EventUpcallWrapper const& right );
  void print(std::ostream& out) const ;
  void printNode(PrintNode& pr, const std::string& name) const ;
  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);
};

class EventOperationInfo: public Serializable, public PrintPrintable {
public:
  const static uint8_t NULL_OP = 0;
  const static uint8_t BROADCAST_OP = 1;
  const static uint8_t ADD_OWNERSHIP_OP = 2;
  const static uint8_t DELETE_OWNERSHIP_OP = 3;

  const static uint8_t ASYNC_OP = 10;
  const static uint8_t ROUTINE_OP = 11;

public:
  mace::OrderID eventId;
  uint16_t opType;
  mace::string toContextName;
  mace::string fromContextName;
  uint32_t ticket;
  uint16_t eventOpType;
  mace::string methodName;
  mace::vector<mace::string> accessedContexts;
  mace::string requireContextName;
  mace::vector<mace::string> permitContexts;
  mace::set<mace::string> newCreateContexts;
  mace::map<mace::string, uint64_t> contextDAGVersions;
  
  EventOperationInfo& operator=(const EventOperationInfo& orig){
    ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
    this->eventId = orig.eventId;
    this->opType = orig.opType;
    this->toContextName = orig.toContextName;
    this->fromContextName = orig.fromContextName;
    this->ticket = orig.ticket;
    this->eventOpType = orig.eventOpType;
    this->methodName = orig.methodName;
    this->accessedContexts = orig.accessedContexts;
    this->requireContextName = orig.requireContextName;
    this->permitContexts = orig.permitContexts;
    this->newCreateContexts = orig.newCreateContexts;
    this->contextDAGVersions = orig.contextDAGVersions;
    return *this;
  }

  bool operator==(const EventOperationInfo& orig ) {
    if( eventId == orig.eventId && opType == orig.opType && toContextName == orig.toContextName && fromContextName == orig.fromContextName && 
        ticket == orig.ticket ){
      return true;
    } else {
      return false;
    }
  }

  bool operator<(const EventOperationInfo& o) const {
    if( this->eventId < o.eventId ) {
      return true;
    }
    if( this->eventId == o.eventId && this->opType < o.opType ){
      return true;
    } 

    if( this->eventId == o.eventId && this->opType == o.opType && this->toContextName < o.toContextName) {
      return true;
    }

    if( this->eventId == o.eventId && this->opType == o.opType && this->toContextName == o.toContextName && 
        this->fromContextName < o.fromContextName ) {
      return true;
    }

    if( this->eventId == o.eventId && this->opType == o.opType && this->toContextName == o.toContextName && 
        this->fromContextName == o.fromContextName && this->ticket < o.ticket ) {
      return true;
    }

    return false;
  }

  bool operator>(const EventOperationInfo& o) const {
    if( this->eventId > o.eventId ) {
      return true;
    }
    if(this->eventId == o.eventId && this->opType > o.opType ){
      return true;
    } 

    if( this->eventId == o.eventId && this->opType == o.opType && this->toContextName > o.toContextName) {
      return true;
    }

    if( this->eventId == o.eventId && this->opType == o.opType && this->toContextName == o.toContextName && this->fromContextName > o.fromContextName ) {
      return true;
    }

    if( this->eventId == o.eventId && this->opType == o.opType && this->toContextName == o.toContextName && this->fromContextName == o.fromContextName && this->ticket > o.ticket ) {
      return true;
    }

    return false;
  }
  
public:
  EventOperationInfo( const mace::OrderID& eventId, const uint8_t type, mace::string const& toCtxName, mace::string const& fromCtxName, 
    const uint32_t ticket, const uint8_t eventOpType, mace::string const& methodName): eventId(eventId), opType(type), 
    toContextName(toCtxName), fromContextName(fromCtxName), ticket(ticket), eventOpType(eventOpType), methodName(methodName), accessedContexts(),
    requireContextName("")  { }
  EventOperationInfo( const mace::OrderID& eventId, const uint8_t type, mace::string const& toCtxName, mace::string const& fromCtxName, 
    const uint32_t ticket, const uint8_t eventOpType ): eventId(eventId), opType(type), 
    toContextName(toCtxName), fromContextName(fromCtxName), ticket(ticket), eventOpType(eventOpType), methodName(""), accessedContexts(),
    requireContextName("") { }
  EventOperationInfo(): opType( NULL_OP ), toContextName(), fromContextName(), ticket(0), eventOpType(1), 
    methodName(""), accessedContexts(), requireContextName("") { }

  mace::string getPreAccessContextName( mace::string const& ctx_name ) const;
  void addAccessedContext( mace::string const& ctx_name );
  bool hasAccessed( const mace::string& ctx_name ) const;
  void setContextDAGVersion( const mace::string& ctx_name, const uint32_t& ver );

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);
  void print(std::ostream& out) const;
  void printNode(PrintNode& pr, const std::string& name) const;
};

class EventExecutionInfo: public Serializable, public PrintPrintable {
public:
  EventExecutionInfo(): currentTicket(1), already_committed(false), eventOpType(1) {}
  EventExecutionInfo( mace::string const& create_ctx_name, mace::string const& target_ctx_name, const uint8_t& event_op_type  ): 
      currentTicket(1), already_committed(false), createContextName(create_ctx_name), targetContextName(target_ctx_name), 
      eventOpType(event_op_type){ }
    
  
  void addEventPermitContext( const mace::string& ctxName );
  mace::set<mace::string> getEventPermitContexts() const { return permitContexts; }
  bool checkEventExecutePermitCache( const mace::string& ctxName );
  void enqueueLocalLockRequest( const mace::EventOperationInfo& eventOpInfo );
  mace::vector< mace::EventOperationInfo > getLocalLockRequests( ) { return localLockRequests; }
  void clearLocalLockRequests() { localLockRequests.clear(); }
  void clearEventPermitCache() { permitContexts.clear(); }

  void addEventOpInfo( mace::EventOperationInfo const& opInfo ) { eventOpInfos.push_back(opInfo); }
  uint64_t getNextTicket() { return currentTicket ++; }

  uint32_t getEventOpsSize() const { return eventOpInfos.size(); }
  void removeEventOp( mace::EventOperationInfo const& opInfo );

  void addEventToContext( mace::string const& toContext );
  void addEventToContextCopy( mace::string const& toContext );
  void addEventFromContext( mace::string const& fromContext );

  uint32_t getToContextSize() const { return toContextNames.size(); }
  mace::set<mace::string> getToContextNames() const { return toContextNames; }

  mace::vector<mace::EventOperationInfo> extractOwnershipOpInfos();
  void enqueueOwnershipOpInfo( EventOperationInfo const& opInfo );
  bool checkParentChildRelation(mace::string const& parentContextName, mace::string const& childContextName ) const;
  mace::EventOperationInfo getNewContextOwnershipOp( mace::string const& parentContextName, mace::string const& childContextName );
  
  bool checkAlreadyCommittedFlag() { return already_committed; }
  void setAlreadyCommittedFlagTrue() { already_committed = true; }
  void eraseToContext( const mace::string& toContextName );
  bool localUnlockContext( mace::EventOperationInfo const& eventOpInfo, mace::vector<mace::EventOperationInfo> const& local_lock_requests, 
    mace::vector<mace::string> const& local_require_contexts);
  
  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);
  void print(std::ostream& out) const { }
  void printNode(PrintNode& pr, const std::string& name) const { }

  EventExecutionInfo& operator=(const EventExecutionInfo& orig) {
    ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
    this->fromContextNames = orig.fromContextNames;
    this->toContextNames = orig.toContextNames;
    this->toContextNamesCopy = orig.toContextNamesCopy;
    this->eventOpInfos = orig.eventOpInfos;
    this->subEvents = orig.subEvents;
    this->deferredMessages = orig.deferredMessages;
    this->permitContexts = orig.permitContexts;
    this->localLockRequests = orig.localLockRequests;
    this->lockedChildren = orig.lockedChildren;
    this->ownershipOps = orig.ownershipOps;
    this->currentTicket = orig.currentTicket;
    this->newContextId = orig.newContextId;
    this->already_committed = orig.already_committed;
    this->createContextName = orig.createContextName;
    this->targetContextName = orig.targetContextName;
    this->eventOpType = orig.eventOpType;
    return *this;
  }

  void setNewContextID(const uint32_t& newContextId) { this->newContextId = newContextId; }
  uint32_t getNewContextID() { return newContextId; }
  void enqueueSubEvent( EventRequestWrapper const& eventRequest ) { subEvents.push_back(eventRequest); }
  void enqueueExternalMessage( EventMessageRecord const& msg ) { deferredMessages.push_back(msg); }
  mace::vector< mace::EventRequestWrapper > getSubEvents() { return subEvents; }
  mace::vector< mace::EventMessageRecord > getExternalMessages() { return deferredMessages; }
  
  mace::set<mace::string> getFromContexts() { return fromContextNames; }
  mace::set<mace::string> getToContextsCopy() const { return toContextNamesCopy; }
  mace::set<mace::string> getToContexts() const { return toContextNames; }
  mace::vector<mace::string> getLockedChildren() const;
  void clearLockedChildren() { lockedChildren.clear(); }

  void addExecutedContextNames( mace::set<mace::string> const& ctxNames );
  void computeLocalUnlockedContexts();
  
private:
  mace::set<mace::string> fromContextNames;
  mace::set<mace::string> toContextNames;
  mace::set<mace::string> toContextNamesCopy;
  mace::vector<mace::EventOperationInfo> eventOpInfos;
  mace::vector<EventRequestWrapper> subEvents;
  mace::vector<EventMessageRecord> deferredMessages;
  mace::set< mace::string > permitContexts;
  mace::vector< mace::EventOperationInfo > localLockRequests;
  mace::set< mace::string > lockedChildren; 
  mace::vector< mace::EventOperationInfo > ownershipOps;
  
  uint64_t currentTicket;
  uint32_t newContextId;
  bool already_committed;

public:
  mace::string createContextName;
  mace::string targetContextName;
    
  uint8_t eventOpType;
  
};

class EventOrderInfo: public PrintPrintable, public Serializable {
public:
  mace::vector< mace::string > lockedContexts;
  mace::vector< mace::EventOperationInfo > localLockRequests;

public:
  EventOrderInfo() { }
  ~EventOrderInfo() { }

  virtual void serialize(std::string& str) const;
  virtual int deserialize(std::istream & is) throw (mace::SerializationException);
  void print(std::ostream& out) const { }
  void printNode(PrintNode& pr, const std::string& name) const { }

  EventOrderInfo& operator=(const EventOrderInfo& orig);

public:
  void setLocalLockRequests( mace::vector<mace::EventOperationInfo> const& local_lock_requests, mace::vector<mace::string> const& locked_contexts);

};


bool operator==( mace::EventMessageRecord const& r1, mace::EventMessageRecord const& r2);
class Event: public PrintPrintable, public Serializable{
public:
  static const uint8_t EVENT_OP_READ = 0;
  static const uint8_t EVENT_OP_WRITE = 1;
  static const uint8_t EVENT_OP_OWNERSHIP = 2;

public:
    /* chuangw: experiment result from Event_test:
     *  mace::set is much faster than mace::hash_set */
    typedef mace::set< uint32_t > EventServiceContextType;
    typedef mace::map<uint8_t, EventServiceContextType > EventContextType;
    /* chuangw: experiment result from Event_test:
     *  mace::map is much faster than mace::hash_map */
    typedef mace::map< uint32_t, mace::string> EventServiceSnapshotContextType;
    typedef mace::map<uint8_t, EventServiceSnapshotContextType > EventSnapshotContextType;
    // typedef EventSkipRecord EventSkipRecordType ;
    // typedef mace::map<uint8_t, EventSkipRecordType > SkipRecordType;
    typedef mace::vector< EventRequestWrapper > EventRequestType;
    typedef mace::vector< EventMessageRecord > DeferredMessageType;
    typedef mace::vector< EventUpcallWrapper > DeferredUpcallType;
    typedef mace::map<uint32_t, uint64_t> EventOrderTicketType;

    /**
     * Default constructor. 
     * Initialize the event ID to zero, and set type to UNDEFEVENT
     * */
    Event():
      eventId ( ), 
      eventType ( mace::Event::UNDEFEVENT ) { }
    /* creates a new event */
    Event( const int8_t type ): eventType(type), eventContexts(), eventSnapshotContexts() {
      newEventID( type );
      initialize();
    }
    /**
     * Initialize a new event, using the ticket number stored in ThreadStructure::myTicket() 
     * @type the event type
     * 
     * */
    void newEventID( const int8_t type);
    void initialize( );

    /* this constructor creates a lighter copy of the event object.
     * this constructor may be used when only the event ID is used. */
    Event( const OrderID& id ):
      eventId( id ),
      eventType( UNDEFEVENT ),
      eventContexts(),
      eventSnapshotContexts(),
      eventContextMappingVersion( 0 ),
      eventContextStructureVersion(0 ),
      eventOpType( mace::Event::EVENT_OP_WRITE ) { 
        //eventOrderTickets[id.ctxId] = id.ticket;
    }

    void initialize2( const OrderID& eventId, const uint8_t& event_op_type, const mace::string& create_ctx_name, const mace::string& target_ctx_name, 
        const mace::string& eventMethodType, const int8_t eventType, const uint64_t contextMappingVersion, const uint64_t contextStructureVersion);
    void initialize3( const mace::string& migration_ctx_name, const uint64_t ctxMappingVer );

    void addServiceID(const uint8_t serviceId);

    Event& operator=(const Event& orig);

    void print(std::ostream& out) const;
    void printNode(PrintNode& pr, const std::string& name) const;

    const OrderID& getEventID() const{
        return eventId;
    }
    const int8_t getEventType() const{
        return eventType;
    }

    void commit() {
      waitToken();
      // create subevents
      if( !subevents.empty() ){
        enqueueDeferredEvents();
      }

      // WC: send deferred messages
      if( !eventMessages.empty() ){
        sendDeferredMessages();
      }
      
      // WC: execute deferred upcalls in the application
      if( !eventUpcalls.empty() ){
        executeApplicationUpcalls();
      }

      // WC: TODO: if this is a migration event, send messages to all physical nodes
      // to clean up the old context-node map.
    }
    virtual void serialize(std::string& str) const;

    virtual int deserialize(std::istream & is) throw (mace::SerializationException);

    bool deferExternalMessage( uint8_t instanceUniqueID, MaceKey const& dest,  std::string const&  message, 
        registration_uid_t const rid );

    static uint64_t getLastContextMappingVersion( )  {
        // WC: need mutex lock?
        return lastWriteContextMapping;
    }
    void deferEventRequest( uint8_t instanceUniqueID, Message* request){
      subevents.push_back( EventRequestWrapper( instanceUniqueID, request) );
    }
    void clearEventRequests(){
      for( EventRequestType::iterator it = subevents.begin(); it != subevents.end(); it++ ){
        //delete it->request;
      }
      subevents.clear();
    }
    void clearEventUpcalls(){
      for( DeferredUpcallType::iterator it = eventUpcalls.begin(); it != eventUpcalls.end(); it++ ){
        delete it->upcall;
      }
      eventUpcalls.clear();
    }
    void deferApplicationUpcalls( uint8_t sid, mace::Message* const& upcall ){
      eventUpcalls.push_back( EventUpcallWrapper(sid, upcall ) );
    }
    static void setLastContextMappingVersion( const uint64_t newVersion )  {
         lastWriteContextMapping = newVersion;
    }

    void clearContexts(){
      eventContexts.clear();
    }
    void clearSnapshotContexts(){
      eventSnapshotContexts.clear();
    }
    void clearEventOrderRecords(){
      //eventOrderRecords.clear();
    }
    
    void null_func();
    void executeApplicationUpcalls();

    void checkSubEvent() const;
    bool checkParentChildRelation(mace::string const& parentContextName, mace::string const& childContextName ) const;
    void enqueueOwnershipOpInfo( EventOperationInfo const& opInfo );
    //void removePreEvent(const uint8_t& serviceId, OrderID const& preId);
    mace::string getParentContextName(mace::string const& childContextName) const;
    
private:
    void sendDeferredMessages();
    void enqueueDeferredEvents();
    void createToken(){
      // chuangw: create a token which is used by the subevents.
    }
    void waitToken(){
      // chuangw:
      // check if the token has arrived,
      // if so, remove that token from record,
      // otherwise, wait to be unlocked.
    }
    void unlockToken(){
      // chuangw:
      // if an event is waiting at this token, signal it,
      // otherwise, store this token
    }
private:
    static uint64_t nextTicketNumber;
    static uint64_t lastWriteContextMapping;

public:
    OrderID eventId;
    mace::string create_ctx_name;
    mace::string target_ctx_name;
    mace::string eventMethodType;

    EventOperationInfo eventOpInfo;
    EventOperationInfo eventOpInfoCopy;
    EventOrderInfo eventOrderInfo;
    
    int8_t  eventType;
    EventContextType eventContexts;
    EventSnapshotContextType eventSnapshotContexts;
    uint64_t eventContextMappingVersion;
    EventRequestType subevents;
    DeferredMessageType eventMessages;
    DeferredUpcallType eventUpcalls;

    bool createCommitFlag;
    uint64_t eventContextStructureVersion;

    uint64_t externalMessageTicket;
    
    mace::vector<mace::EventOperationInfo> ownershipOps;
    
    uint8_t eventOpType;

    static bool isExit;
    static OrderID exitEventId;

    static const int8_t STARTEVENT = 0;
    static const int8_t ENDEVENT   = 1;
    static const int8_t TIMEREVENT = 2;
    static const int8_t ASYNCEVENT = 3;
    static const int8_t UPCALLEVENT= 4;
    static const int8_t DOWNCALLEVENT= 5;
    static const int8_t MIGRATIONEVENT = 6;
    static const int8_t NEWCONTEXTEVENT = 7;
    static const int8_t HEADMIGRATIONEVENT = 8;
    static const int8_t DELETECONTEXT = 9;
    static const int8_t UNDEFEVENT = 10;
    static const int8_t BROADCASTEVENT = 11;
    static const int8_t ROUTINEEVENT = 12;
    static const int8_t ALLOCATE_CTX_OBJECT = 13;
    static const int8_t EXTERNALMESSAGE = 14;
    static const int8_t BROADCASTCOMMITEVENT = 15;
};



}
#endif
