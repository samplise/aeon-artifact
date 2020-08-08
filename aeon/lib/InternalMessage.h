#ifndef _INTERNAL_MESSAGE_H
#define _INTERNAL_MESSAGE_H

#include "Serializable.h"
#include "Printable.h"
#include "m_map.h"
#include "ContextMapping.h"
#include "Message.h"
#include "SpecialMessage.h"
#include <limits>
#include "Log.h" 
#include "mace-macros.h"

namespace mace {
  typedef  AsyncEvent_Message ApplicationUpcall;
  class InvalidInternalMessageException : public SerializationException {
  public:
    InvalidInternalMessageException(const std::string& m) : SerializationException(m) { }
    virtual ~InvalidInternalMessageException() throw() {}
    void rethrow() const { throw *this; }
  };

  class ExternalCommControl_Message: public InternalMessageHelper, virtual public PrintPrintable{
  public:
    static const uint8_t ADD_CLIENT_MAPPING = 0;
        
  private:
  struct ExternalCommControl_struct{
    uint8_t control_type;
    uint32_t externalCommId;
    uint32_t externalCommContextId;
  };
  ExternalCommControl_struct* _data_store_;
  mutable size_t serializedByteSize;
  mutable std::string serializedCache;
  public:
    ExternalCommControl_Message( ): _data_store_( new ExternalCommControl_struct() ), 
      serializedByteSize(0) , 
      control_type( _data_store_->control_type ),
      externalCommId( _data_store_->externalCommId),
      externalCommContextId( _data_store_->externalCommContextId ) { }

    ExternalCommControl_Message( const uint8_t& control_type, uint32_t const& externalCommId, uint32_t const& externalCommContextId): 
      _data_store_( NULL ), serializedByteSize(0) , control_type( control_type ), 
      externalCommId(externalCommId), externalCommContextId( externalCommContextId ) { }
    // WC: C++03 forbids calling a constructor from another constructor.
    ExternalCommControl_Message( InternalMessageHelper const* o): _data_store_( new ExternalCommControl_struct() ), 
        serializedByteSize(0) , 
        control_type( _data_store_->control_type ), externalCommId( _data_store_->externalCommId), externalCommContextId( _data_store_->externalCommContextId) {
      ExternalCommControl_Message const* orig_helper = static_cast< ExternalCommControl_Message const* >( o );
     _data_store_->control_type = orig_helper->control_type;
     _data_store_->externalCommId = orig_helper->externalCommId;
     _data_store_->externalCommContextId = orig_helper->externalCommContextId;
     
    }
    
    ~ExternalCommControl_Message(){ delete _data_store_; _data_store_ = NULL; }
 
