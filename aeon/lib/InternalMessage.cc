

#include "InternalMessage.h"
#include "ContextBaseClass.h"

const uint8_t mace::InternalMessage::messageType;

const uint8_t mace::EventExecutionControl_Message::COMMIT_CONTEXTS;
const uint8_t mace::EventExecutionControl_Message::COMMIT_CONTEXT;
const uint8_t mace::EventExecutionControl_Message::TO_LOCK_CONTEXT;
const uint8_t mace::EventExecutionControl_Message::NEW_EVENT_OP;
const uint8_t mace::EventExecutionControl_Message::EVENT_OP_DONE;
const uint8_t mace::EventExecutionControl_Message::CREATE_NEW_CONTEXT;
const uint8_t mace::EventExecutionControl_Message::CREATE_NEW_CONTEXT_REPLY;
const uint8_t mace::EventExecutionControl_Message::RELEASE_CONTEXTS;
const uint8_t mace::EventExecutionControl_Message::LOCK_CONTEXT_PERMISSION;
const uint8_t mace::EventExecutionControl_Message::RELEASE_CONTEXT_REPLY;
const uint8_t mace::EventExecutionControl_Message::UNLOCK_CONTEXT;
const uint8_t mace::EventExecutionControl_Message::CHECK_COMMIT;
const uint8_t mace::EventExecutionControl_Message::READY_TO_COMMIT;
const uint8_t mace::EventExecutionControl_Message::RELEASE_LOCK_ON_CONTEXT;
const uint8_t mace::EventExecutionControl_Message::ENQUEUE_LOCK_CONTEXTS;
const uint8_t mace::EventExecutionControl_Message::ENQUEUE_OWNERSHIP_OPS;
const uint8_t mace::EventExecutionControl_Message::ENQUEUE_OWNERSHIP_OPS_REPLY;

const uint8_t mace::EventExecutionControl_Message::PRINT_CTX_INFO;

const uint8_t mace::ElasticityControl_Message::CONTEXT_NODE_STRENGTH_REQ;
const uint8_t mace::ElasticityControl_Message::CONTEXT_NODE_STRENGTH_REP;
const uint8_t mace::ElasticityControl_Message::CONTEXTS_COLOCATE_REQ;
const uint8_t mace::ElasticityControl_Message::SERVER_INFO_REPORT;
const uint8_t mace::ElasticityControl_Message::SERVER_INFO_REQ;
const uint8_t mace::ElasticityControl_Message::SERVER_INFO_REPLY;
const uint8_t mace::ElasticityControl_Message::CONTEXTS_MIGRATION_QUERY;
const uint8_t mace::ElasticityControl_Message::CONTEXTS_MIGRATION_QUERY_REPLY;
const uint8_t mace::ElasticityControl_Message::INACTIVATE_CONTEXT;
const uint8_t mace::ElasticityControl_Message::ACTIVATE_CONTEXT;

const uint8_t mace::ContextOwnershipControl_Message::OWNERSHIP_MODIFY;
const uint8_t mace::ContextOwnershipControl_Message::OWNERSHIP_UPDATE;
const uint8_t mace::ContextOwnershipControl_Message::OWNERSHIP_UPDATE_REQUEST;
const uint8_t mace::ContextOwnershipControl_Message::OWNERSHIP_MODIFY_DONE;
const uint8_t mace::ContextOwnershipControl_Message::OWNERSHIP_UPDATE_DONE;
const uint8_t mace::ContextOwnershipControl_Message::OWNERSHIP_AND_DOMINATOR_UPDATE;
const uint8_t mace::ContextOwnershipControl_Message::OWNERSHIP_UPDATE_REPLY;

const uint8_t mace::MigrationControl_Message::MIGRATION_UPDATE_CONTEXTMAPPING;
const uint8_t mace::MigrationControl_Message::MIGRATION_DONE;
const uint8_t mace::MigrationControl_Message::MIGRATION_CTX_REQUEST;

int mace::InternalMessage::deserializeEvent( std::istream& in ){
    BaseMaceService* serviceInstance = BaseMaceService::getInstance( sid );
    mace::Message* ptr;
    int count = serviceInstance->deserializeMethod( in, ptr );
    helper = InternalMessageHelperPtr( static_cast< InternalMessageHelper* >( ptr ) );

    return count;
}
int mace::InternalMessage::deserializeUpcall( std::istream& in ){
    BaseMaceService* serviceInstance = BaseMaceService::getInstance( sid );
    mace::Message* ptr;
    int count = serviceInstance->deserializeMethod( in, ptr );
    helper = InternalMessageHelperPtr( static_cast< InternalMessageHelper* >( ptr ) );

    return count;
}
int mace::InternalMessage::deserializeRoutine( std::istream& in ){
    BaseMaceService* serviceInstance = BaseMaceService::getInstance( sid );
    mace::Message* ptr;
    int count = serviceInstance->deserializeMethod( in, ptr );
    helper = InternalMessageHelperPtr( static_cast< InternalMessageHelper* >( ptr ) );

    return count;
}

void mace::TransferContext_Message::serialize(std::string& str) const {
    if (!serializedCache.empty()) {
        str.append(serializedCache);
        return;
    }
    
    size_t initsize = str.size();
    mace::serialize(str, ctxParams);
    mace::serialize(str, &checkpoint);
    mace::serialize(str, &eventId);
           
    bool __false = false;
    mace::serialize(str, &__false );
      
    if (initsize == 0) {
        serializedCache = str;
    }
    serializedByteSize = str.size() - initsize;
}

int mace::TransferContext_Message::deserialize(std::istream& __mace_in) throw (mace::SerializationException) { 
    serializedByteSize = 0;
    
    ctxParams = new ContextBaseClassParams();

    serializedByteSize +=  mace::deserialize(__mace_in, ctxParams);
    serializedByteSize +=  mace::deserialize(__mace_in, &checkpoint);
    serializedByteSize +=  mace::deserialize(__mace_in, &eventId);
            
    bool __unused;
    serializedByteSize += mace::deserialize( __mace_in, &__unused );
      
    return serializedByteSize;
}

mace::TransferContext_Message::~TransferContext_Message() {
    if( ctxParams != NULL){ 
        delete ctxParams;
        ctxParams = NULL;
    }
}