    void print(std::ostream& __out) const {
      __out << "ExternalCommControl(";
      __out << "control_type=";  mace::printItem(__out, &(control_type));
      __out << ", ";
      __out << "externalCommId="; mace::printItem(__out, &(externalCommId)); __out << ", ";
      __out << "externalCommContextId="; mace::printItem(__out, &(externalCommContextId)); 
      __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      mace::serialize(str, &control_type);
      mace::serialize(str, &externalCommId);
      mace::serialize(str, &externalCommContextId);
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->control_type);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->externalCommId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->externalCommContextId);
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    uint8_t const& control_type;
    uint32_t const& externalCommId;
    uint32_t const& externalCommContextId;
  };

  class BroadcastControl_Message: public InternalMessageHelper, virtual public PrintPrintable{
  public:
    static const uint8_t BROADCAST_DOWNGRADE = 0;
     
  private:
  struct BroadcastControl_struct{
    uint8_t control_type;
    mace::string targetContextName;
    mace::string parentContextName;
    mace::OrderID eventId;
    mace::OrderID bEventId;
    mace::map< mace::string, mace::set<mace::string> > cpRelations;
    mace::set< mace::string > targetContextNames;
  };
  BroadcastControl_struct* _data_store_;
  mutable size_t serializedByteSize;
  mutable std::string serializedCache;
  public:
    BroadcastControl_Message( ): _data_store_( new BroadcastControl_struct() ), 
      serializedByteSize(0) , 
      control_type( _data_store_->control_type ),
      targetContextName( _data_store_->targetContextName ),
      parentContextName(_data_store_->parentContextName ),
      eventId( _data_store_->eventId),
      bEventId( _data_store_->bEventId ),
      cpRelations( _data_store_->cpRelations ),
      targetContextNames(_data_store_->targetContextNames) { }

    BroadcastControl_Message( const uint8_t& control_type, mace::string const& targetContextName, mace::string const& parentContextName, mace::OrderID const& eventId, mace::OrderID const& bEventId, 
        mace::map<mace::string, mace::set<mace::string> >const& cpRelations, mace::set< mace::string > const& targetContextNames): _data_store_( NULL ), serializedByteSize(0) , 
        control_type( control_type ), targetContextName(targetContextName), parentContextName(parentContextName), eventId(eventId),
        bEventId(bEventId), cpRelations(cpRelations), targetContextNames(targetContextNames) { }
    // WC: C++03 forbids calling a constructor from another constructor.
    BroadcastControl_Message( InternalMessageHelper const* o): _data_store_( new BroadcastControl_struct() ), 
        serializedByteSize(0) , 
        control_type( _data_store_->control_type ), targetContextName(_data_store_->targetContextName), parentContextName(_data_store_->parentContextName), 
        eventId( _data_store_->eventId), bEventId(_data_store_->bEventId), cpRelations(_data_store_->cpRelations), targetContextNames(_data_store_->targetContextNames) {
      
      BroadcastControl_Message const* orig_helper = static_cast< BroadcastControl_Message const* >( o );
      _data_store_->control_type = orig_helper->control_type;
      _data_store_->targetContextName = orig_helper->targetContextName;
      _data_store_->parentContextName = orig_helper->parentContextName;
      _data_store_->eventId = orig_helper->eventId;
      _data_store_->bEventId = orig_helper->bEventId;
      _data_store_->cpRelations = orig_helper->cpRelations;
      _data_store_->targetContextNames = orig_helper->targetContextNames;
    }
    ~BroadcastControl_Message(){ delete _data_store_; _data_store_ = NULL; }
 
    void print(std::ostream& __out) const {
      __out << "BroadcastControl(";
      __out << "control_type=";  mace::printItem(__out, &(control_type));
      __out << ", ";
      __out << "targetContextName="; mace::printItem(__out, &(targetContextName));
      __out << ", ";
      __out << "parentContextName="; mace::printItem(__out, &(parentContextName));
      __out << ", ";
      __out << "eventId="; mace::printItem(__out, &(eventId));
      __out << ", ";
      __out << "bEventId="; mace::printItem(__out, &(bEventId));
      __out << ", ";
      __out << "cpRelations="; mace::printItem(__out, &(cpRelations));
      __out << ", ";
      __out << "targetContextNames="; mace::printItem(__out, &(targetContextNames) );
      __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      mace::serialize(str, &control_type);
      mace::serialize(str, &targetContextName);
      mace::serialize(str, &parentContextName);
      mace::serialize(str, &eventId);
      mace::serialize(str, &bEventId);
      mace::serialize(str, &cpRelations);
      mace::serialize(str, & targetContextNames);
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->control_type);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->targetContextName);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->parentContextName);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->bEventId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->cpRelations);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->targetContextNames);
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    uint8_t const& control_type;
    mace::string const& targetContextName;
    mace::string const& parentContextName;
    mace::OrderID const& eventId;
    mace::OrderID const& bEventId;
    mace::map<mace::string, mace::set<mace::string> > const& cpRelations;
    mace::set<mace::string> const& targetContextNames;
  };

  class MigrationControl_Message: public InternalMessageHelper, virtual public PrintPrintable{
  public:
    static const uint8_t MIGRATION_DONE = 0;
    static const uint8_t MIGRATION_PREPARE_RECV_CONTEXTS = 1;
    static const uint8_t MIGRATION_PREPARE_RECV_CONTEXTS_RESPONSE = 2;
    static const uint8_t MIGRATION_UPDATE_CONTEXTMAPPING = 3;
    static const uint8_t MIGRATION_RELEASE_CONTEXTMAPPING = 4;
    static const uint8_t MIGRATION_CTX_REQUEST = 5;
    
  private:
  struct MigrationControl_struct{
    uint8_t control_type;
    uint64_t ticket;
    mace::map<uint32_t, mace::string> migrate_contexts;
    mace::ContextMapping ctxMapping;
    mace::MaceAddr srcAddr;
  };
  MigrationControl_struct* _data_store_;
  mutable size_t serializedByteSize;
  mutable std::string serializedCache;
  public:
    MigrationControl_Message( ): _data_store_( new MigrationControl_struct() ), 
      serializedByteSize(0) , 
      control_type( _data_store_->control_type ),
      ticket( _data_store_->ticket),
      migrate_contexts( _data_store_->migrate_contexts ),
      ctxMapping( _data_store_->ctxMapping ),
      srcAddr( _data_store_->srcAddr ) { }

    MigrationControl_Message( const uint8_t& control_type, const uint64_t& ticket, mace::map<uint32_t, mace::string> const& migrate_contexts, 
      mace::ContextMapping const& ctxMapping, mace::MaceAddr const& srcAddr ): _data_store_( NULL ), 
      serializedByteSize(0) , control_type( control_type ), ticket(ticket), migrate_contexts(migrate_contexts), ctxMapping(ctxMapping),
      srcAddr( srcAddr ) { }
    // WC: C++03 forbids calling a constructor from another constructor.
    MigrationControl_Message( InternalMessageHelper const* o): _data_store_( new MigrationControl_struct() ), 
        serializedByteSize(0) , 
        control_type( _data_store_->control_type ), ticket( _data_store_->ticket), migrate_contexts(_data_store_->migrate_contexts), 
        ctxMapping(_data_store_->ctxMapping), srcAddr( _data_store_->srcAddr) {
      MigrationControl_Message const* orig_helper = static_cast< MigrationControl_Message const* >( o );
     _data_store_->control_type = orig_helper->control_type;
     _data_store_->ticket = orig_helper->ticket;
     _data_store_->migrate_contexts = orig_helper->migrate_contexts;
     _data_store_->ctxMapping = orig_helper->ctxMapping;
     _data_store_->srcAddr = orig_helper->srcAddr;
    }
    ~MigrationControl_Message(){ delete _data_store_; _data_store_ = NULL; }
 
    void print(std::ostream& __out) const {
      __out << "MigrationControl(";
      __out << "control_type=";  mace::printItem(__out, &(control_type));
      __out << ", ";
      __out << "ticket="; mace::printItem(__out, &(ticket));
      __out << ", ";
      __out << "migrate_contexts="; mace::printItem(__out, &(migrate_contexts));
      __out << ", ";
      __out << "ctxMapping="; mace::printItem(__out, &(ctxMapping));
      __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      mace::serialize(str, &control_type);
      mace::serialize(str, &ticket);
      mace::serialize(str, &migrate_contexts);
      mace::serialize(str, &ctxMapping);
      mace::serialize(str, &srcAddr);
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->control_type);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->ticket);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->migrate_contexts);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->ctxMapping);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->srcAddr);
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    uint8_t const& control_type;
    uint64_t const& ticket;
    mace::map<uint32_t, mace::string> const& migrate_contexts;
    mace::ContextMapping const& ctxMapping;
    mace::MaceAddr const& srcAddr;
  };

  class LockRequest;
  typedef mace::map< mace::string, mace::vector<mace::LockRequest> > EventLockQueue;

  class ContextOwnershipControl_Message: public InternalMessageHelper, virtual public PrintPrintable{
  public:
    static const uint8_t OWNERSHIP_MODIFY = 1;
    static const uint8_t OWNERSHIP_UPDATE = 2;
    static const uint8_t OWNERSHIP_UPDATE_REQUEST = 3;
    static const uint8_t OWNERSHIP_MODIFY_DONE = 4;
    static const uint8_t OWNERSHIP_UPDATE_DONE = 5;
    static const uint8_t OWNERSHIP_AND_DOMINATOR_UPDATE = 6;
    static const uint8_t OWNERSHIP_UPDATE_REPLY = 7;

  private:
  struct ContextOwnershipControl_struct{
    uint8_t type;

    // modify
    mace::vector< mace::EventOperationInfo > ownershipOpInfos;
    mace::string dest_contextName;
    mace::string src_contextName;
    mace::OrderID eventId;
    
    // update
    mace::vector< mace::pair<mace::string, mace::string> > ownerships;
    mace::map< mace::string, uint64_t > versions;
    mace::set<  mace::string > contextSet;
    mace::vector< mace::string >  contextVector;
  };

  ContextOwnershipControl_struct* _data_store_;
  mutable size_t serializedByteSize;
  mutable std::string serializedCache;
  public:
    ContextOwnershipControl_Message( ): _data_store_( new ContextOwnershipControl_struct() ), 
      serializedByteSize(0) , 
      type( _data_store_->type ),
      ownershipOpInfos( _data_store_->ownershipOpInfos ),
      dest_contextName( _data_store_->dest_contextName ),
      src_contextName( _data_store_->src_contextName ),
      eventId( _data_store_->eventId ),
      ownerships( _data_store_->ownerships ),
      versions( _data_store_->versions ),
      contextSet( _data_store_->contextSet ),
      contextVector( _data_store_->contextVector ){ }

    ContextOwnershipControl_Message( const uint8_t& type, mace::vector<mace::EventOperationInfo> const& ownershipOpInfos, 
      mace::string const& dest_contextName, mace::string const& src_contextName, mace::OrderID const& eventId,  
      mace::vector< mace::pair<mace::string, mace::string> > const& ownerships, const map<mace::string, uint64_t>& versions, 
      mace::set<mace::string> const& contextSet,
      mace::vector<mace::string> const& contextVector ): _data_store_( NULL ), serializedByteSize(0) , type( type ), 
      ownershipOpInfos(ownershipOpInfos), dest_contextName(dest_contextName), src_contextName(src_contextName), eventId(eventId), 
      ownerships(ownerships), versions(versions), contextSet(contextSet), contextVector(contextVector) { 

        ADD_SELECTORS("ContextOwnershipControl_Message::contructor");
        // macedbg(1) << "Message type: " << (uint16_t)type <<  Log::endl;
    }
    
    // WC: C++03 forbids calling a constructor from another constructor.
    ContextOwnershipControl_Message( InternalMessageHelper const* o): _data_store_( new ContextOwnershipControl_struct() ), 
        serializedByteSize(0), type( _data_store_->type ),  ownershipOpInfos( _data_store_->ownershipOpInfos ),
        dest_contextName( _data_store_->dest_contextName ), src_contextName( _data_store_->src_contextName ),
        eventId( _data_store_->eventId ), ownerships( _data_store_->ownerships ), versions( _data_store_->versions ), 
        contextSet( _data_store_->contextSet ), contextVector( _data_store_->contextVector ) {

      ContextOwnershipControl_Message const* orig_helper = static_cast< ContextOwnershipControl_Message const* >( o );
      _data_store_->type = orig_helper->type;
      _data_store_->ownershipOpInfos = orig_helper->ownershipOpInfos;
      _data_store_->dest_contextName = orig_helper->dest_contextName;
      _data_store_->src_contextName = orig_helper->src_contextName;
      _data_store_->eventId = orig_helper->eventId;
      _data_store_->ownerships = orig_helper->ownerships;
      _data_store_->versions = orig_helper->versions;
      _data_store_->contextSet = orig_helper->contextSet;
      _data_store_->contextVector = orig_helper->contextVector;
    }
    ~ContextOwnershipControl_Message(){ delete _data_store_; _data_store_ = NULL; }
 
    void print(std::ostream& __out) const {
      uint64_t printType = (uint64_t)type;
      __out << "ContextOwnershipControl(";
      __out << "type=";  mace::printItem(__out, &(printType)); __out << ", ";
      __out << "ownershipOpInfos=";  mace::printItem(__out, &(ownershipOpInfos)); __out << ", ";
      __out << "dest_contextName=";  mace::printItem(__out, &(dest_contextName)); __out << ", ";
      __out << "src_contextName=";  mace::printItem(__out, &(src_contextName)); __out << ", ";
      __out << "eventId=";  mace::printItem(__out, &(eventId)); __out << ", ";
      __out << "ownerships=";  mace::printItem(__out, &(ownerships)); __out << ", ";
      __out << "versions=";  mace::printItem(__out, &(versions)); __out << ", ";
      __out << "contextSet=";  mace::printItem(__out, &(contextSet)); __out << ", ";
      __out << "contextVector=";  mace::printItem(__out, &(contextVector));
      __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      mace::serialize(str, &type);
      mace::serialize(str, &ownershipOpInfos);
      mace::serialize(str, &dest_contextName);
      mace::serialize(str, &src_contextName);
      mace::serialize(str, &eventId);
      mace::serialize(str, &ownerships);
      mace::serialize(str, &versions);
      mace::serialize(str, &contextSet);
      mace::serialize(str, &contextVector);
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->type);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->ownershipOpInfos);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->dest_contextName);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->src_contextName);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->ownerships);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->versions);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextSet);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextVector);
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    
    uint8_t const& type;

    // modify
    mace::vector< mace::EventOperationInfo > const& ownershipOpInfos;
    mace::string const& dest_contextName;
    mace::string const& src_contextName;
    mace::OrderID const& eventId;

    // update
    mace::vector< mace::pair<mace::string, mace::string> > const& ownerships;
    mace::map<mace::string, uint64_t> const& versions;
    mace::set<  mace::string > const& contextSet;
    mace::vector< mace::string > const& contextVector;
  };

  class CommitDone_Message: public InternalMessageHelper, virtual public PrintPrintable{
  private:
  struct CommitDone_struct{
    mace::string create_ctx_name;
    mace::string target_ctx_name;
    mace::OrderID eventId;
    mace::set<mace::string> coaccessContexts;
  };
  CommitDone_struct* _data_store_;
  mutable size_t serializedByteSize;
  mutable std::string serializedCache;
  public:
    CommitDone_Message( ): _data_store_( new CommitDone_struct() ), 
      serializedByteSize(0) , 
      create_ctx_name(_data_store_->create_ctx_name), 
      target_ctx_name( _data_store_->target_ctx_name ), 
      eventId( _data_store_->eventId ),
      coaccessContexts( _data_store_->coaccessContexts ) { }

    CommitDone_Message( mace::string const& create_ctx_name, mace::string const& target_ctx_name, mace::OrderID const& eventId, 
      mace::set<mace::string> const& coaccessContexts ): _data_store_( NULL ), serializedByteSize(0) , create_ctx_name( create_ctx_name ), 
      target_ctx_name(target_ctx_name), eventId( eventId ), coaccessContexts( coaccessContexts ) { }
    // WC: C++03 forbids calling a constructor from another constructor.
    CommitDone_Message( InternalMessageHelper const* o): _data_store_( new CommitDone_struct() ), 
      serializedByteSize(0) , 
      create_ctx_name( _data_store_->create_ctx_name ),
      target_ctx_name(_data_store_->target_ctx_name), 
      eventId( _data_store_->eventId ),
      coaccessContexts( _data_store_->coaccessContexts ) {
     CommitDone_Message const* orig_helper = static_cast< CommitDone_Message const* >( o );
     _data_store_->target_ctx_name = orig_helper->target_ctx_name;
     _data_store_->create_ctx_name = orig_helper->create_ctx_name;
     _data_store_->eventId = orig_helper->eventId;
     _data_store_->coaccessContexts = orig_helper->coaccessContexts;
    }
    ~CommitDone_Message(){ delete _data_store_; _data_store_ = NULL; }
 
    void print(std::ostream& __out) const {
      __out << "CommitDone(";
          __out << "targetContextName=";  mace::printItem(__out, &(target_ctx_name));
          __out << ", ";
          __out << "createContextName=";  mace::printItem(__out, &(create_ctx_name));
          __out << ", ";
          __out << "eventID=";  mace::printItem(__out, &(eventId));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      mace::serialize(str, &create_ctx_name);
      mace::serialize(str, &target_ctx_name);
      mace::serialize(str, &eventId);
      mace::serialize(str, &coaccessContexts);
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->create_ctx_name);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->target_ctx_name);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->coaccessContexts);
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    mace::string const & create_ctx_name;
    mace::string const & target_ctx_name;
    mace::OrderID const & eventId;
    mace::set<mace::string> const & coaccessContexts;
  };

  class AllocateContextObject_Message: public InternalMessageHelper, virtual public PrintPrintable{
  private:
  struct AllocateContextObject_struct{
    MaceAddr destNode;
    mace::map< uint32_t, mace::string > contextId;
    mace::OrderID eventId;
    mace::ContextMapping contextMapping;
    int8_t eventType;
    uint64_t version;
    mace::vector< mace::pair<mace::string, mace::string> > ownershipPairs;
    mace::map< mace::string, uint64_t> vers;
  };
  AllocateContextObject_struct* _data_store_;
  mutable size_t serializedByteSize;
  mutable std::string serializedCache;
  public:
    AllocateContextObject_Message( ): _data_store_( new AllocateContextObject_struct() ), 
      serializedByteSize(0) , 
      destNode(_data_store_->destNode), 
      contextId( _data_store_->contextId ), 
      eventId( _data_store_->eventId ), 
      contextMapping( _data_store_->contextMapping ), 
      eventType( _data_store_->eventType ),
      version( _data_store_->version ),
      ownershipPairs( _data_store_->ownershipPairs),
      vers( _data_store_->vers ){ }

    AllocateContextObject_Message( MaceAddr const & destNode, mace::map< uint32_t, mace::string > const & ContextID, mace::OrderID const & eventID, 
        mace::ContextMapping const & contextMapping, int8_t const & eventType, const uint64_t & version,
        const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs,
        const mace::map< mace::string, uint64_t>& vers ): _data_store_( NULL ), 
        serializedByteSize(0) , destNode(destNode), contextId( ContextID ), eventId( eventID ), contextMapping( contextMapping ), 
        eventType( eventType ), version( version ), ownershipPairs(ownershipPairs), vers(vers) { }
    // WC: C++03 forbids calling a constructor from another constructor.
    AllocateContextObject_Message( InternalMessageHelper const* o): _data_store_( new AllocateContextObject_struct() ), 
      serializedByteSize(0) , 
      destNode(_data_store_->destNode), 
      contextId( _data_store_->contextId ), 
      eventId( _data_store_->eventId ), 
      contextMapping( _data_store_->contextMapping ), 
      eventType( _data_store_->eventType ),
      version( _data_store_->version ),
      ownershipPairs( _data_store_->ownershipPairs),
      vers( _data_store_->vers ) {
     AllocateContextObject_Message const* orig_helper = static_cast< AllocateContextObject_Message const* >( o );
     _data_store_->destNode = orig_helper->destNode;
     _data_store_->contextId = orig_helper->contextId;
     _data_store_->eventId = orig_helper->eventId;
     _data_store_->contextMapping = orig_helper->contextMapping;
     _data_store_->eventType = orig_helper->eventType;
     _data_store_->version = orig_helper->version;
     _data_store_->ownershipPairs = orig_helper->ownershipPairs;
     _data_store_->vers = orig_helper->vers;
    }
    ~AllocateContextObject_Message(){ delete _data_store_; _data_store_ = NULL; }
 
    void print(std::ostream& __out) const {
      uint64_t eventType64 = (uint64_t) eventType;
      __out << "AllocateContextObject(";
          __out << "destNode=";  mace::printItem(__out, &(destNode));
          __out << ", ";
          __out << "ContextID=";  mace::printItem(__out, &(contextId));
          __out << ", ";
          __out << "eventID=";  mace::printItem(__out, &(eventId));
          __out << ", ";
          __out << "contextMapping=";  mace::printItem(__out, &(contextMapping));
          __out << ", ";
          __out << "eventType=";  mace::printItem(__out, &( eventType64 ));
          __out << ", ";
          __out << "version="; mace::printItem(__out, &(version));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      mace::serialize(str, &destNode);
      mace::serialize(str, &contextId);
      mace::serialize(str, &eventId);\
      mace::serialize(str, &contextMapping);
      mace::serialize(str, &eventType);
      mace::serialize(str, &version);
      mace::serialize(str, &ownershipPairs );
      mace::serialize(str, &vers );
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->destNode);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextMapping);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventType);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->version);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->ownershipPairs);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->vers);
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    MaceAddr const & destNode;
    mace::map< uint32_t, mace::string > const & contextId;
    mace::OrderID const & eventId;
    mace::ContextMapping const & contextMapping;
    int8_t const & eventType;
    uint64_t const & version;
    mace::vector< mace::pair<mace::string, mace::string> > const & ownershipPairs;
    mace::map< mace::string, uint64_t> const & vers;

  };

	class AllocateContextObjReq_Message: public InternalMessageHelper, virtual PrintPrintable{
	private:
		struct AllocateContextObjReq_struct{
			mace::string createContextName;
			mace::OrderID eventId;
      mace::vector< mace::pair<mace::string, mace::string> > ownershipPairs;
      mace::map< mace::string, uint64_t> vers;
    };

		AllocateContextObjReq_struct* _data_store_;
		mutable size_t serializedByteSize;
		mutable std::string serializedCache;
	public:
		AllocateContextObjReq_Message(): _data_store_( new AllocateContextObjReq_struct() ), 
			serializedByteSize(0), 
			createContextName(_data_store_->createContextName), 
			eventId( _data_store_->eventId ),
      ownershipPairs( _data_store_->ownershipPairs ),
      vers( _data_store_->vers ) {}

		AllocateContextObjReq_Message( const mace::string& ctxName, const OrderID& eventId, 
      const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs,
      const mace::map< mace::string, uint64_t>& vers  ): 
      _data_store_(NULL), serializedByteSize(0), createContextName(ctxName), eventId(eventId), ownershipPairs(ownershipPairs), vers(vers) {}

		AllocateContextObjReq_Message( InternalMessageHelper const*o): _data_store_( new AllocateContextObjReq_struct() ), 
			 serializedByteSize(0), createContextName(_data_store_->createContextName), eventId(_data_store_->eventId),
       ownershipPairs(_data_store_->ownershipPairs), vers(_data_store_->vers) {
			
      AllocateContextObjReq_Message const* orig_helper = static_cast< AllocateContextObjReq_Message const* >(o);
			_data_store_->createContextName = orig_helper->createContextName;
			_data_store_->eventId = orig_helper->eventId;     
      _data_store_->ownershipPairs = orig_helper->ownershipPairs;
      _data_store_->vers = orig_helper->vers;
		}

		~AllocateContextObjReq_Message() {
			delete _data_store_;
			_data_store_ = NULL;
		}

		void print(std::ostream& __out) const {
			__out << "AllocateContextObjReq(";
			__out << "createContextName="; mace::printItem(__out, &(createContextName)); __out << ", ";
			__out << "eventId="; mace::printItem(__out, &(eventId)); __out << ", ";
      __out << ")";
		}

		void serialize(std::string& str) const {
			if(!serializedCache.empty()){
				str.append(serializedCache);
				return;
			}
			size_t initsize = str.size();

			mace::serialize(str, &createContextName);
			mace::serialize(str, &eventId);
      mace::serialize(str, &ownershipPairs);
      mace::serialize(str, &vers);
      bool __false = false;
			mace::serialize(str, &__false);

			if(initsize == 0){
				serializedCache = str;
			}
			serializedByteSize = str.size() - initsize;
		}

		int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
			serializedByteSize = 0;

			serializedByteSize += mace::deserialize(__mace_in, &_data_store_->createContextName);
			serializedByteSize += mace::deserialize(__mace_in, &_data_store_->eventId);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->ownershipPairs);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->vers);
      bool __unused;
			serializedByteSize += mace::deserialize(__mace_in, &__unused);
			return serializedByteSize;
		}

		mace::string const & createContextName;
		OrderID const & eventId;
    mace::vector< mace::pair<mace::string, mace::string> > const& ownershipPairs;
    mace::map< mace::string, uint64_t> const& vers;
	};

  class ContextMappingUpdateReq_Message: public InternalMessageHelper, virtual PrintPrintable {
  private:
    struct ContextMappingUpdateReq_struct{
      uint64_t expectVersion;
    };

    ContextMappingUpdateReq_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    ContextMappingUpdateReq_Message(): _data_store_( new ContextMappingUpdateReq_struct() ), 
      serializedByteSize(0), 
      expectVersion(_data_store_->expectVersion) {}

    ContextMappingUpdateReq_Message(const uint64_t& expectVersion): _data_store_(NULL), serializedByteSize(0), expectVersion(expectVersion) {}

    ContextMappingUpdateReq_Message( InternalMessageHelper const*o): _data_store_( new ContextMappingUpdateReq_struct() ), 
      serializedByteSize(0), 
      expectVersion(_data_store_->expectVersion) {
        ContextMappingUpdateReq_Message const* orig_helper = static_cast< ContextMappingUpdateReq_Message const* >(o);
        _data_store_->expectVersion = orig_helper->expectVersion;
    }

    ~ContextMappingUpdateReq_Message() {
      delete _data_store_;
      _data_store_ = NULL;
    }

    void print(std::ostream& __out) const {
      __out << "ContextMappingUpdateReq(";
      __out << "expectVersion="; mace::printItem(__out, &(expectVersion));
      __out << ")";
    }

    void serialize(std::string& str) const {
      if(!serializedCache.empty()){
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();

      mace::serialize(str, &expectVersion);
      bool __false = false;
      mace::serialize(str, &__false);

      if(initsize == 0){
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }

    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;

      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->expectVersion);
      bool __unused;
      serializedByteSize += mace::deserialize(__mace_in, &__unused);
      return serializedByteSize;
    }

    uint64_t const & expectVersion;
  };

  class ContextMappingUpdateSuggest_Message: public InternalMessageHelper, virtual PrintPrintable {
  private:
    struct ContextMappingUpdateSuggest_struct{
      uint64_t expectVersion;
    };

    ContextMappingUpdateSuggest_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    ContextMappingUpdateSuggest_Message(): _data_store_( new ContextMappingUpdateSuggest_struct() ), 
      serializedByteSize(0), 
      expectVersion(_data_store_->expectVersion) {}

    ContextMappingUpdateSuggest_Message(const uint64_t& expectVersion): _data_store_(NULL), serializedByteSize(0), expectVersion(expectVersion) {}

    ContextMappingUpdateSuggest_Message( InternalMessageHelper const*o): _data_store_( new ContextMappingUpdateSuggest_struct() ), 
      serializedByteSize(0), 
      expectVersion(_data_store_->expectVersion) {
        ContextMappingUpdateSuggest_Message const* orig_helper = static_cast< ContextMappingUpdateSuggest_Message const* >(o);
        _data_store_->expectVersion = orig_helper->expectVersion;
    }

    ~ContextMappingUpdateSuggest_Message() {
      delete _data_store_;
      _data_store_ = NULL;
    }

    void print(std::ostream& __out) const {
      __out << "ContextMappingUpdateSuggest(";
      __out << "expectVersion="; mace::printItem(__out, &(expectVersion));
      __out << ")";
    }

    void serialize(std::string& str) const {
      if(!serializedCache.empty()){
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();

      mace::serialize(str, &expectVersion);
      bool __false = false;
      mace::serialize(str, &__false);

      if(initsize == 0){
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }

    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;

      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->expectVersion);
      bool __unused;
      serializedByteSize += mace::deserialize(__mace_in, &__unused);
      return serializedByteSize;
    }

    uint64_t const & expectVersion;
  };

	class AllocateContextObjectResponse_Message: public InternalMessageHelper, virtual PrintPrintable{
	private:
		struct AllocateContextObjectResponse_struct {
			mace::string ctx_name;
      mace::OrderID eventId;
      bool isCreateContextEvent;
		};

		AllocateContextObjectResponse_struct* _data_store_;
		mutable size_t serializedByteSize;
		mutable std::string serializedCache;
	public:
		AllocateContextObjectResponse_Message(): _data_store_( new AllocateContextObjectResponse_struct() ), 
			serializedByteSize(0), 
			ctx_name(_data_store_->ctx_name),
      eventId(_data_store_->eventId),
      isCreateContextEvent(_data_store_->isCreateContextEvent) {}

		AllocateContextObjectResponse_Message(mace::string const& ctx_name, mace::OrderID const& eventId, bool const& isCreateContextEvent): _data_store_(NULL), 
			serializedByteSize(0),
      ctx_name(ctx_name),
      eventId(eventId),
      isCreateContextEvent(isCreateContextEvent) {}

		AllocateContextObjectResponse_Message( InternalMessageHelper const*o): _data_store_( new AllocateContextObjectResponse_struct() ), 
			serializedByteSize(0), 
      ctx_name(_data_store_->ctx_name),
      eventId(_data_store_->eventId),
      isCreateContextEvent(_data_store_->isCreateContextEvent) {
				AllocateContextObjectResponse_Message const* orig_helper = static_cast< AllocateContextObjectResponse_Message const* >(o);
				_data_store_->ctx_name = orig_helper->ctx_name;
        _data_store_->eventId = orig_helper->eventId;
        _data_store_->isCreateContextEvent = orig_helper->isCreateContextEvent;
		}

		~AllocateContextObjectResponse_Message() {
			delete _data_store_;
			_data_store_ = NULL;
		}

		void print(std::ostream& __out) const {
			__out << "AllocateContextObjResponse(";
			__out << "ccontext name = "; mace::printItem(__out, &(ctx_name));
      __out << ", ";
      __out << "eventId = "; mace::printItem(__out, &(eventId)); __out << ", ";
      __out << "isCreateContextEvent = "; mace::printItem(__out, &(isCreateContextEvent));
			__out << ")";
		}

		void serialize(std::string& str) const {
			if(!serializedCache.empty()){
				str.append(serializedCache);
				return;
			}
			size_t initsize = str.size();

			mace::serialize(str, &ctx_name);
      mace::serialize(str, &eventId);
      mace::serialize(str, &isCreateContextEvent);

			bool __false = false;
			mace::serialize(str, &__false);

			if(initsize == 0){
				serializedCache = str;
			}
			serializedByteSize = str.size() - initsize;
		}

		int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
			serializedByteSize = 0;
			
			serializedByteSize += mace::deserialize(__mace_in, &_data_store_->ctx_name);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->eventId);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->isCreateContextEvent);
			bool __unused;
			serializedByteSize += mace::deserialize(__mace_in, &__unused);
			return serializedByteSize;
		}

		mace::string const & ctx_name;
    mace::OrderID const & eventId;
    bool const& isCreateContextEvent;
	};

class UpdateContextMapping_Message: public InternalMessageHelper,  virtual public PrintPrintable{
private:
	struct UpdateContextMapping_struct{
		mace::ContextMapping ctxMapping;
    mace::string ctxName;
	};

	UpdateContextMapping_struct* _data_store_;
	mutable size_t serializedByteSize;
	mutable std::string serializedCache;
public:
	UpdateContextMapping_Message( ): _data_store_( new UpdateContextMapping_struct() ), 
		serializedByteSize(0) , 
		ctxMapping(_data_store_->ctxMapping),
    ctxName(_data_store_->ctxName) {}

	UpdateContextMapping_Message( mace::ContextMapping const & contextMapping, mace::string const& ctxName ): 
		_data_store_( NULL ),  serializedByteSize(0) ,  ctxMapping( contextMapping ), ctxName(ctxName) {}

	UpdateContextMapping_Message( InternalMessageHelper const* o): 
		_data_store_( new UpdateContextMapping_struct() ), 
		serializedByteSize(0) , 
		ctxMapping( _data_store_->ctxMapping ),
    ctxName( _data_store_->ctxName ) { 
      ADD_SELECTORS("UpdateContextMapping_Message::constructor #3");
			UpdateContextMapping_Message const* orig_helper = static_cast< UpdateContextMapping_Message const* >( o );
			_data_store_->ctxMapping = orig_helper->ctxMapping;
      _data_store_->ctxName = orig_helper->ctxName;
      //macedbg(1) << "Context: " << _data_store_->ctxName << Log::endl;
	}
  
	~UpdateContextMapping_Message(){ delete _data_store_; _data_store_ = NULL; }
		
	void print(std::ostream& __out) const {
		__out << "UpdateContextMapping(";
		__out << "contextMapping = ";  mace::printItem(__out,  &(ctxMapping));
    __out << "contextName = "; mace::printItem(__out, &(ctxName) );
    __out << ")";
	}
  
	void serialize(std::string& str) const {
		if (!serializedCache.empty()) {
			str.append(serializedCache);
			return;
		}
		size_t initsize = str.size();
									
		mace::serialize(str, &ctxMapping);
    mace::serialize(str, &ctxName);
		bool __false = false;
		mace::serialize(str,  &__false );
		if (initsize == 0) {
			serializedCache = str;
		}
		serializedByteSize = str.size() - initsize;
	}
  
	int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
		serializedByteSize = 0;

		serializedByteSize += mace::deserialize(__mace_in, &_data_store_->ctxMapping);
    serializedByteSize += mace::deserialize(__mace_in, &_data_store_->ctxName);
		bool __unused;
		serializedByteSize += mace::deserialize( __mace_in,  &__unused );
		return serializedByteSize;
  }
  
	mace::ContextMapping const& ctxMapping;
  mace::string const& ctxName;
};
  
class ContextMigrationRequest_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct ContextMigrationRequest_struct {
      MaceAddr dest ;
      mace::Event event;
      uint64_t prevContextMapVersion;
      mace::set< uint32_t > migrateContextIds;
      mace::ContextMapping contextMapping;
    };
    ContextMigrationRequest_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    ContextMigrationRequest_Message() : _data_store_(new ContextMigrationRequest_struct()), serializedByteSize(0) , dest(_data_store_->dest), event(_data_store_->event), 
      prevContextMapVersion(_data_store_->prevContextMapVersion), migrateContextIds(_data_store_->migrateContextIds), contextMapping(_data_store_->contextMapping) {}
    ContextMigrationRequest_Message( MaceAddr const & my_dest, mace::Event const & my_event, uint64_t const & my_prevContextMapVersion, mace::set< uint32_t > const & my_migrateContextIds,
      mace::ContextMapping const& my_contextMapping) : _data_store_(NULL), serializedByteSize(0), dest(my_dest), event(my_event), prevContextMapVersion(my_prevContextMapVersion), 
      migrateContextIds(my_migrateContextIds), contextMapping(my_contextMapping) {}
    ContextMigrationRequest_Message( InternalMessageHelper const* o): _data_store_( new ContextMigrationRequest_struct() ), 
      serializedByteSize(0) , dest(_data_store_->dest), event(_data_store_->event), prevContextMapVersion(_data_store_->prevContextMapVersion), 
      migrateContextIds(_data_store_->migrateContextIds), contextMapping(_data_store_->contextMapping)
      {
     ContextMigrationRequest_Message const* orig_helper = static_cast< ContextMigrationRequest_Message const* >( o );
      _data_store_->dest = orig_helper->dest;
      _data_store_->event = orig_helper->event;
      _data_store_->prevContextMapVersion = orig_helper->prevContextMapVersion;
      _data_store_->migrateContextIds = orig_helper->migrateContextIds;
      _data_store_->contextMapping = orig_helper->contextMapping;
    }
    virtual ~ContextMigrationRequest_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "ContextMigrationRequest(";
          __out << "dest=";  mace::printItem(__out, &(dest));
          __out << ", ";
          __out << "event=";  mace::printItem(__out, &(event));
          __out << ", ";
          __out << "prevContextMapVersion=";  mace::printItem(__out, &(prevContextMapVersion));
          __out << ", ";
          __out << "migrateContextIds=";  mace::printItem(__out, &(migrateContextIds));
          __out << ", ";
          __out << "contextMapping="; mace::printItem(__out, &(contextMapping));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      mace::serialize(str, &dest);
      mace::serialize(str, &event);
      mace::serialize(str, &prevContextMapVersion);
      mace::serialize(str, &migrateContextIds);
      mace::serialize(str, &contextMapping);
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) { 
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->dest);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->event);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->prevContextMapVersion);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->migrateContextIds);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextMapping);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }

    MaceAddr const & dest;
    mace::Event const & event;
    uint64_t const & prevContextMapVersion;
    mace::set< uint32_t > const & migrateContextIds;
    mace::ContextMapping const & contextMapping;
  };

  class EnqueueEventRequest_Message: public InternalMessageHelper,  virtual public PrintPrintable{
  private:
    struct EnqueueEventRequest_struct{
      mace::EventOperationInfo eventOpInfo;
      mace::string dest_contextName;
      mace::string src_contextName;
      mace::EventRequestWrapper request;
    };

    EnqueueEventRequest_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    EnqueueEventRequest_Message( ): _data_store_( new EnqueueEventRequest_struct() ), 
      serializedByteSize(0) , 
      eventOpInfo(_data_store_->eventOpInfo),
      dest_contextName(_data_store_->dest_contextName),
      src_contextName(_data_store_->src_contextName),
      request(_data_store_->request) {}

    EnqueueEventRequest_Message( mace::EventOperationInfo const & eventOpInfo, mace::string const& dest_contextName, mace::string const& src_contextName,
      mace::EventRequestWrapper const& request ): _data_store_( NULL ),  serializedByteSize(0) ,  eventOpInfo( eventOpInfo ), dest_contextName(dest_contextName),
      src_contextName(src_contextName), request( request ) {}

    EnqueueEventRequest_Message( InternalMessageHelper const* o): 
        _data_store_( new EnqueueEventRequest_struct() ), 
        serializedByteSize(0) , 
        eventOpInfo( _data_store_->eventOpInfo ), dest_contextName( _data_store_->dest_contextName ), src_contextName(_data_store_->src_contextName),
        request( _data_store_->request ) { 
      
      ADD_SELECTORS("EnqueueEventRequest_Message::constructor #3");
      EnqueueEventRequest_Message const* orig_helper = static_cast< EnqueueEventRequest_Message const* >( o );
      _data_store_->eventOpInfo = orig_helper->eventOpInfo;
      _data_store_->dest_contextName = orig_helper->dest_contextName;
      _data_store_->src_contextName = orig_helper->src_contextName;
      _data_store_->request = orig_helper->request;
    }
  
    ~EnqueueEventRequest_Message(){ delete _data_store_; _data_store_ = NULL; }
    
    void print(std::ostream& __out) const {
      __out << "EnqueueEventRequest(";
      __out << "eventOpInfo = ";  mace::printItem(__out,  &(eventOpInfo)); __out << ", ";
      __out << "dest_contextName = "; mace::printItem(__out, &(dest_contextName) ); __out << ", ";
      __out << "src_contextName = "; mace::printItem(__out, &(src_contextName) ); __out << ", ";
      __out << "eventRequest = "; mace::printItem(__out, &(request) ); 
      __out << ")";
    }
  
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
                  
      mace::serialize(str, &eventOpInfo);
      mace::serialize(str, &dest_contextName);
      mace::serialize(str, &src_contextName);
      mace::serialize(str, &request);
      bool __false = false;
      mace::serialize(str,  &__false );
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
  
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;

      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->eventOpInfo);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->dest_contextName);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->src_contextName);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->request);
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in,  &__unused );
      return serializedByteSize;
    }
  
    mace::EventOperationInfo const& eventOpInfo;
    mace::string const& dest_contextName;
    mace::string const& src_contextName;
    mace::EventRequestWrapper const& request;
  };

  class EnqueueEventReply_Message: public InternalMessageHelper,  virtual public PrintPrintable{
  private:
    struct EnqueueEventReply_struct{
      mace::EventOperationInfo eventOpInfo;
      mace::string dest_contextName;
      mace::string src_contextName;
    };

    EnqueueEventReply_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    EnqueueEventReply_Message( ): _data_store_( new EnqueueEventReply_struct() ), 
      serializedByteSize(0) , 
      eventOpInfo(_data_store_->eventOpInfo),
      dest_contextName(_data_store_->dest_contextName),
      src_contextName(_data_store_->src_contextName) {}
    
    EnqueueEventReply_Message( mace::EventOperationInfo const & eventOpInfo, mace::string const& dest_contextName, 
      mace::string const& src_contextName ): _data_store_( NULL ),  serializedByteSize(0) ,  eventOpInfo( eventOpInfo ), dest_contextName(dest_contextName),
      src_contextName(src_contextName) {}

    EnqueueEventReply_Message( InternalMessageHelper const* o): 
        _data_store_( new EnqueueEventReply_struct() ), 
        serializedByteSize(0) , eventOpInfo( _data_store_->eventOpInfo ), dest_contextName( _data_store_->dest_contextName ), 
        src_contextName(_data_store_->src_contextName) { 
      
      ADD_SELECTORS("EnqueueEventReply_Message::constructor #3");
      EnqueueEventReply_Message const* orig_helper = static_cast< EnqueueEventReply_Message const* >( o );
      _data_store_->eventOpInfo = orig_helper->eventOpInfo;
      _data_store_->dest_contextName = orig_helper->dest_contextName;
      _data_store_->src_contextName = orig_helper->src_contextName;
    }
  
    ~EnqueueEventReply_Message(){ delete _data_store_; _data_store_ = NULL; }
    
    void print(std::ostream& __out) const {
      __out << "EnqueueEventReply(";
      __out << "eventOpInfo = ";  mace::printItem(__out,  &(eventOpInfo)); __out << ", ";
      __out << "dest_contextName = "; mace::printItem(__out, &(dest_contextName) ); __out << ", ";
      __out << "src_contextName = "; mace::printItem(__out, &(src_contextName) ); 
      __out << ")";
    }
  
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
                  
      mace::serialize(str, &eventOpInfo);
      mace::serialize(str, &dest_contextName);
      mace::serialize(str, &src_contextName);
      bool __false = false;
      mace::serialize(str,  &__false );
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
  
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;

      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->eventOpInfo);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->dest_contextName);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->src_contextName);
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in,  &__unused );
      return serializedByteSize;
    }
  
    mace::EventOperationInfo const& eventOpInfo;
    mace::string const& dest_contextName;
    mace::string const& src_contextName;
  };


  class EnqueueMessageRequest_Message: public InternalMessageHelper,  virtual public PrintPrintable{
  private:
    struct EnqueueMessageRequest_struct{
      mace::EventOperationInfo eventOpInfo;
      mace::string dest_contextName;
      mace::string src_contextName;
      mace::EventMessageRecord msg;
    };

    EnqueueMessageRequest_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    EnqueueMessageRequest_Message( ): _data_store_( new EnqueueMessageRequest_struct() ), 
      serializedByteSize(0) , 
      eventOpInfo(_data_store_->eventOpInfo),
      dest_contextName(_data_store_->dest_contextName),
      src_contextName(_data_store_->src_contextName),
      msg(_data_store_->msg) {}

    EnqueueMessageRequest_Message( mace::EventOperationInfo const & eventOpInfo, mace::string const& dest_contextName, mace::string const& src_contextName,
      mace::EventMessageRecord const& msg ): _data_store_( NULL ),  serializedByteSize(0) ,  eventOpInfo( eventOpInfo ), dest_contextName(dest_contextName),
      src_contextName(src_contextName), msg( msg ) {}

    EnqueueMessageRequest_Message( InternalMessageHelper const* o): 
        _data_store_( new EnqueueMessageRequest_struct() ), 
        serializedByteSize(0) , 
        eventOpInfo( _data_store_->eventOpInfo ), dest_contextName( _data_store_->dest_contextName ), src_contextName(_data_store_->src_contextName),
        msg( _data_store_->msg ) { 
      
      ADD_SELECTORS("EnqueueMessageRequest_Message::constructor #3");
      EnqueueMessageRequest_Message const* orig_helper = static_cast< EnqueueMessageRequest_Message const* >( o );
      _data_store_->eventOpInfo = orig_helper->eventOpInfo;
      _data_store_->dest_contextName = orig_helper->dest_contextName;
      _data_store_->src_contextName = orig_helper->src_contextName;
      _data_store_->msg = orig_helper->msg;
    }
  
    ~EnqueueMessageRequest_Message(){ delete _data_store_; _data_store_ = NULL; }
    
    void print(std::ostream& __out) const {
      __out << "EnqueueMessageRequest(";
      __out << "eventOpInfo = ";  mace::printItem(__out,  &(eventOpInfo)); __out << ", ";
      __out << "dest_contextName = "; mace::printItem(__out, &(dest_contextName) ); __out << ", ";
      __out << "src_contextName = "; mace::printItem(__out, &(src_contextName) ); __out << ", ";
      __out << "msg = "; mace::printItem(__out, &(msg) ); 
      __out << ")";
    }
  
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
                  
      mace::serialize(str, &eventOpInfo);
      mace::serialize(str, &dest_contextName);
      mace::serialize(str, &src_contextName);
      mace::serialize(str, &msg);
      bool __false = false;
      mace::serialize(str,  &__false );
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
  
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;

      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->eventOpInfo);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->dest_contextName);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->src_contextName);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->msg);
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in,  &__unused );
      return serializedByteSize;
    }
  
    mace::EventOperationInfo const& eventOpInfo;
    mace::string const& dest_contextName;
    mace::string const& src_contextName;
    mace::EventMessageRecord const& msg;
  };

  class EnqueueMessageReply_Message: public InternalMessageHelper,  virtual public PrintPrintable{
  private:
    struct EnqueueMessageReply_struct{
      mace::EventOperationInfo eventOpInfo;
      mace::string dest_contextName;
      mace::string src_contextName;
    };

    EnqueueMessageReply_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    EnqueueMessageReply_Message( ): _data_store_( new EnqueueMessageReply_struct() ), 
      serializedByteSize(0) , 
      eventOpInfo(_data_store_->eventOpInfo),
      dest_contextName(_data_store_->dest_contextName),
      src_contextName(_data_store_->src_contextName) {}
    
    EnqueueMessageReply_Message( mace::EventOperationInfo const & eventOpInfo, mace::string const& dest_contextName, 
      mace::string const& src_contextName ): _data_store_( NULL ),  serializedByteSize(0) ,  eventOpInfo( eventOpInfo ), dest_contextName(dest_contextName),
      src_contextName(src_contextName) {}

    EnqueueMessageReply_Message( InternalMessageHelper const* o): 
        _data_store_( new EnqueueMessageReply_struct() ), 
        serializedByteSize(0) , eventOpInfo( _data_store_->eventOpInfo ), dest_contextName( _data_store_->dest_contextName ), 
        src_contextName(_data_store_->src_contextName) { 
      
      ADD_SELECTORS("EnqueueMessageReply_Message::constructor #3");
      EnqueueMessageReply_Message const* orig_helper = static_cast< EnqueueMessageReply_Message const* >( o );
      _data_store_->eventOpInfo = orig_helper->eventOpInfo;
      _data_store_->dest_contextName = orig_helper->dest_contextName;
      _data_store_->src_contextName = orig_helper->src_contextName;
    }
  
    ~EnqueueMessageReply_Message(){ delete _data_store_; _data_store_ = NULL; }
    
    void print(std::ostream& __out) const {
      __out << "EnqueueMessageReply(";
      __out << "eventOpInfo = ";  mace::printItem(__out,  &(eventOpInfo)); __out << ", ";
      __out << "dest_contextName = "; mace::printItem(__out, &(dest_contextName) ); __out << ", ";
      __out << "src_contextName = "; mace::printItem(__out, &(src_contextName) ); 
      __out << ")";
    }
  
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
                  
      mace::serialize(str, &eventOpInfo);
      mace::serialize(str, &dest_contextName);
      mace::serialize(str, &src_contextName);
      bool __false = false;
      mace::serialize(str,  &__false );
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
  
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;

      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->eventOpInfo);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->dest_contextName);
      serializedByteSize += mace::deserialize(__mace_in, &_data_store_->src_contextName);
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in,  &__unused );
      return serializedByteSize;
    }
  
    mace::EventOperationInfo const& eventOpInfo;
    mace::string const& dest_contextName;
    mace::string const& src_contextName;
  };

  class EventExecutionControl_Message: public InternalMessageHelper, virtual public PrintPrintable {
  public:
    static const uint8_t COMMIT_CONTEXTS = 3;
    static const uint8_t COMMIT_CONTEXT = 4;
    static const uint8_t TO_LOCK_CONTEXT = 5;
    static const uint8_t NEW_EVENT_OP = 6;
    static const uint8_t EVENT_OP_DONE = 7;
    static const uint8_t CREATE_NEW_CONTEXT = 8;
    static const uint8_t CREATE_NEW_CONTEXT_REPLY = 9;
    static const uint8_t RELEASE_CONTEXTS = 10;
    static const uint8_t LOCK_CONTEXT_PERMISSION = 11;
    static const uint8_t RELEASE_CONTEXT_REPLY = 12;
    static const uint8_t UNLOCK_CONTEXT = 13;
    static const uint8_t CHECK_COMMIT = 14;
    static const uint8_t READY_TO_COMMIT = 15;
    static const uint8_t RELEASE_LOCK_ON_CONTEXT = 16;
    static const uint8_t ENQUEUE_LOCK_CONTEXTS = 17;
    static const uint8_t ENQUEUE_OWNERSHIP_OPS = 18;
    static const uint8_t ENQUEUE_OWNERSHIP_OPS_REPLY = 19;

    static const uint8_t PRINT_CTX_INFO = 30;
    
  private:
  struct EventExecutionControl_struct{
    uint8_t control_type;

    mace::string src_contextName;
    mace::string dest_contextName;
    mace::OrderID eventId;
    mace::vector<mace::string> contextNames;
    mace::vector<EventOperationInfo> opInfos;
    uint32_t newContextId;
  };

  EventExecutionControl_struct* _data_store_;
  mutable size_t serializedByteSize;
  mutable std::string serializedCache;
  public:
    EventExecutionControl_Message( ): _data_store_( new EventExecutionControl_struct() ), 
      serializedByteSize(0) , 
      control_type( _data_store_->control_type ),
      src_contextName( _data_store_->src_contextName ),
      dest_contextName( _data_store_->dest_contextName ),
      eventId( _data_store_->eventId),
      contextNames( _data_store_->contextNames ),
      opInfos( _data_store_->opInfos ),
      newContextId( _data_store_->newContextId ) { }

    EventExecutionControl_Message( const uint8_t& control_type, mace::string const& src_contextName, mace::string const& dest_contextName, mace::OrderID const& eventId, 
      mace::vector<mace::string> const& contextNames, mace::vector<mace::EventOperationInfo> const& opInfos, uint32_t const& newContextId ): 
      _data_store_( NULL ), serializedByteSize(0) , control_type( control_type ), src_contextName(src_contextName), dest_contextName(dest_contextName), eventId( eventId ), contextNames( contextNames ), 
      opInfos( opInfos ), newContextId(newContextId) { }
   
    // WC: C++03 forbids calling a constructor from another constructor.
    EventExecutionControl_Message( InternalMessageHelper const* o): _data_store_( new EventExecutionControl_struct() ), 
        serializedByteSize(0) , 
        control_type( _data_store_->control_type ), src_contextName( _data_store_->src_contextName ), 
        dest_contextName( _data_store_->dest_contextName ), eventId( _data_store_->eventId), 
        contextNames( _data_store_->contextNames ),
        opInfos( _data_store_->opInfos ), newContextId(_data_store_->newContextId) {

      EventExecutionControl_Message const* orig_helper = static_cast< EventExecutionControl_Message const* >( o );
      _data_store_->control_type = orig_helper->control_type;
      _data_store_->dest_contextName = orig_helper->dest_contextName;
      _data_store_->src_contextName = orig_helper->src_contextName;
      _data_store_->eventId = orig_helper->eventId;
      _data_store_->contextNames = orig_helper->contextNames;
      _data_store_->opInfos = orig_helper->opInfos;
      _data_store_->newContextId = orig_helper->newContextId;
    }

    ~EventExecutionControl_Message(){ 
      ADD_SELECTORS("EventExecutionControl_Message::destory");
      // macedbg(1) << "Destory event control message: " << *this << Log::endl;
      delete _data_store_; 
      _data_store_ = NULL; 
    }

    void print(std::ostream& __out) const {
      __out << "EventExecutionControl(";
      __out << "control_type=";  mace::printItem(__out, &(control_type));
      __out << ", ";
      __out << "src_contextName="; mace::printItem(__out, &(src_contextName));
      __out << ", ";
      __out << "dest_contextName="; mace::printItem(__out, &(dest_contextName));
      __out << ", ";
      __out << "eventId="; mace::printItem(__out, &(eventId));
      __out << ", ";
      __out << "contextNames="; mace::printItem(__out, &(contextNames));
      __out << ", ";
      __out << "opInfos="; mace::printItem(__out, &(opInfos));
      __out << ")";
    }

    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      mace::serialize(str, &control_type);
      mace::serialize(str, &src_contextName);
      mace::serialize(str, &dest_contextName);
      mace::serialize(str, &eventId);
      mace::serialize(str, &contextNames);
      mace::serialize(str, &opInfos);
      mace::serialize(str, &newContextId);
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->control_type);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->src_contextName);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->dest_contextName);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextNames);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->opInfos);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->newContextId);
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    uint8_t const& control_type;
    mace::string const& src_contextName;
    mace::string const& dest_contextName;
    mace::OrderID const& eventId;
    mace::vector<mace::string> const& contextNames;
    mace::vector<EventOperationInfo> const& opInfos;
    uint32_t const& newContextId;
  };


  class ElasticityControl_Message: public InternalMessageHelper, virtual public PrintPrintable {
  public:
    static const uint8_t CONTEXT_NODE_STRENGTH_REQ = 0;
    static const uint8_t CONTEXT_NODE_STRENGTH_REP = 1;
    static const uint8_t CONTEXTS_COLOCATE_REQ = 2;
    static const uint8_t SERVER_INFO_REPORT = 3;
    static const uint8_t SERVER_INFO_REQ = 4;
    static const uint8_t SERVER_INFO_REPLY = 5;
    static const uint8_t CONTEXTS_MIGRATION_QUERY = 6;
    static const uint8_t CONTEXTS_MIGRATION_QUERY_REPLY = 7;
    static const uint8_t INACTIVATE_CONTEXT = 8;
    static const uint8_t ACTIVATE_CONTEXT = 9;

  private:
  struct ElasticityControl_struct{
    uint8_t controlType;

    uint32_t handlerThreadId;
    mace::string contextName;
    mace::MaceAddr addr;
    uint64_t value;
    mace::vector< mace::string > contextNames;
    mace::map< mace::MaceAddr, mace::ServerRuntimeInfo > serversInfo;
    mace::vector< mace::MigrationContextInfo> migrationContextsInfo;
  };

  ElasticityControl_struct* _data_store_;
  mutable size_t serializedByteSize;
  mutable std::string serializedCache;
  public:
    ElasticityControl_Message( ): _data_store_( new ElasticityControl_struct() ), 
      serializedByteSize(0) , 
      controlType( _data_store_->controlType ),
      handlerThreadId( _data_store_->handlerThreadId ),
      contextName( _data_store_->contextName ),
      addr( _data_store_->addr ),
      value( _data_store_->value ),
      contextNames( _data_store_->contextNames ),
      serversInfo( _data_store_->serversInfo ),
      migrationContextsInfo( _data_store_->migrationContextsInfo ) { }

    ElasticityControl_Message( const uint8_t& controlType, const uint32_t& handlerThreadId, mace::string const& contextName, 
      mace::MaceAddr const& addr, const uint64_t& value, const mace::vector<mace::string>& contextNames, 
      const mace::map< mace::MaceAddr, mace::ServerRuntimeInfo>& serversInfo, 
      const mace::vector<mace::MigrationContextInfo>& migrationContextsInfo ): _data_store_( NULL ), 
      serializedByteSize(0) , controlType( controlType ), handlerThreadId(handlerThreadId), contextName(contextName), addr(addr), 
      value( value ), contextNames( contextNames ), serversInfo( serversInfo ), migrationContextsInfo( migrationContextsInfo) { }
   
    // WC: C++03 forbids calling a constructor from another constructor.
    ElasticityControl_Message( InternalMessageHelper const* o): _data_store_( new ElasticityControl_struct() ), 
        serializedByteSize(0) , 
        controlType( _data_store_->controlType ), handlerThreadId( _data_store_->handlerThreadId ), contextName( _data_store_->contextName ),
        addr( _data_store_->addr ), value( _data_store_->value ), contextNames( _data_store_->contextNames ),
        serversInfo( _data_store_->serversInfo ), migrationContextsInfo( _data_store_->migrationContextsInfo) {

      ElasticityControl_Message const* orig_helper = static_cast< ElasticityControl_Message const* >( o );
      _data_store_->controlType = orig_helper->controlType;
      _data_store_->handlerThreadId = orig_helper->handlerThreadId;
      _data_store_->contextName = orig_helper->contextName;
      _data_store_->addr = orig_helper->addr;
      _data_store_->value = orig_helper->value;
      _data_store_->contextNames = orig_helper->contextNames;
      _data_store_->serversInfo = orig_helper->serversInfo;
      _data_store_->migrationContextsInfo = orig_helper->migrationContextsInfo;
    }
    ~ElasticityControl_Message(){ delete _data_store_; _data_store_ = NULL; }

    void print(std::ostream& __out) const {
      __out << "ElasticityControl(";
      __out << "controlType=";  mace::printItem(__out, &(controlType));
      __out << ", ";
      __out << "contextName="; mace::printItem(__out, &(contextName));
      __out << ")";
    }

    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      mace::serialize(str, &controlType);
      mace::serialize(str, &handlerThreadId);
      mace::serialize(str, &contextName);
      mace::serialize(str, &addr);
      mace::serialize(str, &value);
      mace::serialize(str, &contextNames);
      mace::serialize(str, &serversInfo );
      mace::serialize(str, &migrationContextsInfo);
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->controlType);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->handlerThreadId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextName);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->addr);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->value);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextNames);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->serversInfo );
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->migrationContextsInfo);
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    uint8_t const& controlType;
    uint32_t const& handlerThreadId;
    mace::string const& contextName;
    mace::MaceAddr const& addr;
    uint64_t const& value;
    mace::vector<mace::string> const& contextNames;
    mace::map< mace::MaceAddr, mace::ServerRuntimeInfo> const& serversInfo;
    mace::vector< mace::MigrationContextInfo> const& migrationContextsInfo;
  };

  class ContextBaseClassParams;

  class TransferContext_Message: public InternalMessageHelper, virtual public PrintPrintable{
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    TransferContext_Message() : serializedByteSize(0) , ctxParams(NULL), checkpoint(""), eventId( ) {}
    TransferContext_Message( const ContextBaseClassParams* my_ctxParams, mace::string const & my_checkpoint, mace::OrderID const & my_eventId) : serializedByteSize(0) {
      ctxParams = const_cast<ContextBaseClassParams*>(my_ctxParams);
      checkpoint = my_checkpoint;
      eventId = my_eventId;
    }
    TransferContext_Message(InternalMessageHelper const* o) : serializedByteSize(0) {
     TransferContext_Message const* orig_helper = static_cast< TransferContext_Message const* >( o );
      ctxParams = const_cast<ContextBaseClassParams*>(orig_helper->ctxParams);
      checkpoint = orig_helper->checkpoint;
      eventId = orig_helper->eventId;
    }
    virtual ~TransferContext_Message();

    void print(std::ostream& __out) const {
      const ContextBaseClassParams* constCtxParams = ctxParams;
      __out << "TransferContext(";
      __out << "ctxParams=";  mace::printItem(__out, constCtxParams);
      __out << ", ";
      __out << "checkpoint=";  mace::printItem(__out, &(checkpoint));
      __out << ", ";
      __out << "eventId=";  mace::printItem(__out, &(eventId));
      __out << ")";
    }
    void serialize(std::string& str) const;

    int deserialize(std::istream& __mace_in) throw (mace::SerializationException);

    mace::ContextBaseClassParams* ctxParams ;
    mace::string checkpoint ;
    mace::OrderID eventId ;
    
  };

  class create_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct create_struct {
      __asyncExtraField extra ;
      uint64_t counter;
			//bsang: events should include created context id 
			uint32_t create_ctxId;
		};
    create_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    create_Message() : _data_store_(new create_struct()), serializedByteSize(0) , extra(_data_store_->extra), counter(_data_store_->counter), create_ctxId(_data_store_->create_ctxId) {}
    create_Message(__asyncExtraField const & my_extra, uint64_t const & my_counter, uint32_t const & my_create_ctxId) : _data_store_(NULL), serializedByteSize(0), extra(my_extra), counter(my_counter), create_ctxId(my_create_ctxId) {}
    create_Message(InternalMessageHelper const* o) : _data_store_(new create_struct()), serializedByteSize(0) , extra(_data_store_->extra), counter(_data_store_->counter), create_ctxId(_data_store_->create_ctxId) {
     create_Message const* orig_helper = static_cast< create_Message const* >( o );
      _data_store_->extra = orig_helper->extra;
      _data_store_->counter = orig_helper->counter;
			_data_store_->create_ctxId = orig_helper->create_ctxId;
		}
    virtual ~create_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "create(";
          __out << "extra=";  mace::printItem(__out, &(extra));
          __out << ", ";
          __out << "counter=";  mace::printItem(__out, &(counter));
					__out << ", ";
					__out << "create context ID="; mace::printItem(__out, &(create_ctxId));
          __out << ")";
    }
    void serialize(std::string& str) const { 
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      mace::serialize(str, &extra);
      mace::serialize(str, &counter);
			mace::serialize(str, &create_ctxId);
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
    
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->extra);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->counter);
			serializedByteSize +=	 mace::deserialize(__mace_in, &_data_store_->create_ctxId);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    
    __asyncExtraField const & extra;
    uint64_t const & counter;
		uint64_t const & create_ctxId;
	};
  class create_head_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct create_head_struct {
      __asyncExtraField extra ;
      uint64_t counter ;
			uint32_t ctxId;
      MaceAddr src ;
    };
    create_head_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    create_head_Message() : _data_store_(new create_head_struct()), serializedByteSize(0) , extra(_data_store_->extra), 
      counter(_data_store_->counter), src(_data_store_->src), ctxId(_data_store_->ctxId) {}
    create_head_Message(__asyncExtraField const & my_extra, uint64_t const & my_counter, uint32_t const & my_ctxID, 
      MaceAddr const & my_src) : _data_store_(NULL), serializedByteSize(0), extra(my_extra), counter(my_counter), 
      src(my_src), ctxId(my_ctxID) {}
    create_head_Message(InternalMessageHelper const* o) : _data_store_(new create_head_struct()), serializedByteSize(0) , 
      extra(_data_store_->extra), counter(_data_store_->counter), src(_data_store_->src), ctxId(_data_store_->ctxId) {
    
     create_head_Message const* orig_helper = static_cast< create_head_Message const* >( o );
      _data_store_->extra = orig_helper->extra;
      _data_store_->counter = orig_helper->counter;
      _data_store_->src = orig_helper->src;
			_data_store_->ctxId = orig_helper->ctxId;
    }


    virtual ~create_head_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "create_head(";
          __out << "extra=";  mace::printItem(__out, &(extra));
          __out << ", ";
          __out << "counter=";  mace::printItem(__out, &(counter));
          __out << ", ";
          __out << "src=";  mace::printItem(__out, &(src));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      mace::serialize(str, &extra);
      mace::serialize(str, &counter);
      mace::serialize(str, &src);
			mace::serialize(str, &ctxId);
      
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->extra);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->counter);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->src);
			serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->ctxId);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    __asyncExtraField const& extra;
    uint64_t const& counter;
    MaceAddr const& src;
		uint32_t const& ctxId;
  };
  class create_response_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct create_response_struct {
      mace::Event event ;
      uint32_t counter ;
      MaceAddr targetAddress ;
    };
    create_response_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    create_response_Message() : _data_store_(new create_response_struct()), serializedByteSize(0) , event(_data_store_->event), counter(_data_store_->counter), targetAddress(_data_store_->targetAddress) {}
    create_response_Message(mace::Event const & my_event, uint32_t const & my_counter, MaceAddr const & my_targetAddress) : _data_store_(NULL), serializedByteSize(0), event(my_event), counter(my_counter), targetAddress(my_targetAddress) {}
    create_response_Message(InternalMessageHelper const* o) : _data_store_(new create_response_struct()), serializedByteSize(0) , event(_data_store_->event), counter(_data_store_->counter), targetAddress(_data_store_->targetAddress) {
     create_response_Message const* orig_helper = static_cast< create_response_Message const* >( o );
      _data_store_->event = orig_helper->event;
      _data_store_->counter = orig_helper->counter;
      _data_store_->targetAddress = orig_helper->targetAddress;
    
    
    }
    virtual ~create_response_Message() { delete _data_store_; _data_store_ = NULL; }
    
    void print(std::ostream& __out) const {
      __out << "create_response(";
          __out << "event=";  mace::printItem(__out, &(event));
          __out << ", ";
          __out << "counter=";  mace::printItem(__out, &(counter));
          __out << ", ";
          __out << "targetAddress=";  mace::printItem(__out, &(targetAddress));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      mace::serialize(str, &event);
      mace::serialize(str, &counter);
      mace::serialize(str, &targetAddress);
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->event);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->counter);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->targetAddress);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }

    mace::Event const & event;
    uint32_t const & counter;
    MaceAddr const & targetAddress;
  };
  class exit_committed_Message: public InternalMessageHelper, virtual public PrintPrintable{
  struct exit_committed_struct {  };
    exit_committed_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    exit_committed_Message() : _data_store_(new exit_committed_struct()), serializedByteSize(0)  {}
    exit_committed_Message(InternalMessageHelper const* o) : _data_store_(new exit_committed_struct()), serializedByteSize(0)  {
    }
    virtual ~exit_committed_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "exit_committed()";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
  };
  class enter_context_Message: public InternalMessageHelper, virtual public PrintPrintable{
  struct enter_context_struct {
    mace::Event event ;
    mace::vector< uint32_t > contextIDs ;
  };
    enter_context_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    enter_context_Message() : _data_store_(new enter_context_struct()), serializedByteSize(0) , event(_data_store_->event), contextIDs(_data_store_->contextIDs) {}
    enter_context_Message(mace::Event const & my_event, mace::vector< uint32_t > const & my_contextIDs) : _data_store_(NULL), serializedByteSize(0), event(my_event), contextIDs(my_contextIDs) {}
    enter_context_Message(InternalMessageHelper const* o ) : _data_store_(new enter_context_struct()), serializedByteSize(0) , event(_data_store_->event), contextIDs(_data_store_->contextIDs) {
     enter_context_Message const* orig_helper = static_cast< enter_context_Message const* >( o );
      _data_store_->event = orig_helper->event;
      _data_store_->contextIDs = orig_helper->contextIDs;
    
    }
    virtual ~enter_context_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "enter_context(";
          __out << "event=";  mace::printItem(__out, &(event));
          __out << ", ";
          __out << "contextIDs=";  mace::printItem(__out, &(contextIDs));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      mace::serialize(str, &event);
      mace::serialize(str, &contextIDs);
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->event);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextIDs);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    
    mace::Event const & event;
    mace::vector< uint32_t > const & contextIDs;
  };

  class commit_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct commit_struct {
      mace::Event event;
    };
    commit_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    commit_Message() : _data_store_(new commit_struct()), serializedByteSize(0) , event(_data_store_->event) {}
    commit_Message(mace::Event const & my_event) : _data_store_(NULL), serializedByteSize(0), event(my_event) {
      
    }
    commit_Message(InternalMessageHelper const* o) : _data_store_(new commit_struct()), serializedByteSize(0) , event(_data_store_->event) {
      commit_Message const* orig_helper = static_cast< commit_Message const* >( o );
      _data_store_->event = orig_helper->event;
    }
    virtual ~commit_Message() {
      delete _data_store_; 
      _data_store_ = NULL; 
    }
    
    void print(std::ostream& __out) const {
      __out << "commit(";
          __out << "event=";  mace::printItem(__out, &(event));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      mace::serialize(str, &event);
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->event);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    
    mace::Event const & event;
  };
  class commit_single_context_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct commit_single_context_struct {
      uint32_t ctxId ;
      mace::OrderID eventId ;
      int8_t eventType ;
      uint64_t eventContextMappingVersion ;
      // mace::Event::SkipRecordType eventOrderRecords ;
      bool isresponse ;
      bool hasException ;
      uint32_t exceptionContextId ;
      mace::set< mace::string > accessCtxs;
      mace::OrderID bEventId;
    };
    commit_single_context_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    commit_single_context_Message() : _data_store_(new commit_single_context_struct()), serializedByteSize(0) , 
      ctxId(_data_store_->ctxId), eventId(_data_store_->eventId), eventType(_data_store_->eventType), 
      eventContextMappingVersion(_data_store_->eventContextMappingVersion), 
      isresponse(_data_store_->isresponse), hasException(_data_store_->hasException), exceptionContextId(_data_store_->exceptionContextId), accessCtxs(_data_store_->accessCtxs),
      bEventId(_data_store_->bEventId) {}

    commit_single_context_Message(uint32_t const & my_ctxId, mace::OrderID const & my_eventId, 
      int8_t const & my_eventType, uint64_t const & my_eventContextMappingVersion, 
      bool const & my_isresponse, bool const & my_hasException, 
      uint32_t const & my_exceptionContextId, mace::set<mace::string> const& my_accessCtxs, mace::OrderID const& my_bEventId) : _data_store_(new commit_single_context_struct()), 
        serializedByteSize(0) , ctxId(_data_store_->ctxId), eventId(_data_store_->eventId), eventType(_data_store_->eventType), 
        eventContextMappingVersion(_data_store_->eventContextMappingVersion), 
        isresponse(_data_store_->isresponse), hasException(_data_store_->hasException), exceptionContextId(_data_store_->exceptionContextId), accessCtxs(_data_store_->accessCtxs),
        bEventId(_data_store_->bEventId) {

      _data_store_->ctxId = my_ctxId;
      _data_store_->eventId = my_eventId;
      _data_store_->eventType = my_eventType;
      _data_store_->eventContextMappingVersion = my_eventContextMappingVersion;
      _data_store_->isresponse = my_isresponse;
      _data_store_->hasException = my_hasException;
      _data_store_->exceptionContextId = my_exceptionContextId;
      _data_store_->accessCtxs = my_accessCtxs;
      _data_store_->bEventId = my_bEventId;
    }

    commit_single_context_Message(InternalMessageHelper const* o) : _data_store_(new commit_single_context_struct()), 
        serializedByteSize(0) , ctxId(_data_store_->ctxId), eventId(_data_store_->eventId), eventType(_data_store_->eventType), 
        eventContextMappingVersion(_data_store_->eventContextMappingVersion),  
        isresponse(_data_store_->isresponse), hasException(_data_store_->hasException), exceptionContextId(_data_store_->exceptionContextId), accessCtxs(_data_store_->accessCtxs),
        bEventId(_data_store_->bEventId) {
    
     commit_single_context_Message const* orig_helper = static_cast< commit_single_context_Message const* >( o );
      _data_store_->ctxId = orig_helper->ctxId;
      _data_store_->eventId = orig_helper->eventId;
      _data_store_->eventType = orig_helper->eventType;
      _data_store_->eventContextMappingVersion = orig_helper->eventContextMappingVersion;
      _data_store_->isresponse = orig_helper->isresponse;
      _data_store_->hasException = orig_helper->hasException;
      _data_store_->exceptionContextId = orig_helper->exceptionContextId;
      _data_store_->accessCtxs = orig_helper->accessCtxs;
      _data_store_->bEventId = orig_helper->bEventId;
    }
    virtual ~commit_single_context_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "commit_context(";
          __out << "contextId=";  mace::printItem(__out, &(ctxId));
          __out << ", ";
          __out << "eventID=";  mace::printItem(__out, &(eventId));
          __out << ", ";
          __out << "eventType=";  mace::printItem(__out, &(eventType));
          __out << ", ";
          __out << "eventContextMappingVersion=";  mace::printItem(__out, &(eventContextMappingVersion));
          __out << ", ";
          __out << "isresponse=";  mace::printItem(__out, &(isresponse));
          __out << ", ";
          __out << "hasException=";  mace::printItem(__out, &(hasException));
          __out << ", ";
          __out << "exceptionContextId=";  mace::printItem(__out, &(exceptionContextId));
          __out << ", ";
          __out << "access_contexts="; mace::printItem(__out, &(accessCtxs));
          __out << ", ";
          __out << "bEventId="; mace::printItem(__out, &(bEventId));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      mace::serialize(str, &ctxId);
      mace::serialize(str, &eventId);
      mace::serialize(str, &eventType);
      mace::serialize(str, &eventContextMappingVersion);
      mace::serialize(str, &isresponse);
      mace::serialize(str, &hasException);
      mace::serialize(str, &exceptionContextId);
      mace::serialize(str, &accessCtxs);
      mace::serialize(str, &bEventId);
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->ctxId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventType);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventContextMappingVersion);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->isresponse);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->hasException);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->exceptionContextId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->accessCtxs);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->bEventId);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    
    uint32_t const & ctxId;
    mace::OrderID const & eventId;
    int8_t const & eventType;
    uint64_t const & eventContextMappingVersion;
    bool const & isresponse;
    bool const & hasException;
    uint32_t const & exceptionContextId;
    mace::set<mace::string> const & accessCtxs;
    mace::OrderID const & bEventId;
  };

  class commit_context_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct commit_context_struct {
      mace::vector< uint32_t > ctxIds ;
      mace::OrderID eventId ;
      int8_t eventType ;
      uint64_t eventContextMappingVersion ;
      //mace::map< uint8_t, mace::Event::EventSkipRecordType > eventSkipID ;
      //mace::Event::SkipRecordType eventOrderRecords ;
      bool isresponse ;
      bool hasException ;
      uint32_t exceptionContextId ;
      mace::set<mace::string> accessCtxs;
      mace::OrderID bEventId;
    };
    commit_context_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    commit_context_Message() : _data_store_(new commit_context_struct()), serializedByteSize(0) , 
      ctxIds(_data_store_->ctxIds), eventId(_data_store_->eventId), eventType(_data_store_->eventType), 
      eventContextMappingVersion(_data_store_->eventContextMappingVersion), 
      isresponse(_data_store_->isresponse), hasException(_data_store_->hasException), exceptionContextId(_data_store_->exceptionContextId), accessCtxs(_data_store_->accessCtxs),
      bEventId(_data_store_->bEventId) {}

    commit_context_Message(mace::vector< uint32_t > const & my_ctxIds, mace::OrderID const & my_eventId, 
      int8_t const & my_eventType, uint64_t const & my_eventContextMappingVersion, 
      bool const & my_isresponse, bool const & my_hasException, 
      uint32_t const & my_exceptionContextId, mace::set<mace::string> const& my_accessCtxs, mace::OrderID const& my_bEventId) : 
      _data_store_(NULL), serializedByteSize(0), ctxIds(my_ctxIds), eventId(my_eventId), eventType(my_eventType), eventContextMappingVersion(my_eventContextMappingVersion), 
      isresponse(my_isresponse), hasException(my_hasException), 
      exceptionContextId(my_exceptionContextId), accessCtxs(my_accessCtxs), bEventId(my_bEventId) {}



    commit_context_Message(InternalMessageHelper const* o) : _data_store_(new commit_context_struct()), 
        serializedByteSize(0) , ctxIds(_data_store_->ctxIds), eventId(_data_store_->eventId), eventType(_data_store_->eventType), 
        eventContextMappingVersion(_data_store_->eventContextMappingVersion),  
        isresponse(_data_store_->isresponse), hasException(_data_store_->hasException), exceptionContextId(_data_store_->exceptionContextId), accessCtxs(_data_store_->accessCtxs),
        bEventId(_data_store_->bEventId) {
    
     commit_context_Message const* orig_helper = static_cast< commit_context_Message const* >( o );
      _data_store_->ctxIds = orig_helper->ctxIds;
      _data_store_->eventId = orig_helper->eventId;
      _data_store_->eventType = orig_helper->eventType;
      _data_store_->eventContextMappingVersion = orig_helper->eventContextMappingVersion;
      _data_store_->isresponse = orig_helper->isresponse;
      _data_store_->hasException = orig_helper->hasException;
      _data_store_->exceptionContextId = orig_helper->exceptionContextId;
      _data_store_->accessCtxs = orig_helper->accessCtxs;
      _data_store_->bEventId = orig_helper->bEventId;
      
    }
    virtual ~commit_context_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "commit_context(";
          __out << "contextIds=";  mace::printItem(__out, &(ctxIds));
          __out << ", ";
          __out << "eventID=";  mace::printItem(__out, &(eventId));
          __out << ", ";
          __out << "eventType=";  mace::printItem(__out, &(eventType));
          __out << ", ";
          __out << "eventContextMappingVersion=";  mace::printItem(__out, &(eventContextMappingVersion));
          __out << ", ";
          __out << "isresponse=";  mace::printItem(__out, &(isresponse));
          __out << ", ";
          __out << "hasException=";  mace::printItem(__out, &(hasException));
          __out << ", ";
          __out << "exceptionContextId=";  mace::printItem(__out, &(exceptionContextId));
          __out << ", ";
          __out << "access_contexts="; mace::printItem(__out, &(accessCtxs));
          __out << ", ";
          __out << "bEventId="; mace::printItem(__out, &(bEventId));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      mace::serialize(str, &ctxIds);
      mace::serialize(str, &eventId);
      mace::serialize(str, &eventType);
      mace::serialize(str, &eventContextMappingVersion);
      mace::serialize(str, &isresponse);
      mace::serialize(str, &hasException);
      mace::serialize(str, &exceptionContextId);
      mace::serialize(str, &accessCtxs);
      mace::serialize(str, &bEventId);
            
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->ctxIds);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventType);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventContextMappingVersion);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->isresponse);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->hasException);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->exceptionContextId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->accessCtxs);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->bEventId);
            
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    
    mace::vector< uint32_t > const & ctxIds;
    mace::OrderID const & eventId;
    int8_t const & eventType;
    uint64_t const & eventContextMappingVersion;
    bool const & isresponse;
    bool const & hasException;
    uint32_t const & exceptionContextId;
    mace::set< mace::string > const & accessCtxs;
    mace::OrderID const & bEventId;
  };

  class commit_context_migration_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct commit_context_migration_struct {
      mace::OrderID eventId;
      mace::string contextName;
      mace::MaceAddr srcAddr;
      mace::MaceAddr destAddr;
    };

    commit_context_migration_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    commit_context_migration_Message() : _data_store_(new commit_context_migration_struct()), serializedByteSize(0) , 
      eventId(_data_store_->eventId), contextName( _data_store_->contextName ), srcAddr( _data_store_->srcAddr ), destAddr( _data_store_->destAddr ) {}
    commit_context_migration_Message(mace::OrderID const& eventId, mace::string const& contextName, mace::MaceAddr const& srcAddr, 
      mace::MaceAddr const& destAddr ) : _data_store_(NULL), serializedByteSize(0), eventId( eventId ), contextName( contextName ), 
      srcAddr( srcAddr ), destAddr( destAddr ) { }
    commit_context_migration_Message(InternalMessageHelper const* o) : _data_store_(new commit_context_migration_struct()), serializedByteSize(0), 
      eventId(_data_store_->eventId), contextName( _data_store_->contextName ), srcAddr( _data_store_->srcAddr ), destAddr( _data_store_->destAddr ) {
      commit_context_migration_Message const* orig_helper = static_cast< commit_context_migration_Message const* >( o );
      _data_store_->eventId = orig_helper->eventId;
      _data_store_->contextName = orig_helper->contextName;
      _data_store_->srcAddr = orig_helper->srcAddr;
      _data_store_->destAddr = orig_helper->destAddr;
    }
    virtual ~commit_context_migration_Message() {
      delete _data_store_; 
      _data_store_ = NULL; 
    }
    
    void print(std::ostream& __out) const {
      __out << "commit_context_migration(";
      __out << "eventId=";  mace::printItem(__out, &(eventId));
      __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      mace::serialize(str, &eventId);
      mace::serialize(str, &contextName);
      mace::serialize(str, &srcAddr);
      mace::serialize(str, &destAddr);

      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventId);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextName);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->srcAddr);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->destAddr);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    
    mace::OrderID const& eventId;
    mace::string const& contextName;
    mace::MaceAddr const& srcAddr;
    mace::MaceAddr const& destAddr;
  };




  class snapshot_Message: public InternalMessageHelper, virtual public PrintPrintable{
  struct snapshot_struct {
    mace::Event event ;
    mace::string ctxID ;
    mace::string snapshotContextID ;
    mace::string snapshot ;
  };
    snapshot_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    snapshot_Message() : _data_store_(new snapshot_struct()), serializedByteSize(0) , event(_data_store_->event), ctxID(_data_store_->ctxID), snapshotContextID(_data_store_->snapshotContextID), snapshot(_data_store_->snapshot) {}
    snapshot_Message(mace::Event const & my_event, mace::string const & my_ctxID, mace::string const & my_snapshotContextID, mace::string const & my_snapshot) : _data_store_(NULL), serializedByteSize(0), event(my_event), ctxID(my_ctxID), snapshotContextID(my_snapshotContextID), snapshot(my_snapshot) {}
    snapshot_Message(InternalMessageHelper const* o) : _data_store_(new snapshot_struct()), serializedByteSize(0) , event(_data_store_->event), ctxID(_data_store_->ctxID), snapshotContextID(_data_store_->snapshotContextID), snapshot(_data_store_->snapshot) {
    
     snapshot_Message const* orig_helper = static_cast< snapshot_Message const* >( o );
      _data_store_->event = orig_helper->event;
      _data_store_->ctxID = orig_helper->ctxID;
      _data_store_->snapshotContextID = orig_helper->snapshotContextID;
      _data_store_->snapshot = orig_helper->snapshot;
    
    }
    virtual ~snapshot_Message() { delete _data_store_; _data_store_ = NULL; }
    
    void print(std::ostream& __out) const {
      __out << "snapshot(";
          __out << "event=";  mace::printItem(__out, &(event));
          __out << ", ";
          __out << "ctxID=";  mace::printItem(__out, &(ctxID));
          __out << ", ";
          __out << "snapshotContextID=";  mace::printItem(__out, &(snapshotContextID));
          __out << ", ";
          __out << "snapshot=";  mace::printItem(__out, &(snapshot));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      mace::serialize(str, &event);
      mace::serialize(str, &ctxID);
      mace::serialize(str, &snapshotContextID);
      mace::serialize(str, &snapshot);
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->event);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->ctxID);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->snapshotContextID);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->snapshot);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    
    mace::Event const & event;
    mace::string const & ctxID;
    mace::string const & snapshotContextID;
    mace::string const & snapshot;
  };
  class downgrade_context_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct downgrade_context_struct {
      uint32_t contextID ;
      mace::OrderID eventID ;
      bool isresponse ;
    };
    downgrade_context_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    downgrade_context_Message() : _data_store_(new downgrade_context_struct()), serializedByteSize(0) , contextID(_data_store_->contextID), eventID(_data_store_->eventID), isresponse(_data_store_->isresponse) {}
    downgrade_context_Message(uint32_t const & my_contextID, mace::OrderID const & my_eventID, bool const & my_isresponse) : _data_store_(NULL), serializedByteSize(0), contextID(my_contextID), eventID(my_eventID), isresponse(my_isresponse) {}
    downgrade_context_Message(InternalMessageHelper const* o) : _data_store_(new downgrade_context_struct()), serializedByteSize(0) , contextID(_data_store_->contextID), eventID(_data_store_->eventID), isresponse(_data_store_->isresponse) {
    
     downgrade_context_Message const* orig_helper = static_cast< downgrade_context_Message const* >( o );
      _data_store_->contextID = orig_helper->contextID;
      _data_store_->eventID = orig_helper->eventID;
      _data_store_->isresponse = orig_helper->isresponse;
    
    }
    virtual ~downgrade_context_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "downgrade_context(";
          __out << "contextID=";  mace::printItem(__out, &(contextID));
          __out << ", ";
          __out << "eventID=";  mace::printItem(__out, &(eventID));
          __out << ", ";
          __out << "isresponse=";  mace::printItem(__out, &(isresponse));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      mace::serialize(str, &contextID);
      mace::serialize(str, &eventID);
      mace::serialize(str, &isresponse);
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextID);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventID);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->isresponse);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    
    uint32_t const & contextID;
    mace::OrderID const & eventID;
    bool const & isresponse;
  };
  class evict_Message: public InternalMessageHelper, virtual public PrintPrintable{
  struct evict_struct {  };
    evict_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    evict_Message() : _data_store_(new evict_struct()), serializedByteSize(0)  {}
    
    evict_Message(const evict_Message& _orig) : _data_store_(new evict_struct()), serializedByteSize(0)  {
      
    }
    evict_Message(InternalMessageHelper const* o) : _data_store_(new evict_struct()), serializedByteSize(0)  {
    }
    virtual ~evict_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "evict()";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
  };
  class migrate_context_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct migrate_context_struct {
      mace::MaceAddr newNode ;
      mace::string contextName ;
      uint64_t delay ;
    };
    migrate_context_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
    
  public:
    migrate_context_Message() : _data_store_(new migrate_context_struct()), serializedByteSize(0) , newNode(_data_store_->newNode), contextName(_data_store_->contextName), delay(_data_store_->delay) {}
    migrate_context_Message(mace::MaceAddr const & my_newNode, mace::string const & my_contextName, uint64_t const & my_delay) : _data_store_(NULL), serializedByteSize(0), newNode(my_newNode), contextName(my_contextName), delay(my_delay) {}
    migrate_context_Message(InternalMessageHelper const* o) : _data_store_(new migrate_context_struct()), serializedByteSize(0) , newNode(_data_store_->newNode), contextName(_data_store_->contextName), delay(_data_store_->delay) {
     migrate_context_Message const* orig_helper = static_cast< migrate_context_Message const* >( o );
      _data_store_->newNode = orig_helper->newNode;
      _data_store_->contextName = orig_helper->contextName;
      _data_store_->delay = orig_helper->delay;
    }
    virtual ~migrate_context_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "migrate_context(";
          __out << "newNode=";  mace::printItem(__out, &(newNode));
          __out << ", ";
          __out << "contextName=";  mace::printItem(__out, &(contextName));
          __out << ", ";
          __out << "delay=";  mace::printItem(__out, &(delay));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      mace::serialize(str, &newNode);
      mace::serialize(str, &contextName);
      mace::serialize(str, &delay);
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->newNode);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextName);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->delay);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    mace::MaceAddr const & newNode;
    mace::string const & contextName;
    uint64_t const & delay;
  };
  class migrate_param_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct migrate_param_struct {
      mace::string paramid;
    };
    migrate_param_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    migrate_param_Message() : _data_store_(new migrate_param_struct()), serializedByteSize(0) , paramid(_data_store_->paramid) {}
    migrate_param_Message(mace::string const & my_paramid) : _data_store_(NULL), serializedByteSize(0), paramid(my_paramid) {}
    migrate_param_Message(InternalMessageHelper const* o ) : _data_store_(new migrate_param_struct()), serializedByteSize(0) , paramid(_data_store_->paramid) {
     migrate_param_Message const* orig_helper = static_cast< migrate_param_Message const* >( o );
      _data_store_->paramid = orig_helper->paramid;
    }
    virtual ~migrate_param_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "migrate_param(";
          __out << "paramid=";  mace::printItem(__out, &(paramid));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      mace::serialize(str, &paramid);
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->paramid);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    
    mace::string const & paramid;

  };
  class RemoveContextObject_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct RemoveContextObject_struct {
      mace::OrderID eventID ;
      mace::ContextMapping ctxmapCopy ;
      MaceAddr dest ;
      uint32_t contextID ;
    };
    RemoveContextObject_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    RemoveContextObject_Message() : _data_store_(new RemoveContextObject_struct()), serializedByteSize(0) , eventID(_data_store_->eventID), ctxmapCopy(_data_store_->ctxmapCopy), dest(_data_store_->dest), contextID(_data_store_->contextID) {}
    RemoveContextObject_Message(mace::OrderID const & my_eventID, mace::ContextMapping const & my_ctxmapCopy, 
      MaceAddr const & my_dest, uint32_t const & my_contextID) : _data_store_(NULL), serializedByteSize(0), 
      eventID(my_eventID), ctxmapCopy(my_ctxmapCopy), dest(my_dest), contextID(my_contextID) {}
    RemoveContextObject_Message(InternalMessageHelper const* o ) : _data_store_(new RemoveContextObject_struct()), serializedByteSize(0) , eventID(_data_store_->eventID), ctxmapCopy(_data_store_->ctxmapCopy), dest(_data_store_->dest), contextID(_data_store_->contextID) {
     RemoveContextObject_Message const* orig_helper = static_cast< RemoveContextObject_Message const* >( o );
      _data_store_->eventID = orig_helper->eventID;
      _data_store_->ctxmapCopy = orig_helper->ctxmapCopy;
      _data_store_->dest = orig_helper->dest;
      _data_store_->contextID = orig_helper->contextID;
    }

    virtual ~RemoveContextObject_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "RemoveContextObject(";
          __out << "eventID=";  mace::printItem(__out, &(eventID));
          __out << ", ";
          __out << "ctxmapCopy=";  mace::printItem(__out, &(ctxmapCopy));
          __out << ", ";
          __out << "dest=";  mace::printItem(__out, &(dest));
          __out << ", ";
          __out << "contextID=";  mace::printItem(__out, &(contextID));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();

      mace::serialize(str, &eventID);
      mace::serialize(str, &ctxmapCopy);
      mace::serialize(str, &dest);
      mace::serialize(str, &contextID);
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->eventID);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->ctxmapCopy);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->dest);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextID);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    
    mace::OrderID const & eventID;
    mace::ContextMapping const & ctxmapCopy;
    MaceAddr const & dest;
    uint32_t const & contextID;
    
  };
  class delete_context_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct delete_context_struct {
      mace::string contextName ;
    };
    delete_context_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    delete_context_Message() : _data_store_(new delete_context_struct()), serializedByteSize(0) , contextName(_data_store_->contextName) {}
    delete_context_Message(mace::string const & my_contextName) : _data_store_(NULL), serializedByteSize(0), contextName(my_contextName) {}
    delete_context_Message(InternalMessageHelper const* o) : _data_store_(new delete_context_struct()), serializedByteSize(0) , contextName(_data_store_->contextName) {
     delete_context_Message const* orig_helper = static_cast< delete_context_Message const* >( o );
      _data_store_->contextName = orig_helper->contextName;
    }
    virtual ~delete_context_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "delete_context(";
          __out << "contextName=";  mace::printItem(__out, &(contextName));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      mace::serialize(str, &contextName);
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->contextName);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
      mace::string const& contextName ;

  };
  class new_head_ready_Message: public InternalMessageHelper, virtual public PrintPrintable{
    struct new_head_ready_struct {  };
    new_head_ready_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    new_head_ready_Message() : _data_store_(new new_head_ready_struct()), serializedByteSize(0)  {}
    
    
    new_head_ready_Message(InternalMessageHelper const* o) : _data_store_(new new_head_ready_struct()), serializedByteSize(0)  {
    }
    virtual ~new_head_ready_Message() { delete _data_store_; _data_store_ = NULL; }
    void print(std::ostream& __out) const {
      __out << "new_head_ready(";
          
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
    
  };
  class routine_return_Message: public InternalMessageHelper, virtual public PrintPrintable{
  struct routine_return_struct {
    mace::string returnValue ;
    mace::Event event ;
  };
    routine_return_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    routine_return_Message() : _data_store_(new routine_return_struct()), serializedByteSize(0) , returnValue(_data_store_->returnValue), event(_data_store_->event) {}
    routine_return_Message(mace::string const & my_returnValue, mace::Event const & my_event) : _data_store_(NULL), serializedByteSize(0), returnValue(my_returnValue), event(my_event) {}
    routine_return_Message(InternalMessageHelper const* o) : _data_store_(new routine_return_struct()), serializedByteSize(0) , returnValue(_data_store_->returnValue), event(_data_store_->event) {
     routine_return_Message const* orig_helper = static_cast< routine_return_Message const* >( o );
      _data_store_->returnValue = orig_helper->returnValue;
      _data_store_->event = orig_helper->event;
    }
    virtual ~routine_return_Message() { delete _data_store_; _data_store_ = NULL; }
    
    mace::string const & returnValue;
    mace::Event const & event;

    void print(std::ostream& __out) const {
      __out << "routine_return(";
          __out << "returnValue=";  mace::printItem(__out, &(returnValue));
          __out << ", ";
          __out << "event=";  mace::printItem(__out, &(event));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      mace::serialize(str, &returnValue);
      mace::serialize(str, &event);
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->returnValue);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->event);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
  };
  class appupcall_return_Message: public InternalMessageHelper, virtual public PrintPrintable{
  struct appupcall_return_struct {
    mace::string returnValue ;
    mace::Event event ;
  };
    appupcall_return_struct* _data_store_;
    mutable size_t serializedByteSize;
    mutable std::string serializedCache;
  public:
    appupcall_return_Message() : _data_store_(new appupcall_return_struct()), serializedByteSize(0) , returnValue(_data_store_->returnValue), event(_data_store_->event) {}
    appupcall_return_Message(mace::string const & my_returnValue, mace::Event const & my_event) : _data_store_(NULL), serializedByteSize(0), returnValue(my_returnValue), event(my_event) {}
    appupcall_return_Message(InternalMessageHelper const* o) : _data_store_(new appupcall_return_struct()), serializedByteSize(0) , returnValue(_data_store_->returnValue), event(_data_store_->event) {
     appupcall_return_Message const* orig_helper = static_cast< appupcall_return_Message const* >( o );
      _data_store_->returnValue = orig_helper->returnValue;
      _data_store_->event = orig_helper->event;
    }
    virtual ~appupcall_return_Message() { delete _data_store_; _data_store_ = NULL; }
    
    mace::string const & returnValue;
    mace::Event const & event;

    void print(std::ostream& __out) const {
      __out << "appupcall_return(";
          __out << "returnValue=";  mace::printItem(__out, &(returnValue));
          __out << ", ";
          __out << "event=";  mace::printItem(__out, &(event));
          __out << ")";
    }
    void serialize(std::string& str) const {
      if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
      }
      size_t initsize = str.size();
      mace::serialize(str, &returnValue);
      mace::serialize(str, &event);
      
      bool __false = false;
      mace::serialize(str, &__false );
      
      if (initsize == 0) {
        serializedCache = str;
      }
      serializedByteSize = str.size() - initsize;
    }
    int deserialize(std::istream& __mace_in) throw (mace::SerializationException) {
      serializedByteSize = 0;
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->returnValue);
      serializedByteSize +=  mace::deserialize(__mace_in, &_data_store_->event);
      
      bool __unused;
      serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
      return serializedByteSize;
    }
  };

  class InternalMessageID : public Serializable, public PrintPrintable {
  public:
    mace::MaceAddr orig_src;
    mace::string targetContextName;
    uint64_t msgTicket;

    InternalMessageID(): orig_src(), targetContextName(""), msgTicket(0) { }
    InternalMessageID(mace::MaceAddr const& src, mace::string const& targetContextName, const uint64_t ticket): orig_src(src), targetContextName(targetContextName), msgTicket(ticket) {

    }

    virtual void serialize(std::string& str) const{
        mace::serialize( str, &orig_src );
        mace::serialize( str, &targetContextName );
        mace::serialize( str, &msgTicket );
    }

    virtual int deserialize(std::istream & is) throw (mace::SerializationException){
        int serializedByteSize = 0;
        serializedByteSize += mace::deserialize( is, &orig_src );
        serializedByteSize += mace::deserialize( is, &targetContextName );
        serializedByteSize += mace::deserialize( is, &msgTicket );
        return serializedByteSize;
    }

    void print(std::ostream& out) const {
      out<< "InternalMessageID(";
      out<< "orig_src = "; mace::printItem(out, &(orig_src)); out<<", ";
      out<< "targetContextName = "; mace::printItem(out, &(targetContextName)); out<<", ";
      out<< "msgTicket = "; mace::printItem(out, &(msgTicket)); 
      out<< ")";
    }

    void printNode(PrintNode& pr, const std::string& name) const {
      mace::PrintNode printer(name, "InternalMessageID" );
  
      mace::printItem( printer, "orig_src", &orig_src );
      mace::printItem( printer, "targetContextName", &targetContextName );
      mace::printItem( printer, "msgTicket", &msgTicket );
      pr.addChild( printer );
    }

    InternalMessageID& operator=(const InternalMessageID & orig){
        ASSERTMSG( this != &orig, "Self assignment is forbidden!" );
        this->orig_src = orig.orig_src;
        this->targetContextName = orig.targetContextName;
        this->msgTicket = orig.msgTicket;
        return *this;
    }
  };

  class InternalMessage : public Message, virtual public PrintPrintable{
  private:
    uint8_t msgType;
    uint8_t sid;
    InternalMessageID msgId;
    mutable InternalMessageHelperPtr helper;
  public:

    static const uint8_t messageType = 255;
    static uint8_t getMsgType() { return messageType; }
    uint8_t getType() const { return InternalMessage::getMsgType(); }

    InternalMessageHelperPtr const& getHelper()const { 
      return helper;
    }
    void unlinkHelper() const{
      //helper = InternalMessageHelperPtr( NULL );
      helper = NULL;
    }
    uint8_t getMessageType() const{ return msgType; }
    const InternalMessageID& getMsgID() const { return msgId; }
    const MaceAddr& getOrigAddr() const { return msgId.orig_src; }
    const mace::string& getTargetContextName() const { return msgId.targetContextName; }


    int deserializeEvent( std::istream& in );
    int deserializeUpcall( std::istream& in );
    int deserializeRoutine( std::istream& in );

    struct AllocateContextObject_type{};
		struct AllocateContextObjReq_type{};
		struct AllocateContextObjectResponse_type{};
    struct ContextMigrationRequest_type{}; // TODO: WC: change to a better name
    struct TransferContext_type{};
    struct create_type{};
    struct create_head_type{};
    struct create_response_type{};
    struct exit_committed_type{};
    struct enter_context_type{};
    struct commit_type{};
    struct commit_context_type{}; // TODO: WC: rename it. it should be renamed to downgrade
    struct commit_single_context_type{};
    struct snapshot_type{};
    struct downgrade_context_type{}; // TODO: may be this is a duplicate of commit_context?
    struct evict_type{};
    struct migrate_context_type{};
    struct migrate_param_type{};
    struct RemoveContextObject_type{};
    struct delete_context_type{};
    struct new_head_ready_type{};
    struct routine_return_type{};
    struct appupcall_return_type{};
    struct routine_type{};
    struct new_event_request_type{};
		struct UpdateContextMapping_type{};
    struct CommitDone_type {};
    struct ContextMappingUpdateReq_type {};
    struct ContextMappingUpdateSuggest_type {};
    struct ContextOwnershipControl_type {};
    struct UpdateOwnership_type {};
    struct ContextStructureUpdateReq_type {};
    struct MigrationControl_type {};
    struct ExternalCommControl_type {};
    struct BroadcastControl_type {};
    struct EventExecutionControl_type {};
    struct ElasticityControl_type {};
    struct EnqueueEventRequest_type {};
    struct EnqueueEventReply_type {};
    struct EnqueueMessageRequest_type {};
    struct EnqueueMessageReply_type {};
    struct CommitContextMigration_type {};

  const static uint8_t UNKNOWN = 0;
  const static uint8_t ALLOCATE_CONTEXT_OBJECT = 1;
  const static uint8_t CONTEXT_MIGRATION_REQUEST = 3;
  const static uint8_t TRANSFER_CONTEXT = 4;
  const static uint8_t CREATE = 5;
  const static uint8_t CREATE_HEAD = 6;
  const static uint8_t CREATE_RESPONSE = 7;
  const static uint8_t EXIT_COMMITTED = 8;
  const static uint8_t ENTER_CONTEXT = 9;
  const static uint8_t COMMIT = 10;
  const static uint8_t COMMIT_CONTEXT = 11;
  const static uint8_t SNAPSHOT = 12;
  const static uint8_t DOWNGRADE_CONTEXT = 13;
  const static uint8_t EVICT = 14;
  const static uint8_t MIGRATE_CONTEXT = 15;
  const static uint8_t MIGRATE_PARAM = 16;
  const static uint8_t REMOVE_CONTEXT_OBJECT = 17;
  const static uint8_t DELETE_CONTEXT = 18;
  const static uint8_t NEW_HEAD_READY = 19;
  const static uint8_t ROUTINE_RETURN = 20;
  const static uint8_t ASYNC_EVENT = 21;
  const static uint8_t APPUPCALL = 22;
  const static uint8_t APPUPCALL_RETURN = 23;
  const static uint8_t ROUTINE = 24;
  const static uint8_t TRANSITION_CALL = 25;
  const static uint8_t NEW_EVENT_REQUEST = 26;
	const static uint8_t ALLOCATE_CONTEXT_OBJ_REQ = 27;
	const static uint8_t ALLOCATE_CONTEXT_OBJ_RESPONSE = 28;
	const static uint8_t UPDATE_CONTEXT_MAPPING = 29;
  const static uint8_t COMMIT_SINGLE_CONTEXT = 30;
  const static uint8_t COMMIT_DONE = 31;
  const static uint8_t CONTEXTMAPPING_UPDATE_REQ = 32;
  const static uint8_t CONTEXT_OWNERSHIP_CONTROL = 33;
  const static uint8_t CONTEXTMAPPING_UPDATE_SUGGEST = 34;
  const static uint8_t MIGRATION_CONTROL = 35;
  const static uint8_t EXTERNALCOMM_CONTROL = 36;
  const static uint8_t BROADCAST_CONTROL = 37;
  const static uint8_t EVENT_EXECUTION_CONTROL = 38;
  const static uint8_t ENQUEUE_EVENT_REQUEST = 39;
  const static uint8_t ENQUEUE_EVENT_REPLY = 40;
  const static uint8_t ENQUEUE_MESSAGE_REQUEST = 41;
  const static uint8_t ENQUEUE_MESSAGE_REPLY = 42;
  const static uint8_t ELASTICITY_CONTROL = 43;
  const static uint8_t COMMIT_CONTEXT_MIGRATION = 44;
  
    InternalMessage() {}
    InternalMessage( AllocateContextObject_type t, MaceAddr const & destNode, mace::map< uint32_t, mace::string > const& ContextID, 
      mace::OrderID const & eventID, mace::ContextMapping const & contextMapping, int8_t const & eventType, const uint64_t& version,
      const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, const mace::map<mace::string, uint64_t>& vers): 
      msgType( ALLOCATE_CONTEXT_OBJECT ), msgId( ), helper(new AllocateContextObject_Message(destNode, ContextID, eventID, contextMapping, 
      eventType, version, ownershipPairs, vers) ) {}
    //bsang
    InternalMessage( CommitDone_type t, InternalMessageID const & msgId, mace::string const& create_ctx_name, mace::string const& target_ctx_name, 
      mace::OrderID const& eventId, mace::set<mace::string> const& coaccess_contexts ): msgType( COMMIT_DONE ),
      msgId(msgId), helper( new CommitDone_Message(create_ctx_name, target_ctx_name, eventId, coaccess_contexts ) ) { }
		InternalMessage( AllocateContextObjReq_type t, const mace::string& ctxName, const OrderID& eventId, 
      const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs,
      const mace::map< mace::string, uint64_t>& vers ): 
      msgType( ALLOCATE_CONTEXT_OBJ_REQ ), msgId( ), 
      helper( new AllocateContextObjReq_Message(ctxName, eventId, ownershipPairs, vers ) ) { }
		InternalMessage( AllocateContextObjectResponse_type t, mace::string const& ctx_name, mace::OrderID const& eventId, bool const& isCreateObjectEvent): msgType( ALLOCATE_CONTEXT_OBJ_RESPONSE ), 
      msgId( ), helper( new AllocateContextObjectResponse_Message(ctx_name, eventId, isCreateObjectEvent) ) { }
		InternalMessage( UpdateContextMapping_type t, mace::ContextMapping const& ctxMapping, mace::string const& ctxName): msgType( UPDATE_CONTEXT_MAPPING ), msgId( ), helper( new UpdateContextMapping_Message(ctxMapping, ctxName)) { }
		InternalMessage( ContextMappingUpdateReq_type t, uint64_t const & expectVersion ): msgType( CONTEXTMAPPING_UPDATE_REQ ), msgId( ), helper( new ContextMappingUpdateReq_Message(expectVersion)) { }
    
    InternalMessage( ContextOwnershipControl_type t, InternalMessageID const& msgId, const uint8_t& type, mace::vector<mace::EventOperationInfo> const& ownershipOpInfos, 
      mace::string const& dest_contextName, mace::string const& src_contextName, mace::OrderID const& eventId,  
      mace::vector< mace::pair<mace::string, mace::string> > const& ownerships, const mace::map<mace::string, uint64_t>& versions, 
      mace::set<mace::string> const& contextSet, mace::vector<mace::string> const& contextVector ) : 
      msgType( CONTEXT_OWNERSHIP_CONTROL ), msgId(msgId), 
      helper( new ContextOwnershipControl_Message(type, ownershipOpInfos, dest_contextName, src_contextName, eventId, ownerships, versions, 
        contextSet, contextVector) ) { }
    
    InternalMessage( ContextMappingUpdateSuggest_type t, uint64_t const & expectVersion ): msgType( CONTEXTMAPPING_UPDATE_SUGGEST ), msgId( ), helper( new ContextMappingUpdateSuggest_Message(expectVersion)) { }
    
    InternalMessage( MigrationControl_type t, uint8_t const& control_type, uint64_t const& ticket, 
      mace::map<uint32_t, mace::string> const& migrate_contexts, mace::ContextMapping const& ctxMapping, mace::MaceAddr const& srcAddr):
      msgType( MIGRATION_CONTROL), msgId(), helper( new MigrationControl_Message(control_type, ticket, migrate_contexts, ctxMapping, srcAddr ) ) { }
    InternalMessage( ExternalCommControl_type t, uint8_t const& control_type, uint32_t const& externalCommId, uint32_t const& externalCommContextId ): msgType( EXTERNALCOMM_CONTROL ), 
      msgId(), helper( new ExternalCommControl_Message(control_type, externalCommId, externalCommContextId) ) { }
    InternalMessage( BroadcastControl_type t, InternalMessageID const& msgId, uint8_t const& control_type, mace::string const& targetContextName, mace::string const& parentContextName, 
      mace::OrderID const& eventId, mace::OrderID const& bEventId, mace::map<mace::string, mace::set<mace::string> > const& cpRelations, mace::set<mace::string> const& targetContextNames): 
      msgType( BROADCAST_CONTROL), msgId( msgId ), 
      helper( new BroadcastControl_Message(control_type, targetContextName, parentContextName, eventId, bEventId, cpRelations, targetContextNames) ) { }
    
    InternalMessage( EventExecutionControl_type t, InternalMessageID const& msgId, const uint8_t& control_type, 
      mace::string const& src_contextName, mace::string const& dest_contextName, 
      mace::OrderID const& eventId, mace::vector<mace::string> const& contextNames, mace::vector<mace::EventOperationInfo> const& opInfos, 
      uint32_t const& newContextId ): msgType( EVENT_EXECUTION_CONTROL), msgId( msgId ), 
      helper( new EventExecutionControl_Message(control_type, src_contextName, dest_contextName, eventId, contextNames,
              opInfos, newContextId ) ) { }

    InternalMessage( ElasticityControl_type t, InternalMessageID const& msgId, const uint8_t& controlType, const uint32_t& handlerThreadId,
      mace::string const& contextName, mace::MaceAddr const& addr, const uint64_t& value, const mace::vector<mace::string>& contextNames,
      mace::map<mace::MaceAddr, mace::ServerRuntimeInfo> const& serversInfo, mace::vector<mace::MigrationContextInfo> const& migrationContextsInfo ): 
      msgType( ELASTICITY_CONTROL), msgId( msgId ), 
      helper( new ElasticityControl_Message(controlType, handlerThreadId, contextName, addr, value, contextNames, serversInfo, migrationContextsInfo ) ) { }

    InternalMessage( CommitContextMigration_type t, InternalMessageID const& msgId, const mace::OrderID& eventId, mace::string const& contextName, 
      mace::MaceAddr const& srcAddr, mace::MaceAddr const& destAddr ): msgType( COMMIT_CONTEXT_MIGRATION ), msgId( msgId ), 
      helper( new commit_context_migration_Message( eventId, contextName, srcAddr, destAddr ) ) { }

    InternalMessage( EnqueueEventRequest_type t, InternalMessageID const& msgId, mace::EventOperationInfo const& eventOpInfo, 
      mace::string const& dest_contextName,
      mace::string const& src_contextName, mace::EventRequestWrapper const& request): msgType( ENQUEUE_EVENT_REQUEST), msgId( msgId ),
      helper( new EnqueueEventRequest_Message(eventOpInfo, dest_contextName, src_contextName, request) ) { }
    InternalMessage( EnqueueEventReply_type t, InternalMessageID const& msgId, mace::EventOperationInfo const& eventOpInfo, 
      mace::string const& dest_contextName, mace::string const& src_contextName): msgType( ENQUEUE_EVENT_REPLY), msgId( msgId ),
      helper( new EnqueueEventReply_Message(eventOpInfo, dest_contextName, src_contextName) ) { }

    InternalMessage( EnqueueMessageRequest_type t, InternalMessageID const& msgId, mace::EventOperationInfo const& eventOpInfo, 
      mace::string const& dest_contextName,
      mace::string const& src_contextName, mace::EventMessageRecord const& msg): msgType( ENQUEUE_MESSAGE_REQUEST), msgId( msgId ),
      helper( new EnqueueMessageRequest_Message(eventOpInfo, dest_contextName, src_contextName, msg) ) { }
    InternalMessage( EnqueueMessageReply_type t, InternalMessageID const& msgId, mace::EventOperationInfo const& eventOpInfo, 
      mace::string const& dest_contextName, mace::string const& src_contextName): msgType( ENQUEUE_MESSAGE_REPLY), msgId( msgId ),
      helper( new EnqueueMessageReply_Message(eventOpInfo, dest_contextName, src_contextName) ) { }

    InternalMessage( ContextMigrationRequest_type t, MaceAddr const & my_dest, mace::Event const & my_event, uint64_t const & my_prevContextMapVersion, mace::set< uint32_t > const & my_migrateContextIds, mace::ContextMapping const& my_contextMapping): 
      msgType( CONTEXT_MIGRATION_REQUEST), msgId( ), helper(new ContextMigrationRequest_Message(my_dest, my_event, my_prevContextMapVersion, my_migrateContextIds, my_contextMapping) ) {} // TODO: WC: change to a better name
    InternalMessage( TransferContext_type t, const mace::ContextBaseClassParams* my_ctxParams, mace::string const & my_checkpoint, mace::OrderID const & my_eventId ): msgType( TRANSFER_CONTEXT), 
      msgId( ), helper(new TransferContext_Message( my_ctxParams, my_checkpoint, my_eventId ) ) {}
    InternalMessage( create_type t, __asyncExtraField const & my_extra, uint64_t const & my_counter, uint32_t const & my_create_ctxId): 
      msgType( CREATE), msgId( ), helper(new create_Message( my_extra, my_counter, my_create_ctxId ) ) {}
    InternalMessage( create_head_type t, __asyncExtraField const & my_extra, uint64_t const & my_counter, uint64_t const & my_ctxID, MaceAddr const & my_src): msgType( CREATE_HEAD), msgId( ),
      helper(new create_head_Message( my_extra, my_counter, my_ctxID, my_src) ) {}
    InternalMessage( create_response_type t, mace::Event const & my_event, uint32_t const & my_counter, MaceAddr const & my_targetAddress): msgType( CREATE_RESPONSE ), msgId( ),
      helper(new create_response_Message( my_event, my_counter, my_targetAddress) ) {}
    InternalMessage( exit_committed_type t ): msgType( EXIT_COMMITTED), msgId(), helper(new exit_committed_Message() ) {}
    InternalMessage( enter_context_type t, mace::Event const & my_event, mace::vector< uint32_t > const & my_contextIDs): msgType( ENTER_CONTEXT), msgId(), helper(new enter_context_Message( my_event, my_contextIDs) ) {}
    InternalMessage( commit_type t, InternalMessageID const& msgId, mace::Event const & my_event): msgType( COMMIT), msgId(msgId), helper(new commit_Message( my_event ) ) {}
    
    InternalMessage( commit_context_type t, InternalMessageID const& msgId, mace::vector< uint32_t > const & my_nextHops, mace::OrderID const & my_eventID, 
      int8_t const & my_eventType, uint64_t const & my_eventContextMappingVersion,  
      bool const & my_isresponse, bool const & my_hasException, uint32_t const & my_exceptionContextID, mace::set<mace::string> const& my_accessCtxs, mace::OrderID const& my_bEventId): msgType( COMMIT_CONTEXT), msgId( msgId ), helper(new commit_context_Message( my_nextHops, my_eventID, my_eventType, 
      my_eventContextMappingVersion, my_isresponse, my_hasException, my_exceptionContextID, my_accessCtxs, my_bEventId) ) {} // TODO: WC: rename it. it should be renamed to downgrade
    
    InternalMessage( snapshot_type t, mace::Event const & my_event, mace::string const & my_ctxID, mace::string const & my_snapshotContextID, mace::string const & my_snapshot): 
      msgType( SNAPSHOT), msgId( ), helper(new snapshot_Message( my_event, my_ctxID, my_snapshotContextID, my_snapshot) ) {}
    InternalMessage( downgrade_context_type t, uint32_t const & my_contextID, mace::OrderID const & my_eventID, bool const & my_isresponse): msgType( DOWNGRADE_CONTEXT ), msgId(),
      helper(new downgrade_context_Message( my_contextID, my_eventID, my_isresponse) ) {} // TODO: may be this is a duplicate of commit_context?
    InternalMessage( evict_type t ): msgType( EVICT ), msgId(), helper(new evict_Message( ) ) {}
    InternalMessage( migrate_context_type t, mace::MaceAddr const & my_newNode, mace::string const & my_contextName, uint64_t const & my_delay): msgType( MIGRATE_CONTEXT), msgId(),
      helper(new migrate_context_Message( my_newNode, my_contextName, my_delay ) ) {}
    InternalMessage( migrate_param_type t, mace::string const & my_paramid): msgType( MIGRATE_PARAM ), msgId(), helper(new migrate_param_Message(my_paramid) ) {}
    InternalMessage( RemoveContextObject_type t, mace::OrderID const & my_eventID, mace::ContextMapping const & my_ctxmapCopy, 
      MaceAddr const & my_dest, uint32_t const & my_contextID): msgType( REMOVE_CONTEXT_OBJECT), msgId(),
      helper(new RemoveContextObject_Message( my_eventID, my_ctxmapCopy, my_dest, my_contextID) ) {}
    InternalMessage( delete_context_type t, mace::string const & my_contextName): msgType( DELETE_CONTEXT), msgId(), helper(new delete_context_Message( my_contextName ) ) {}
    InternalMessage( new_head_ready_type t): msgType( NEW_HEAD_READY), msgId(), helper(new new_head_ready_Message() ) {}
    InternalMessage( routine_return_type t, mace::string const & my_returnValue, mace::Event const & my_event): msgType( ROUTINE_RETURN), msgId(),
      helper(new routine_return_Message( my_returnValue, my_event) ) {} 
    InternalMessage( appupcall_return_type t, mace::string const & my_returnValue, mace::Event const & my_event): msgType( APPUPCALL_RETURN), msgId(),
      helper(new appupcall_return_Message( my_returnValue, my_event) ) {}


    InternalMessage( mace::AsyncEvent_Message* m, InternalMessageID const& msgId, uint8_t sid): msgType( ASYNC_EVENT ), sid(sid), msgId(msgId), helper(m ) {}

    InternalMessage( mace::ApplicationUpcall_Message* m, uint8_t sid): msgType( APPUPCALL ), sid(sid), helper(m ) {}

    InternalMessage( mace::Routine_Message* m, InternalMessageID const& msgId, uint8_t sid): msgType( ROUTINE ), sid(sid), msgId(msgId), helper(m ) {}

    InternalMessage( mace::Transition_Message* m, uint8_t sid): msgType( TRANSITION_CALL ), sid(sid), helper(m ) {}


    InternalMessage( new_event_request_type t, mace::AsyncEvent_Message* m, uint8_t sid): msgType( NEW_EVENT_REQUEST ), sid(sid), helper(m ) {}

    /// copy constructor
    InternalMessage( InternalMessage const& orig ){
      msgType = orig.msgType;
      switch( orig.msgType ){

  case UNKNOWN: break;
  case ALLOCATE_CONTEXT_OBJECT: helper = InternalMessageHelperPtr( new AllocateContextObject_Message(orig.getHelper() ) ); break;

  case ALLOCATE_CONTEXT_OBJ_RESPONSE: helper = InternalMessageHelperPtr( new AllocateContextObjectResponse_Message(orig.getHelper()) ); break;
  case ALLOCATE_CONTEXT_OBJ_REQ: helper = InternalMessageHelperPtr( new AllocateContextObjReq_Message(orig.getHelper()) ); break;
  case UPDATE_CONTEXT_MAPPING: helper = InternalMessageHelperPtr( new UpdateContextMapping_Message(orig.getHelper()) ); break;
  case COMMIT_DONE: helper = InternalMessageHelperPtr( new CommitDone_Message(orig.getHelper()) ); break;
  case CONTEXTMAPPING_UPDATE_REQ: helper = InternalMessageHelperPtr( new ContextMappingUpdateReq_Message(orig.getHelper()) ); break;
  case CONTEXT_OWNERSHIP_CONTROL: helper = InternalMessageHelperPtr( new ContextOwnershipControl_Message(orig.getHelper()) ); break;
  case CONTEXTMAPPING_UPDATE_SUGGEST: helper = InternalMessageHelperPtr( new ContextMappingUpdateSuggest_Message(orig.getHelper()) ); break;
  case MIGRATION_CONTROL: helper = InternalMessageHelperPtr( new MigrationControl_Message(orig.getHelper()) ); break;
  case EXTERNALCOMM_CONTROL: helper = InternalMessageHelperPtr( new ExternalCommControl_Message(orig.getHelper()) ); break;
  case BROADCAST_CONTROL: helper = InternalMessageHelperPtr( new BroadcastControl_Message(orig.getHelper()) ); break;
  case EVENT_EXECUTION_CONTROL: helper = InternalMessageHelperPtr( new EventExecutionControl_Message(orig.getHelper()) ); break;
  case ELASTICITY_CONTROL: helper = InternalMessageHelperPtr( new ElasticityControl_Message(orig.getHelper()) ); break;
  case ENQUEUE_EVENT_REQUEST: helper = InternalMessageHelperPtr( new EnqueueEventRequest_Message(orig.getHelper()) ); break;
  case ENQUEUE_EVENT_REPLY: helper = InternalMessageHelperPtr( new EnqueueEventReply_Message(orig.getHelper()) ); break;
  case ENQUEUE_MESSAGE_REQUEST: helper = InternalMessageHelperPtr( new EnqueueMessageRequest_Message(orig.getHelper()) ); break;
  case ENQUEUE_MESSAGE_REPLY: helper = InternalMessageHelperPtr( new EnqueueMessageReply_Message(orig.getHelper()) ); break;

  case CONTEXT_MIGRATION_REQUEST: helper = InternalMessageHelperPtr( new ContextMigrationRequest_Message(orig.getHelper()) ); break;
  case TRANSFER_CONTEXT: helper = InternalMessageHelperPtr( new TransferContext_Message(orig.getHelper()) ); break;
  case CREATE: helper = InternalMessageHelperPtr( new create_Message(orig.getHelper()) ); break;
  case CREATE_HEAD: helper = InternalMessageHelperPtr( new create_head_Message(orig.getHelper()) ); break; 
  case CREATE_RESPONSE: helper = InternalMessageHelperPtr( new create_response_Message(orig.getHelper()) ); break;
  case EXIT_COMMITTED: helper = InternalMessageHelperPtr( new exit_committed_Message(orig.getHelper()) ); break;
  case ENTER_CONTEXT: helper = InternalMessageHelperPtr( new enter_context_Message(orig.getHelper()) ); break;
  case COMMIT: helper = InternalMessageHelperPtr( new commit_Message(orig.getHelper()) ); break;
  case COMMIT_CONTEXT: helper = InternalMessageHelperPtr( new commit_context_Message(orig.getHelper()) ); break;
  case SNAPSHOT: helper = InternalMessageHelperPtr( new snapshot_Message(orig.getHelper()) ); break;
  case DOWNGRADE_CONTEXT: helper = InternalMessageHelperPtr( new downgrade_context_Message(orig.getHelper()) ); break;
  case EVICT: helper = InternalMessageHelperPtr( new evict_Message(orig.getHelper()) ); break;
  case MIGRATE_CONTEXT: helper = InternalMessageHelperPtr( new migrate_context_Message(orig.getHelper()) ); break;
  case MIGRATE_PARAM: helper = InternalMessageHelperPtr( new migrate_param_Message(orig.getHelper()) ); break;
  case REMOVE_CONTEXT_OBJECT: helper = InternalMessageHelperPtr( new RemoveContextObject_Message(orig.getHelper()) ); break;
  case DELETE_CONTEXT: helper = InternalMessageHelperPtr( new delete_context_Message(orig.getHelper()) ); break;
  case NEW_HEAD_READY: helper = InternalMessageHelperPtr( new new_head_ready_Message(orig.getHelper()) ); break;
  case ROUTINE_RETURN: helper = InternalMessageHelperPtr( new routine_return_Message(orig.getHelper()) ); break;
  case APPUPCALL_RETURN: helper = InternalMessageHelperPtr( new appupcall_return_Message(orig.getHelper())); break;
  case COMMIT_CONTEXT_MIGRATION: helper = InternalMessageHelperPtr( new commit_context_migration_Message(orig.getHelper()) ); break;
  
  default:
      helper = orig.helper;
  } 


      switch( orig.msgType ){

        case ASYNC_EVENT: 
        case APPUPCALL: 
        case ROUTINE: 
        case TRANSITION_CALL: 
        case NEW_EVENT_REQUEST:
          sid = orig.sid;
          break;
      }

      msgId = orig.msgId;
    }

    void print(std::ostream& out) const {
      if( helper != NULL ){
        out << (*helper);
      }
    }
    virtual void serialize(std::string& str) const {
      mace::serialize(str, &messageType);
      mace::serialize(str, &msgType);
      mace::serialize(str, &msgId);
      if(msgType != UNKNOWN && helper != NULL) {
        switch( msgType ){
          case ASYNC_EVENT:
            mace::serialize(str, &sid);
            break;
          case APPUPCALL:
            mace::serialize(str, &(ThreadStructure::myEvent() ) );
            mace::serialize(str, &sid);
            break;
          case ROUTINE:
            mace::serialize(str, &sid);
            break;
          case TRANSITION_CALL:
            mace::serialize(str, &sid);
            break;
          case NEW_EVENT_REQUEST:
            mace::serialize(str, &sid);
            break;
        }
        helper->serialize(str);
      }
    }
    virtual int deserialize(std::istream& in) throw(SerializationException) {
      int count = 0;
      uint8_t t;
      count = mace::deserialize(in, &t);
      ASSERT( t == messageType );
      count += mace::deserialize(in, &msgType);
      count += mace::deserialize(in, &msgId);

      switch( msgType ){
  case UNKNOWN: return count; break;
  case ALLOCATE_CONTEXT_OBJECT: helper = InternalMessageHelperPtr( new AllocateContextObject_Message() ); break;
  case ALLOCATE_CONTEXT_OBJ_RESPONSE: helper = InternalMessageHelperPtr( new AllocateContextObjectResponse_Message() ); break;
  case ALLOCATE_CONTEXT_OBJ_REQ: helper = InternalMessageHelperPtr( new AllocateContextObjReq_Message() ); break;
  case UPDATE_CONTEXT_MAPPING: helper = InternalMessageHelperPtr( new UpdateContextMapping_Message() ); break;
  case COMMIT_DONE: helper = InternalMessageHelperPtr( new CommitDone_Message() ); break;
  case COMMIT_CONTEXT_MIGRATION: helper = InternalMessageHelperPtr( new commit_context_migration_Message() ); break;
  case CONTEXT_OWNERSHIP_CONTROL: helper = InternalMessageHelperPtr( new ContextOwnershipControl_Message() ); break;
  case CONTEXTMAPPING_UPDATE_REQ: helper = InternalMessageHelperPtr( new ContextMappingUpdateReq_Message() ); break;
  case CONTEXTMAPPING_UPDATE_SUGGEST: helper = InternalMessageHelperPtr( new ContextMappingUpdateSuggest_Message() ); break;
  case CONTEXT_MIGRATION_REQUEST: helper = InternalMessageHelperPtr( new ContextMigrationRequest_Message() ); break;
  case TRANSFER_CONTEXT: helper = InternalMessageHelperPtr( new TransferContext_Message() ); break;
  case MIGRATION_CONTROL: helper = InternalMessageHelperPtr( new MigrationControl_Message() ); break;
  case BROADCAST_CONTROL: helper = InternalMessageHelperPtr( new BroadcastControl_Message() ); break;
  case EVENT_EXECUTION_CONTROL: helper = InternalMessageHelperPtr( new EventExecutionControl_Message() ); break;
  case ELASTICITY_CONTROL: helper = InternalMessageHelperPtr( new ElasticityControl_Message() ); break;
  case ENQUEUE_EVENT_REQUEST: helper = InternalMessageHelperPtr( new EnqueueEventRequest_Message() ); break;
  case ENQUEUE_EVENT_REPLY: helper = InternalMessageHelperPtr( new EnqueueEventReply_Message() ); break;
  case ENQUEUE_MESSAGE_REQUEST: helper = InternalMessageHelperPtr( new EnqueueMessageRequest_Message() ); break;
  case ENQUEUE_MESSAGE_REPLY: helper = InternalMessageHelperPtr( new EnqueueMessageReply_Message() ); break;
  case EXTERNALCOMM_CONTROL: helper = InternalMessageHelperPtr( new ExternalCommControl_Message() ); break;
  case CREATE: helper = InternalMessageHelperPtr( new create_Message() ); break;
  case CREATE_HEAD: helper = InternalMessageHelperPtr( new create_head_Message() ); break; 
  case CREATE_RESPONSE: helper = InternalMessageHelperPtr( new create_response_Message() ); break;
  case EXIT_COMMITTED: helper = InternalMessageHelperPtr( new exit_committed_Message() ); break;
  case ENTER_CONTEXT: helper = InternalMessageHelperPtr( new enter_context_Message() ); break;
  case COMMIT: helper = InternalMessageHelperPtr( new commit_Message() ); break;
  case COMMIT_CONTEXT: helper = InternalMessageHelperPtr( new commit_context_Message() ); break;
  case SNAPSHOT: helper = InternalMessageHelperPtr( new snapshot_Message() ); break;
  case DOWNGRADE_CONTEXT: helper = InternalMessageHelperPtr( new downgrade_context_Message() ); break;
  case EVICT: helper = InternalMessageHelperPtr( new evict_Message() ); break;
  case MIGRATE_CONTEXT: helper = InternalMessageHelperPtr( new migrate_context_Message() ); break;
  case MIGRATE_PARAM: helper = InternalMessageHelperPtr( new migrate_param_Message() ); break;
  case REMOVE_CONTEXT_OBJECT: helper = InternalMessageHelperPtr( new RemoveContextObject_Message() ); break;
  case DELETE_CONTEXT: helper = InternalMessageHelperPtr( new delete_context_Message() ); break;
  case NEW_HEAD_READY: helper = InternalMessageHelperPtr( new new_head_ready_Message() ); break;
  case ROUTINE_RETURN: helper = InternalMessageHelperPtr( new routine_return_Message() ); break;
  case APPUPCALL_RETURN: helper = InternalMessageHelperPtr( new appupcall_return_Message()); break;

  case ASYNC_EVENT: {
    count += mace::deserialize(in, &sid );
    count += deserializeEvent( in );
    return count;
  }
  case APPUPCALL: {
    // these are the application upcalls that return a value (either have a return value, or have non-const reference parameter )
    // this internal message updates the current event
    count += mace::deserialize(in, &(ThreadStructure::myEvent() ) );
    count += mace::deserialize(in, &sid );
    count += deserializeUpcall( in );
    return count;
  }
  case ROUTINE: {
    count += mace::deserialize(in, &sid );
    count += deserializeRoutine( in );
    return count;
  }
  case TRANSITION_CALL: {
    count += mace::deserialize(in, &sid );
    count += deserializeRoutine( in );
    return count;
  }
  case NEW_EVENT_REQUEST: {
    count += mace::deserialize(in, &sid );
    count += deserializeEvent( in );
    return count;
  }
  default: throw(InvalidInternalMessageException("Deserializing bad internal message type "+boost::lexical_cast<std::string>(msgType)+"!"));
    
  }
  count += helper->deserialize(in); 
  return count;
}
    virtual ~InternalMessage() { 
#ifndef INTERNALMESSAGE_USE_SHARED_PTR
      delete helper; 
#endif
    }
};

 const mace::InternalMessage::AllocateContextObject_type AllocateContextObject = mace::InternalMessage::AllocateContextObject_type();
 const mace::InternalMessage::AllocateContextObjectResponse_type AllocateContextObjectResponse = mace::InternalMessage::AllocateContextObjectResponse_type();
 const mace::InternalMessage::AllocateContextObjReq_type AllocateContextObjReq = mace::InternalMessage::AllocateContextObjReq_type();
 const mace::InternalMessage::UpdateContextMapping_type UpdateContextMapping = mace::InternalMessage::UpdateContextMapping_type();
 const mace::InternalMessage::ContextMigrationRequest_type ContextMigrationRequest = mace::InternalMessage::ContextMigrationRequest_type(); // TODO: WC: change to a better name
 const mace::InternalMessage::TransferContext_type TransferContext = mace::InternalMessage::TransferContext_type();
 const mace::InternalMessage::create_type create = mace::InternalMessage::create_type();
 const mace::InternalMessage::create_head_type create_head = mace::InternalMessage::create_head_type();
 const mace::InternalMessage::create_response_type create_response = mace::InternalMessage::create_response_type();
 const mace::InternalMessage::exit_committed_type exit_committed = mace::InternalMessage::exit_committed_type();
 const mace::InternalMessage::enter_context_type enter_context = mace::InternalMessage::enter_context_type();
 const mace::InternalMessage::commit_type commit = mace::InternalMessage::commit_type();
 const mace::InternalMessage::commit_context_type commit_context = mace::InternalMessage::commit_context_type(); // TODO: WC: rename it. it should be renamed to downgrade
 const mace::InternalMessage::commit_single_context_type commit_single_context = mace::InternalMessage::commit_single_context_type();
 const mace::InternalMessage::snapshot_type snapshot = mace::InternalMessage::snapshot_type();
 const mace::InternalMessage::downgrade_context_type downgrade_context = mace::InternalMessage::downgrade_context_type(); // TODO: may be this is a duplicate of commit_context?
 const mace::InternalMessage::evict_type evict = mace::InternalMessage::evict_type();
 const mace::InternalMessage::migrate_context_type migrate_context = mace::InternalMessage::migrate_context_type();
 const mace::InternalMessage::migrate_param_type migrate_param = mace::InternalMessage::migrate_param_type();
 const mace::InternalMessage::RemoveContextObject_type RemoveContextObject = mace::InternalMessage::RemoveContextObject_type();
 const mace::InternalMessage::delete_context_type delete_context = mace::InternalMessage::delete_context_type();
 const mace::InternalMessage::new_head_ready_type new_head_ready = mace::InternalMessage::new_head_ready_type();
 const mace::InternalMessage::routine_return_type routine_return = mace::InternalMessage::routine_return_type();
 const mace::InternalMessage::appupcall_return_type appupcall_return = mace::InternalMessage::appupcall_return_type();
 const mace::InternalMessage::new_event_request_type new_event_request = mace::InternalMessage::new_event_request_type();

 const mace::InternalMessage::CommitDone_type CommitDone = mace::InternalMessage::CommitDone_type();
 const mace::InternalMessage::ContextMappingUpdateReq_type ContextMappingUpdateReq = mace::InternalMessage::ContextMappingUpdateReq_type();
 const mace::InternalMessage::ContextOwnershipControl_type ContextOwnershipControl = mace::InternalMessage::ContextOwnershipControl_type();
 const mace::InternalMessage::UpdateOwnership_type UpdateOwnership = mace::InternalMessage::UpdateOwnership_type();
 const mace::InternalMessage::ContextStructureUpdateReq_type ContextStructureUpdateReq = mace::InternalMessage::ContextStructureUpdateReq_type();
 const mace::InternalMessage::ContextMappingUpdateSuggest_type ContextMappingUpdateSuggest = mace::InternalMessage::ContextMappingUpdateSuggest_type();
 const mace::InternalMessage::MigrationControl_type MigrationControl = mace::InternalMessage::MigrationControl_type();
 const mace::InternalMessage::ExternalCommControl_type ExternalCommControl = mace::InternalMessage::ExternalCommControl_type();
 const mace::InternalMessage::BroadcastControl_type BroadcastControl = mace::InternalMessage::BroadcastControl_type();
 const mace::InternalMessage::EventExecutionControl_type EventExecutionControl = mace::InternalMessage::EventExecutionControl_type(); 
 const mace::InternalMessage::ElasticityControl_type ElasticityControl = mace::InternalMessage::ElasticityControl_type();
 const mace::InternalMessage::EnqueueEventRequest_type EnqueueEventRequest = mace::InternalMessage::EnqueueEventRequest_type();
 const mace::InternalMessage::EnqueueEventReply_type EnqueueEventReply = mace::InternalMessage::EnqueueEventReply_type();
 const mace::InternalMessage::EnqueueMessageRequest_type EnqueueMessageRequest = mace::InternalMessage::EnqueueMessageRequest_type();
 const mace::InternalMessage::EnqueueMessageReply_type EnqueueMessageReply = mace::InternalMessage::EnqueueMessageReply_type();
 const mace::InternalMessage::CommitContextMigration_type CommitContextMigration = mace::InternalMessage::CommitContextMigration_type();
}
#endif
