#include "ContextService.h"
#include "ScopedContextRPC.h"
#include "ReadLine.h"
#include "AccessLine.h"
#include "params.h"
#include "HeadEventDispatch.h"
#include "ContextLock.h"

using mace::ReadLine;

mace::vector< mace::string > mace::__CheckMethod__::nullNames;

std::map< uint64_t, std::set< pthread_cond_t* > > ContextService::contextWaitingThreads;
std::map< mace::string, std::set< pthread_cond_t* > > ContextService::contextWaitingThreads2;
std::map< mace::OrderID, std::set< pthread_cond_t* > > ContextService::contextWaitingThreads3;
std::map< uint64_t, std::set< pthread_cond_t* > > ContextService::contextMappingUpdateWaitingThreads;
std::map< uint64_t, std::vector< PthreadCondPointer > > ContextService::contextStructureUpdateWaitingThreads;
std::map< uint64_t, pthread_cond_t* > ContextService::externalMsgWaitingThread;

pthread_mutex_t ContextService::waitExitMutex = PTHREAD_MUTEX_INITIALIZER;;
pthread_cond_t ContextService::waitExitCond = PTHREAD_COND_INITIALIZER;;

/****************************** class RemoteContextRuntimeInfo ****************************************************************/
uint32_t RemoteContextRuntimeInfo::getConnectionStrength( mace::MaceAddr const& addr ) {
  if( connectionStrengths.find(addr) != connectionStrengths.end() ){
    return connectionStrengths[addr];
  } else {
    return 0;
  }
}

/****************************** class ContextService ****************************************************************/
void ContextService::acquireContextLocks(uint32_t const  targetContextID, mace::vector<uint32_t> const & snapshotContextIDs) const {
    mace::map< MaceAddr, mace::vector< uint32_t > > ancestorContextNodes;
    acquireContextLocksCommon(targetContextID, snapshotContextIDs, ancestorContextNodes );
    
    for( mace::map< MaceAddr, mace::vector< uint32_t > >::iterator nodeIt = ancestorContextNodes.begin(); nodeIt != ancestorContextNodes.end(); nodeIt ++ ){
      mace::InternalMessage msg( mace::enter_context , ThreadStructure::myEvent(), nodeIt->second );

      sender->sendInternalMessage( nodeIt->first, msg );
    }
}
void ContextService::acquireContextLocksCommon(uint32_t const targetContextID, mace::vector<uint32_t> const& snapshotContextIDs, mace::map< MaceAddr, mace::vector< uint32_t > >& ancestorContextNodes) const{
  ADD_SELECTORS("ContextService::acquireContextLocksCommon");
}
void ContextService::doDeleteContext( mace::string const& contextName  ){
  ADD_SELECTORS("ContextService::doDeleteContext");
  mace::Event& newEvent = ThreadStructure::myEvent();
  // must be head
  ASSERT( isLocal( this->contextMapping.getHead() ) );
  newEvent.newEventID( mace::Event::DELETECONTEXT );
  newEvent.initialize(  );
  mace::AgentLock lock( mace::AgentLock::WRITE_MODE ); // global lock is used to ensure new events are created in order

  const mace::ContextMapping* snapshotContext = & ( contextMapping.getSnapshot( newEvent.getLastContextMappingVersion() ) );
  uint32_t contextID = mace::ContextMapping::hasContext2( *snapshotContext, contextName );
  if( contextID == 0 ){ // context not found
    maceerr<<"Context '"<< contextName <<"' does not exist. Ignore delete request"<< Log::endl;
    lock.downgrade( mace::AgentLock::READ_MODE );
    HeadEventDispatch::HeadEventTP::commitEvent( newEvent ); // commit
    return;
  }

  //mace::Event::setLastContextMappingVersion( newEvent.eventId );
  //newEvent.eventContextMappingVersion = newEvent.eventID;

  mace::MaceAddr removeMappingReturn = contextMapping.removeMapping( contextName );
  const mace::ContextMapping* ctxmapCopy =  contextMapping.snapshot( newEvent.eventContextMappingVersion ) ; // create ctxmap snapshot
  ASSERT( ctxmapCopy != NULL );

  contextEventRecord.deleteContextEntry( contextName, contextID, newEvent.eventId );
  //newEvent.setSkipID( instanceUniqueID, contextID, newEvent.eventID );
  //mace::Event::EventSkipRecordType & skipIDStorage = newEvent.getPreEventIdsStorage( instanceUniqueID );
  //skipIDStorage.set( contextID, newEvent.eventId );

  BaseMaceService::globalNotifyNewContext( newEvent, instanceUniqueID );

  send__event_RemoveContextObject( newEvent.eventId, *ctxmapCopy, removeMappingReturn, contextID );
  BaseMaceService::globalNotifyNewEvent( newEvent, instanceUniqueID );

  lock.downgrade( mace::AgentLock::READ_MODE ); // downgrade to read mode to allow later events to enter.
}
void ContextService::handle__event_RemoveContextObject( mace::OrderID const eventID, mace::ContextMapping const& ctxmapCopy, 
      MaceAddr const& dest, uint32_t const& contextID ){
    ThreadStructure::setEventID( eventID );
    ThreadStructure::myEvent().eventType = mace::Event::DELETECONTEXT;
    mace::ContextBaseClass *thisContext = getContextObjByID( contextID ); 
    // make sure previous events have released the context.
    mace::ContextLock ctxlock( *thisContext, ThreadStructure::myEventID(), false, mace::ContextLock::WRITE_MODE );
    ctxlock.downgrade( mace::ContextLock::NONE_MODE );
    eraseContextData( thisContext );// erase the context from this node.

    // TODO: commit the delete context event
}
void ContextService::copyContextData(mace::ContextBaseClass* thisContext, mace::string& s ) const{
    mace::serialize(s, thisContext );
}
void ContextService::eraseContextData(mace::ContextBaseClass* thisContext){
    ADD_SELECTORS("ContextService::eraseContextData");
    uint32_t contextID = thisContext->getID();
    mace::string contextName = thisContext->getName();
    // (2) remove the context object from ctxobjIDMap & ctxobjNameMap
    ScopedLock sl(getContextObjectMutex);
    macedbg(1)<<"Erase context object '" << contextName << "'"<<Log::endl;
    // (1) erase the context object
    delete thisContext;

    ASSERT( contextID < ctxobjIDMap.size() && ctxobjIDMap[ contextID ] != NULL );
    ctxobjIDMap[ contextID ] = NULL;


    mace::hash_map< mace::string, mace::ContextBaseClass*, mace::SoftState >::const_iterator cpIt2 = ctxobjNameMap.find( contextName );
    ASSERT( cpIt2 != ctxobjNameMap.end() );
    ctxobjNameMap.erase( cpIt2 );
}

void ContextService::handleInternalMessages( mace::InternalMessage const& message, MaceAddr const& src, uint64_t msg_size ){
  ADD_SELECTORS("ContextService::handleInternalMessages");
  // macedbg(1) << "Message: " << message << Log::endl;
  switch( message.getMessageType() ){
    case mace::InternalMessage::UNKNOWN: break;
    case mace::InternalMessage::ALLOCATE_CONTEXT_OBJECT: {
      mace::AllocateContextObject_Message* m = static_cast< mace::AllocateContextObject_Message* >( message.getHelper() );
      handle__event_AllocateContextObject( src, m->destNode, m->contextId, m->eventId, m->contextMapping, m->eventType, m->version, 
        m->ownershipPairs, m->vers );
      break;
    }
		case mace::InternalMessage::ALLOCATE_CONTEXT_OBJ_RESPONSE: {
			mace::AllocateContextObjectResponse_Message* m = static_cast<mace::AllocateContextObjectResponse_Message*>( message.getHelper() );
			handle__event_AllocateContextObjectResponse(m->ctx_name, m->eventId, m->isCreateContextEvent );
			break;
		}
		case mace::InternalMessage::ALLOCATE_CONTEXT_OBJ_REQ: {
      mace::AllocateContextObjReq_Message* m = static_cast<mace::AllocateContextObjReq_Message* >( message.getHelper() );
			handle__event_AllocateContextObjReq( m->createContextName, m->eventId, src, m->ownershipPairs, m->vers );
      message.unlinkHelper();
			break;
		}
		case mace::InternalMessage::UPDATE_CONTEXT_MAPPING: {
			mace::UpdateContextMapping_Message* m = static_cast<mace::UpdateContextMapping_Message*>(message.getHelper());
			handle__event_UpdateContextMapping( m->ctxMapping, m->ctxName );
			break;
		}
    case mace::InternalMessage::COMMIT_DONE: {
      mace::CommitDone_Message* m = static_cast<mace::CommitDone_Message*>(message.getHelper());
      macedbg(1) << "Received a commit_done control message for " << message.getTargetContextName() << Log::endl;
      mace::ContextBaseClass* ctxObj = getContextObjByName(message.getTargetContextName());
      ASSERT( ctxObj != NULL );
      if( handleMessageToMigratingAndNullContext(ctxObj, message, src) ) {
        handle__event_CommitDone( m->target_ctx_name, m->eventId, m->coaccessContexts );
        ctxObj->decreaseHandlingMessageNumber();
      }
      break;
    }
    case mace::InternalMessage::CONTEXT_OWNERSHIP_CONTROL: {
      mace::ContextOwnershipControl_Message* m = static_cast<mace::ContextOwnershipControl_Message*>( message.getHelper());
      // macedbg(1) << "It is an ownership_control message: " << (uint16_t)m->type <<  Log::endl;
      if( m->type == mace::ContextOwnershipControl_Message::OWNERSHIP_MODIFY ) {
        const mace::vector<mace::EventOperationInfo>& eops = m->ownershipOpInfos;
        const mace::EventOperationInfo& eop = eops[0];
        mace::vector<mace::EventOperationInfo> ownershipOpInfos;
        for( uint32_t i=1; i<eops.size(); i++ ){
          ownershipOpInfos.push_back( eops[i] );
        }

        mace::ContextBaseClass* ctxObj = getContextObjByName(message.getTargetContextName());
        if( handleMessageToMigratingAndNullContext(ctxObj, message, src) ) {
          handle__ownership_modifyOwnership( m->dest_contextName, eop, m->src_contextName, ownershipOpInfos );
          ctxObj->decreaseHandlingMessageNumber();
        }
      } else if( m->type == mace::ContextOwnershipControl_Message::OWNERSHIP_UPDATE ) {
        handle__ownership_updateOwnership( m->ownerships, m->versions);
      } else if( m->type == mace::ContextOwnershipControl_Message::OWNERSHIP_UPDATE_REQUEST ) {
        handle__ownership_contextDAGRequest( src, m->src_contextName, m->contextSet );
      } else if( m->type == mace::ContextOwnershipControl_Message::OWNERSHIP_UPDATE_REPLY ) { 
        handle__ownership_contextDAGReply( m->dest_contextName, m->contextSet, m->ownerships, m->versions );
      } else if( m->type == mace::ContextOwnershipControl_Message::OWNERSHIP_MODIFY_DONE ) {
        if( m->dest_contextName == ""){
          //this->contextStructure.releaseLock(ContextStructure::WRITER);
        } else {
          mace::ContextBaseClass* ctxObj = getContextObjByName(m->dest_contextName);
          if( ctxObj == NULL ) {
            macedbg(1) << "Fail to find Context("<< m->dest_contextName <<")!!!" << Log::endl;
            ASSERT(false);
          }
          ctxObj->handleOwnershipOperationsReply(m->ownershipOpInfos[0]);
        }
      } else if( m->type == mace::ContextOwnershipControl_Message::OWNERSHIP_UPDATE_DONE ) {
        handle__ownership_updateReply( m->dest_contextName, m->contextSet, m->ownershipOpInfos );
      } else if( m->type == mace::ContextOwnershipControl_Message::OWNERSHIP_AND_DOMINATOR_UPDATE) {
        handle__ownership_updateDominators( m->ownerships, m->versions, m->src_contextName, m->contextSet, m->ownershipOpInfos );
      } else {
        ASSERT(false);
      }
      break;
    }
    case mace::InternalMessage::CONTEXTMAPPING_UPDATE_SUGGEST: {
      mace::ContextMappingUpdateSuggest_Message* m = static_cast<mace::ContextMappingUpdateSuggest_Message*>(message.getHelper() );
      handle__event_contextmapping_update_suggest(m->expectVersion);
      break;
    }
    case mace::InternalMessage::EXTERNALCOMM_CONTROL:{
      mace::ExternalCommControl_Message* m = static_cast< mace::ExternalCommControl_Message*>( message.getHelper() );
      handle__event_ExternalCommControl(m);
      break;
    }
    case mace::InternalMessage::CONTEXT_MIGRATION_REQUEST:{
      mace::ContextMigrationRequest_Message* m = static_cast< mace::ContextMigrationRequest_Message* >( message.getHelper() );
      handle__event_ContextMigrationRequest( src, m->dest, m->event, m->prevContextMapVersion, m->migrateContextIds, m->contextMapping );
      break;
    }
    case mace::InternalMessage::TRANSFER_CONTEXT:{
      mace::TransferContext_Message* m = static_cast< mace::TransferContext_Message* >( message.getHelper() );
      handle__event_TransferContext( src, m->ctxParams, m->checkpoint, m->eventId);
      break;
    }
    case mace::InternalMessage::MIGRATION_CONTROL: {
      mace::MigrationControl_Message* m = static_cast< mace::MigrationControl_Message* >(message.getHelper() );
      handle__event_MigrationControl( m );
      break;
    }
    case mace::InternalMessage::CREATE:{
      mace::create_Message* m = static_cast< mace::create_Message* >( message.getHelper() );
      handle__event_create( src, m->extra, m->counter, m->create_ctxId );
      break;
    }
    case mace::InternalMessage::CREATE_HEAD:{
      mace::create_head_Message* m = static_cast< mace::create_head_Message* >( message.getHelper() );
      handle__event_create_head( m->extra, m->counter, m->src );
      break;
    }
    case mace::InternalMessage::CREATE_RESPONSE:{
      mace::create_response_Message* m = static_cast< mace::create_response_Message* >( message.getHelper() );
      handle__event_create_response( m->event , m->counter , m->targetAddress);
      break;
    }
    case mace::InternalMessage::EXIT_COMMITTED:{
      handle__event_exit_committed(  );
      break;
    }
    case mace::InternalMessage::ENTER_CONTEXT:{
      mace::enter_context_Message* m = static_cast< mace::enter_context_Message* >( message.getHelper() );
      handle__event_enter_context( m->event, m->contextIDs );
      break;
    }
    case mace::InternalMessage::COMMIT:{
      mace::commit_Message* m = static_cast< mace::commit_Message* >( message.getHelper() );
      macedbg(1) << "Received a commit message for " << message.getTargetContextName() << Log::endl;
      if( message.getTargetContextName() == "" ) {
        mace::Event event = m->event;
        handle__event_commit( event, message, src );
        return;
      }
      mace::ContextBaseClass* ctxObj = getContextObjByName(message.getTargetContextName());
      if( handleMessageToMigratingAndNullContext(ctxObj, message, src) ) {
        mace::Event event = m->event;
        handle__event_commit( event, message, src );
        ctxObj->decreaseHandlingMessageNumber();
      }  
      break;
    }
    case mace::InternalMessage::SNAPSHOT:{
      mace::snapshot_Message* m = static_cast< mace::snapshot_Message* >( message.getHelper() );
      handle__event_snapshot( m->event , m->ctxID , m->snapshotContextID , m->snapshot);
      break;
    }
    case mace::InternalMessage::DOWNGRADE_CONTEXT:{
      mace::downgrade_context_Message* m = static_cast< mace::downgrade_context_Message* >( message.getHelper() );
      handle__event_downgrade_context( m->contextID , m->eventID , m->isresponse);
      break;
    }
    case mace::InternalMessage::EVICT:{
      handle__event_evict( src );
      break;
    }
    case mace::InternalMessage::MIGRATE_CONTEXT:{
      mace::migrate_context_Message* m = static_cast< mace::migrate_context_Message* >( message.getHelper() );
      handle__event_migrate_context( m->newNode, m->contextName, m->delay );
      break;
    }
    case mace::InternalMessage::MIGRATE_PARAM:{
      mace::migrate_param_Message* m = static_cast< mace::migrate_param_Message* >( message.getHelper() );
      handle__event_migrate_param( m->paramid  );
      break;
    }
    case mace::InternalMessage::REMOVE_CONTEXT_OBJECT:{
      mace::RemoveContextObject_Message* m = static_cast< mace::RemoveContextObject_Message* >( message.getHelper() );
      handle__event_RemoveContextObject( m->eventID , m->ctxmapCopy , m->dest , m->contextID);
      break;
    }
    case mace::InternalMessage::DELETE_CONTEXT:{
     mace::delete_context_Message* m = static_cast< mace::delete_context_Message* >( message.getHelper() );
     handle__event_delete_context( m->contextName  );
      break;
    }
    case mace::InternalMessage::NEW_HEAD_READY:{
      handle__event_new_head_ready( src  );
      break;
    }
    case mace::InternalMessage::ROUTINE_RETURN:{
      mace::routine_return_Message* m = static_cast< mace::routine_return_Message* >( message.getHelper() );
      handle__event_routine_return( m->returnValue, m->event  );
      break;
    }
    case mace::InternalMessage::ASYNC_EVENT:{
      mace::AsyncEvent_Message* h = static_cast< mace::AsyncEvent_Message*>( message.getHelper() );
      macedbg(1) << "Received an async message for " << message.getTargetContextName() << Log::endl;
      mace::ContextBaseClass* ctxObj = getContextObjByName(message.getTargetContextName());

      if( handleMessageToMigratingAndNullContext(ctxObj, message, src) ) {
        handleEventMessage( h, message, src, 0, msg_size );
        message.unlinkHelper();
        ctxObj->decreaseHandlingMessageNumber();
      }
      break;
    }
    case mace::InternalMessage::APPUPCALL:{
      mace::ApplicationUpcall_Message* m = static_cast< mace::ApplicationUpcall_Message* >( message.getHelper() );
      processRPCApplicationUpcall( m, src );
      break;
    }
    case mace::InternalMessage::APPUPCALL_RETURN:{
      mace::appupcall_return_Message* m = static_cast< mace::appupcall_return_Message* >( message.getHelper() );
      handle__event_appupcall_return( m->returnValue, m->event  );
      break;
    }
    case mace::InternalMessage::ROUTINE:{
      mace::Routine_Message* m = static_cast< mace::Routine_Message* >( message.getHelper() );

      mace::ContextBaseClass* ctxObj = getContextObjByName(message.getTargetContextName());
      if( handleMessageToMigratingAndNullContext(ctxObj, message, src) ) {
        handleRoutineMessage( m, src, message );
        message.unlinkHelper();
        ctxObj->decreaseHandlingMessageNumber();
      }
      break;
    }
    case mace::InternalMessage::TRANSITION_CALL:{
     mace::Routine_Message* m = static_cast< mace::Routine_Message* >( message.getHelper() );
      handleRoutineMessage( m, src, message );
      break;
    }
    case mace::InternalMessage::NEW_EVENT_REQUEST:{
      mace::AsyncEvent_Message* h = static_cast< mace::AsyncEvent_Message*>( message.getHelper() );
      addTimerEvent( h );
      message.unlinkHelper();
      break;
    }
    case mace::InternalMessage::CONTEXTMAPPING_UPDATE_REQ : {
      mace::ContextMappingUpdateReq_Message* m = static_cast<mace::ContextMappingUpdateReq_Message*>(message.getHelper());
      handle__event_contextmapping_update_req(m->expectVersion, src);
      break;
    }
    case mace::InternalMessage::EVENT_EXECUTION_CONTROL: {
      mace::EventExecutionControl_Message* m = static_cast<mace::EventExecutionControl_Message*>( message.getHelper() );
      if( m->control_type == mace::EventExecutionControl_Message::COMMIT_CONTEXTS ) {
        handle__event_commit_contexts(m->contextNames, m->src_contextName, m->eventId, src, src);
      } else if( m->control_type == mace::EventExecutionControl_Message::COMMIT_CONTEXT ) {
        mace::vector<mace::string> ctxNames;
        ctxNames.push_back(m->dest_contextName);
        handle__event_commit_contexts(ctxNames, m->src_contextName, m->eventId, message.getOrigAddr(), src);
      } else if( m->control_type == mace::EventExecutionControl_Message::TO_LOCK_CONTEXT) {
        const mace::EventOperationInfo& eventOpInfo = (m->opInfos)[0];
        const mace::string& dominator = contextStructure.getUpperBoundContextName( m->src_contextName );
        
        mace::ContextBaseClass* ctxObj = getContextObjByName(dominator);
        if( handleMessageToMigratingAndNullContext(ctxObj, message, src) ) {
          mace::set<mace::string> permittedContextNames;
          if( ctxObj->checkEventExecutePermission(this, eventOpInfo, false, permittedContextNames) ){
            ASSERT( permittedContextNames.size() > 0 );
            mace::vector<mace::EventOperationInfo> eventOpInfos;
            mace::vector<mace::string> contextNames;

            eventOpInfos.push_back(eventOpInfo);
            for( mace::set<mace::string>::iterator iter = permittedContextNames.begin(); iter != permittedContextNames.end(); iter++ ){
              contextNames.push_back(*iter);
            }
            send__event_replyEventExecutePermission( m->src_contextName, dominator, eventOpInfo.eventId, contextNames, eventOpInfos );
          }
          ctxObj->decreaseHandlingMessageNumber();
        } else {
          ASSERTMSG(false, "This should not happen!!!");
        }
      } else if( m->control_type == mace::EventExecutionControl_Message::ENQUEUE_LOCK_CONTEXTS) {
        ASSERT(false); 
      } else if( m->control_type == mace::EventExecutionControl_Message::LOCK_CONTEXT_PERMISSION) {
        mace::ContextBaseClass* ctxObj = getContextObjByName(m->dest_contextName);
        if( handleMessageToMigratingAndNullContext(ctxObj, message, src) ) {
          ctxObj->handleEventExecutePermission(this, m->opInfos, m->contextNames);
          ctxObj->decreaseHandlingMessageNumber();
        }
      } else if( m->control_type == mace::EventExecutionControl_Message::RELEASE_CONTEXTS) {
        const mace::vector<mace::string>& contextNames = m->contextNames;
        const mace::vector<mace::EventOperationInfo>& local_lock_requests = m->opInfos;
        ASSERT( contextNames.size() > 0 );
        mace::vector<mace::string> locked_contexts;
        mace::string release_context;
        for( uint32_t i=0; i<contextNames.size(); i++ ){
          if( i == 0 ){
            release_context = contextNames[i];
          } else {
            locked_contexts.push_back( contextNames[i] );
          }
        }
        const mace::string dominatorContext = contextStructure.getUpperBoundContextName( release_context );
        mace::ContextBaseClass* ctxObj = getContextObjByName(dominatorContext);

        if( handleMessageToMigratingAndNullContext(ctxObj, message, src) ) {
          ctxObj->releaseContext(this, m->eventId, release_context, m->src_contextName, local_lock_requests, locked_contexts);
          ctxObj->decreaseHandlingMessageNumber();
        }
      } else if( m->control_type == mace::EventExecutionControl_Message::UNLOCK_CONTEXT ){
        ASSERT( (m->opInfos).size() >=1 );
        const mace::string& destContext = m->dest_contextName;
        
        mace::ContextBaseClass* ctxObj = getContextObjByName(destContext);
        if( handleMessageToMigratingAndNullContext(ctxObj, message, src) ) {
          const mace::EventOperationInfo& eventOpInfo = (m->opInfos)[0];
          const mace::vector<mace::EventOperationInfo>& eops = m->opInfos;
          mace::vector<mace::EventOperationInfo> local_lock_requests;
          
          for( mace::vector<mace::EventOperationInfo>::const_iterator iter=eops.begin(); iter!=eops.end(); 
              iter++ ){
            if( iter == eops.begin() ){
              continue;
            }
            local_lock_requests.push_back(*iter);
          }

          ctxObj->unlockContext( this, eventOpInfo, local_lock_requests, m->contextNames );
          ctxObj->decreaseHandlingMessageNumber();
        }

      }else if( m->control_type == mace::EventExecutionControl_Message::RELEASE_CONTEXT_REPLY) {  
        ASSERTMSG(false, "No EventExecutionControl_Message::RELEASE_CONTEXT_REPLY any more!");
      }else if( m->control_type == mace::EventExecutionControl_Message::NEW_EVENT_OP) {
        ASSERTMSG(false, "No EventExecutionControl_Message::NEW_EVENT_OP any more!");
      } else if( m->control_type == mace::EventExecutionControl_Message::EVENT_OP_DONE ) {
        ASSERTMSG(false, "No EventExecutionControl_Message::EVENT_OP_DONE any more!");
      } else if( m->control_type == mace::EventExecutionControl_Message::CREATE_NEW_CONTEXT) {
        this->handle__event_NewContextCreation( m->src_contextName, m->contextNames[0], m->opInfos[0]);
      } else if( m->control_type == mace::EventExecutionControl_Message::CREATE_NEW_CONTEXT_REPLY) {
        mace::ContextBaseClass* ctxObj = getContextObjByName(m->dest_contextName);
        ASSERT( ctxObj != NULL );
        if( ctxObj != NULL ) {
          ctxObj->handleCreateNewContextReply( m->opInfos[0], m->newContextId);
        }
      } else if( m->control_type == mace::EventExecutionControl_Message::CHECK_COMMIT ) {
        mace::ContextBaseClass* ctxObj = getContextObjByName(m->dest_contextName);
        ASSERT( ctxObj != NULL );
        if( handleMessageToMigratingAndNullContext(ctxObj, message, src) ) {
          ctxObj->getReadyToCommit( this, m->eventId );
          ctxObj->decreaseHandlingMessageNumber();
        }
      } else if( m->control_type == mace::EventExecutionControl_Message::RELEASE_LOCK_ON_CONTEXT ) {
        mace::ContextBaseClass* ctxObj = getContextObjByName(m->dest_contextName);
        ASSERT( ctxObj != NULL );
        if( handleMessageToMigratingAndNullContext(ctxObj, message, src) ) {
          ctxObj->releaseLock(this, m->eventId);
          ctxObj->decreaseHandlingMessageNumber();
        }
      } else if( m->control_type == mace::EventExecutionControl_Message::READY_TO_COMMIT ) {
        const mace::vector<mace::string>& contextNames = m->contextNames;

        bool flag = true;
        mace::vector<mace::string> notifyContexts;
        mace::vector<mace::string> executedContexts;
        for( uint32_t i=0; i<contextNames.size(); i++ ){
          if( contextNames[i] == "#" ){
            flag = false;
            continue;
          }

          if( flag ){
            notifyContexts.push_back( contextNames[i] );
          } else {
            executedContexts.push_back( contextNames[i] );
          }
        }

        for( uint32_t i=0; i<notifyContexts.size(); i++ ){
          const mace::string& notifyContext = notifyContexts[i];

          mace::vector<mace::string> ctxNames;
          ctxNames.push_back(notifyContext);
          ctxNames.push_back("#");
          for( uint32_t i=0; i<executedContexts.size(); i++ ){
            ctxNames.push_back( executedContexts[i] );
          }
          
          mace::InternalMessageID msgId( Util::getMaceAddr(), notifyContext, 0);
          mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::READY_TO_COMMIT, 
            m->src_contextName, notifyContext, m->eventId, ctxNames, m->opInfos, m->newContextId);

          mace::ContextBaseClass* ctxObj = getContextObjByName(notifyContext);
          ASSERT( ctxObj != NULL );
          if( handleMessageToMigratingAndNullContext(ctxObj, msg, src) ) {
            ctxObj->notifyReadyToCommit(this, m->eventId, m->src_contextName, executedContexts );
            ctxObj->decreaseHandlingMessageNumber();
          }
        }
      } else if( m->control_type == mace::EventExecutionControl_Message::ENQUEUE_OWNERSHIP_OPS ) {
        mace::ContextBaseClass* ctxObj = getContextObjByName(m->dest_contextName);
        ASSERT( ctxObj != NULL );
        const mace::vector< mace::EventOperationInfo >& oops = m->opInfos;
        mace::vector< mace::EventOperationInfo >::const_iterator iter = oops.begin();
        const mace::EventOperationInfo& eop = *iter;

        iter ++;
        for( ; iter != oops.end(); iter++ ) {
          ctxObj->enqueueOwnershipOpInfo( m->eventId, *iter );
        }
        send__event_enqueueOwnershipOpsReply(eop, m->src_contextName, ctxObj->contextName);
      } else if( m->control_type == mace::EventExecutionControl_Message::ENQUEUE_OWNERSHIP_OPS_REPLY ) {
        mace::ContextBaseClass* ctxObj = getContextObjByName(m->dest_contextName);
        ASSERT( ctxObj != NULL && (m->opInfos).size() == 0 );
        const mace::EventOperationInfo& eop = (m->opInfos)[0];
        ctxObj->handleEnqueueOwnershipOpsReply(eop);
      }
      break;
    }
    case mace::InternalMessage::ENQUEUE_EVENT_REQUEST: {
      mace::EnqueueEventRequest_Message* m = static_cast<mace::EnqueueEventRequest_Message*>( message.getHelper() );
      mace::ContextBaseClass* ctxObj = getContextObjByName(m->dest_contextName);
      ASSERT( ctxObj != NULL );
      ctxObj->enqueueSubEvent(m->eventOpInfo.eventId, m->request);
      send__event_enqueueSubEventReply(m->eventOpInfo, m->src_contextName, ctxObj->contextName);
      break;
    }
    case mace::InternalMessage::ENQUEUE_EVENT_REPLY: {
      mace::EnqueueEventReply_Message* m = static_cast<mace::EnqueueEventReply_Message*>( message.getHelper() );
      mace::ContextBaseClass* ctxObj = getContextObjByName(m->dest_contextName);
      ASSERT( ctxObj != NULL );
      ctxObj->handleEnqueueSubEventReply(m->eventOpInfo);
      break;
    }
    case mace::InternalMessage::ENQUEUE_MESSAGE_REQUEST: {
      mace::EnqueueMessageRequest_Message* m = static_cast<mace::EnqueueMessageRequest_Message*>( message.getHelper() );
      mace::ContextBaseClass* ctxObj = getContextObjByName(m->dest_contextName);
      ASSERT( ctxObj != NULL );
      ctxObj->enqueueExternalMessage(m->eventOpInfo.eventId, m->msg);
      send__event_enqueueExternalMessageReply(m->eventOpInfo, m->src_contextName, ctxObj->contextName);
      break;
    }
    case mace::InternalMessage::ENQUEUE_MESSAGE_REPLY: {
      mace::EnqueueEventReply_Message* m = static_cast<mace::EnqueueEventReply_Message*>( message.getHelper() );
      mace::ContextBaseClass* ctxObj = getContextObjByName(m->dest_contextName);
      ASSERT( ctxObj != NULL );
      ctxObj->handleEnqueueExternalMessageReply(m->eventOpInfo);
      break;
    }
    case mace::InternalMessage::ELASTICITY_CONTROL: {
      mace::ElasticityControl_Message* m = static_cast<mace::ElasticityControl_Message*>( message.getHelper() );
      if( m->controlType == mace::ElasticityControl_Message::CONTEXT_NODE_STRENGTH_REQ) {
        mace::ContextBaseClass* ctxObj = getContextObjByName(m->contextName);
        ASSERT( ctxObj != NULL );
        const mace::ContextMapping& snapshot = this->getLatestContextMapping();
        uint64_t value = (ctxObj->runtimeInfo).getConnectionStrength( snapshot, m->addr );
        this->send__elasticity_ContextNodeStrengthReply( src, m->handlerThreadId, m->contextName, m->addr, value );
      } else if(m->controlType == mace::ElasticityControl_Message::CONTEXT_NODE_STRENGTH_REP) {
        wakeupHandlerThreadForContextStrength( m->handlerThreadId, m->contextName, m->addr, m->value );
      } else if( m->controlType == mace::ElasticityControl_Message::CONTEXTS_COLOCATE_REQ ){
        const mace::ContextMapping& snapshot = this->getLatestContextMapping();
        ASSERTMSG( mace::ContextMapping::getHead(snapshot) == Util::getMaceAddr(), 
          "ElasticityControl_Message::CONTEXTS_COLOCATE_REQ could only forwarded to the eMonitor!" );

        HeadEventDispatch::HeadEventTP::enqueueElasticityContextsColocationRequest( const_cast<ContextService*>(this), m->contextName, m->contextNames, 
          this->instanceUniqueID);
      } else if( m->controlType == mace::ElasticityControl_Message::SERVER_INFO_REPORT ) { 
        handleServerInformationReport( src, m->serversInfo); 
      } else if( m->controlType == mace::ElasticityControl_Message::SERVER_INFO_REQ ) {
        // mace::map< mace::MaceAddr, uint64_t > servers_cpu_usage = elasticityMonitor->getServersUintCPUUsage();
        // this->send__elasticity_serverInfoReply( src, m->handlerThreadId, servers_cpu_usage );
      } else if(m->controlType == mace::ElasticityControl_Message::SERVER_INFO_REPLY) {
        elasticityMonitor->setScaleOpType( m->value, m->addr );
        wakeupHandlerThreadForServersInfo( m->handlerThreadId, m->serversInfo );
      } else if(m->controlType == mace::ElasticityControl_Message::CONTEXTS_MIGRATION_QUERY){
        elasticityMonitor->enqueueContextMigrationQuery( src, m->migrationContextsInfo );
      } else if(m->controlType == mace::ElasticityControl_Message::CONTEXTS_MIGRATION_QUERY_REPLY){
        elasticityMonitor->processContextsMigrationQueryReply( src, m->contextNames );
      } else if(m->controlType == mace::ElasticityControl_Message::INACTIVATE_CONTEXT){
        contextMapping.inactivateContext(m->contextName);
      } else if(m->controlType == mace::ElasticityControl_Message::ACTIVATE_CONTEXT){
        contextMapping.activateContext(m->contextName);
      } else {
        ASSERTMSG(false, "Invalid controlType of ElasticityControl_Message!");
      }
      break;
    }
    case mace::InternalMessage::COMMIT_CONTEXT_MIGRATION: {
      mace::commit_context_migration_Message* m = static_cast<mace::commit_context_migration_Message*>( message.getHelper() );
      // update ContextMapping after migration
      contextMapping.updateMapping( m->destAddr, m->contextName );

      const mace::ContextMapping& snapshot = this->getLatestContextMapping();
      const uint32_t contextId = mace::ContextMapping::hasContext2( snapshot, m->contextName );

      mace::map<uint32_t, mace::string> migrate_contexts;
      migrate_contexts[contextId] = m->contextName;

      send__ContextMigration_UpdateContextMapping( m->destAddr, migrate_contexts, snapshot, m->srcAddr );
      break;
    }

    //default: throw(InvalidMaceKeyException("Deserializing bad internal message type "+boost::lexical_cast<std::string>(msgType)+"!"));
    
  }
}

// void ContextService::wakeupContextStructureUpdateThreads() const {
//   ADD_SELECTORS("wakeupContextStructureUpdateThreads");

//   macedbg(1) << "To wakeup threads waiting for (or less) version=" << ver << Log::endl;
//     std::vector<uint64_t> toDelete;
    
//     ScopedLock sl(contextStructureUpdateMutex);
//     std::map< uint64_t, std::vector<PthreadCondPointer> >::iterator condSetIt = contextStructureUpdateWaitingThreads.begin();
//     for(; condSetIt != contextStructureUpdateWaitingThreads.end(); condSetIt ++ ) {
//       if( condSetIt->first <= ver ) {
//         std::vector<PthreadCondPointer>::iterator setIter = condSetIt->second.begin();
//         for(; setIter != condSetIt->second.end(); setIter++) {
//           pthread_cond_signal( (*setIter).cond_ptr );
//         }
//         toDelete.push_back(condSetIt->first);
//       }
//     }

//     if( !toDelete.empty() ) {
//       for( uint32_t i=0; i<toDelete.size(); i++ ) {
//         contextStructureUpdateWaitingThreads.erase(toDelete[i]);
//       }
//     }
//   }

// void ContextService::getUpdatedContextStructure(const uint64_t expectVer) {
//     ADD_SELECTORS("ContextService::getUpdatedContextStructure");
//     const mace::ContextMapping& snapshot = getLatestContextMapping();
//     if( mace::ContextMapping::getHead(snapshot) == Util::getMaceAddr() ) {
//       return;
//     }

//     ScopedLock sl(contextStructureUpdateMutex);
//     //ScopedLock sl(contextStructure.contextStructureMutex);
//     send__event_contextStructureUpdateReqMsg(expectVer);
//     pthread_cond_t cond;
//     pthread_cond_init( &cond, NULL );
//     PthreadCondPointer p(&cond);

//     std::map<uint64_t, std::vector<PthreadCondPointer> >::iterator iter = contextStructureUpdateWaitingThreads.find(expectVer);
//     if( iter == contextStructureUpdateWaitingThreads.end() ) {
//       std::vector<PthreadCondPointer> pVec;
//       pVec.push_back(p);
//       contextStructureUpdateWaitingThreads[expectVer] = pVec;
//     } else {
//       iter->second.push_back(p);
//     }
    
//     macedbg(1) << "Wait for ContextStructure of " << expectVer << Log::endl;
//     pthread_cond_wait( &cond, &contextStructureUpdateMutex );
//     macedbg(1) << "Wakeup for ContextStructure of " << expectVer << Log::endl;
//     pthread_cond_destroy( &cond );
//   }




// Assuming events created from message delivery, or downcall transition can only take place at head node.
bool ContextService::handleEventMessage( mace::AsyncEvent_Message* m, mace::InternalMessage const& msg, mace::MaceAddr const& src, 
    uint32_t targetContextID, const uint64_t msg_size ){
  ADD_SELECTORS("ContextService::handleEventMessage");
  mace::Event& e = m->getEvent();
  mace::ContextBaseClass * contextObject = getContextObjByName( e.eventOpInfo.toContextName );
  ASSERT( contextObject != NULL );
    
  checkAndUpdateContextMapping(e.eventContextMappingVersion);
        
  macedbg(1)<<"Enqueue a message into context("<< contextObject->contextName <<") event queue: "<< m <<Log::endl;
  if( contextInfoCollectFlag > 0 ){
    (contextObject->runtimeInfo).addEventMessageSize( e, msg_size );
  }
  contextObject->enqueueEvent(this,m );   
  return true;
 }

void ContextService::handleRoutineMessage( mace::Routine_Message* m, mace::MaceAddr const& source, mace::InternalMessage const& message ){
    ADD_SELECTORS("ContextService::handleRoutineMessage");
    
    //macedbg(1) << "Current Target Context = " << m->getEvent().curTarget_ctx_name << Log::endl;
  mace::ContextBaseClass * contextObject = getContextObjByName( m->getEvent().eventOpInfo.toContextName );
    
  contextObject->enqueueRoutine(this, m, source ); 
}

void ContextService::handle__event_AllocateContextObject( MaceAddr const& src, MaceAddr const& destNode, 
      mace::map< uint32_t, mace::string > const& ContextID, mace::OrderID const& eventID, mace::ContextMapping const& ctxMapping, 
      int8_t const& eventType, const uint64_t& current_version, 
      const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, 
      const mace::map<mace::string, uint64_t>& vers){

    ADD_SELECTORS("ContextService::handle__event_AllocateContextObject");
    bool isCreateContextEvent = false;
    bool migrationFlag = false;
    bool contextMappingUpdateFlag = true;
    if(eventType == mace::Event::ALLOCATE_CTX_OBJECT) {
      macedbg(1) << "It's an allocateObj event for " << ContextID << Log::endl;
      isCreateContextEvent = true;
      migrationFlag = false;
      contextMappingUpdateFlag = true;
    } else {
      isCreateContextEvent = false;
      migrationFlag = true;
      contextMappingUpdateFlag = false;
    }
    
    mace::Event currentEvent( eventID );
    ThreadStructure::setEvent( currentEvent );

    ThreadStructure::setEventContextMappingVersion(current_version);

    MaceAddr headAddr = this->contextMapping.getHead();
    if( contextMappingUpdateFlag && this->contextMapping.getCurrentVersion() < ctxMapping.getCurrentVersion() ) {
      this->contextMapping.updateToNewMapping(ctxMapping);
    }
    
    contextStructure.getLock(ContextStructure::WRITER);
    contextStructure.updateOwnerships( ownershipPairs, vers );
    contextStructure.releaseLock(ContextStructure::WRITER);

    if( isLocal( destNode ) && destNode != headAddr ){ 
      // if the context is at the head node, asyncHead() creates the context already
      for( mace::map< uint32_t, mace::string >::const_iterator ctxIt = ContextID.begin(); ctxIt != ContextID.end(); ctxIt++ ){
        mace::ContextBaseClass *thisContext = createContextObjectWrapper( eventID, ctxIt->second, ctxIt->first, current_version, migrationFlag ); // create context object
        ASSERTMSG( thisContext != NULL, "createContextObjectWrapper() returned NULL!");

        // const mace::vector<mace::string> dominateContexts = contextStructure.getDominateContexts(thisContext->contextName);
        // mace::string dom_ctx_name = contextStructure.getUpperBoundContextName(thisContext->contextName);
        // uint64_t ver = contextStructure.getDAGNodeVersion(dom_ctx_name);
        // ASSERT(ver > 0);
        // thisContext->initializeDominator( thisContext->contextName, dom_ctx_name, ver, dominateContexts);

        macedbg(1) << "Create context(" << ctxIt->second <<") successfully!" << Log::endl;
        send__event_AllocateContextObjectResponseMsg(ctxIt->second, eventID, isCreateContextEvent);
      }
    }
		
}

void ContextService::handle__event_AllocateContextObjReq( mace::string const& ctxName, mace::OrderID const& eventId, 
    const mace::MaceAddr& src, const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, 
    const mace::map< mace::string, uint64_t>& vers ){
	
  ADD_SELECTORS("ContextService::handle__event_AllocateContextObjReq");
  //macedbg(1) << "Create context object: " << ctxName <<Log::endl;
  HeadEventDispatch::HeadEventTP::executeContextCreateEvent(this, ctxName, eventId, src, ownershipPairs, vers);	
}

void ContextService::handle__event_AllocateContextObjectResponse(mace::string const& ctx_name, mace::OrderID const& eventId, bool const& isCreateContextEvent){
  this->notifyContextMappingUpdate(ctx_name);
  if( isCreateContextEvent ) {
    HeadEventDispatch::HeadEventTP::commitGlobalEvent(eventId.ticket);
  }
}

void ContextService::handle__event_UpdateContextMapping(mace::ContextMapping const& ctxMapping, mace::string const& ctxName){
	ADD_SELECTORS("ContextService::handle__event_UpdateContextMapping");
  if(ctxMapping.getCurrentVersion() > contextMapping.getCurrentVersion()){
		contextMapping.updateToNewMapping(ctxMapping);
  }

  wakeupContextMappingUpdateThreads(contextMapping.getCurrentVersion());

  if( !ctxName.empty() ) {
    macedbg(1) << "Try to wakeup threads waiting for Context " << ctxName << Log::endl;
    wakeupWaitingThreads(ctxName);
  }
}

void ContextService::handle__event_contextmapping_update_suggest( const uint64_t ver ) {
  if( ver > contextMapping.getCurrentVersion() && ver > contextMapping.getExpectVersion() ) {
    contextMapping.setExpectVersion(ver);
    //getUpdatedContextMapping(ver);
  }
}



void ContextService::handle__ownership_modifyOwnership( const mace::string& dominator, const mace::EventOperationInfo& eop, 
    const mace::string& ctxName, const mace::vector<mace::EventOperationInfo>& ownershipOpInfos){
  ADD_SELECTORS("ContextService::handle__event_modifyOwnership");
  // const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
  
  // for( uint32_t i=0; i<ownershipOpInfos.size(); i++ ){
  //   const mace::EventOperationInfo& op_info = ownershipOpInfos[i];
  //   if( op_info.opType == mace::EventOperationInfo::ADD_OWNERSHIP_OP && op_info.fromContextName != "globalContext" ){
  //     const mace::vector<mace::ElasticityRule>& rules = eConfig.getRulesByContextType( Util::extractContextType(op_info.toContextName) );
  //     for( uint32_t i=0; i<rules.size(); i++ ){
  //       if( rules[i].includeReferenceCondition() ) {
  //         mace::ElasticityCondition cond = rules[i].getReferenceCondition();
  //         if( (cond.contextTypes.count(op_info.toContextName) > 0 && cond.contextTypes.count(op_info.fromContextName) > 0 ) 
  //             || cond.contextTypes.count("ANY") > 0 ) {
  //           if( rules[i].behavior.behaviorType == mace::ElasticityBehavior::COLOCATE ){
  //             contextMapping.colocateContexts( op_info.fromContextName, op_info.toContextName );
  //           } else if( rules[i].behavior.behaviorType == mace::ElasticityBehavior::SEPARATE ) {
  //             contextMapping.separateContexts( op_info.fromContextName, op_info.toContextName );
  //           }
  //         }
  //       }
  //     } 
  //   }
  // }

  // HeadEventDispatch::HeadEventTP::executeModifyOwnershipEvent(this, eop, ctxName, ownershipOpInfos);


  mace::ContextBaseClass* ctxObj = this->getContextObjByName(dominator);
  ASSERT(ctxObj != NULL);
  ContextService* sv = const_cast<ContextService*>(this);
  ctxObj->handleOwnershipOperations(sv, eop, ctxName, ownershipOpInfos);
}



void ContextService::handle__ownership_updateOwnership( const mace::vector< mace::pair<mace::string, mace::string> >& ownerships, 
  const mace::map< mace::string, uint64_t>& vers ) {
  ADD_SELECTORS("ContextService::handle__ownership_updateOwnership");
  this->contextStructure.getLock(ContextStructure::WRITER);
  contextStructure.updateOwnerships(ownerships, vers);
  this->contextStructure.releaseLock(ContextStructure::WRITER);
  // wakeupContextStructureUpdateThreads();
}

void ContextService::handle__ownership_contextDAGRequest( const mace::MaceAddr& srcAddr, const mace::string& src_context, 
    const mace::set<mace::string>& contexts ) const {
  ADD_SELECTORS("ContextService::handle__ownership_contextDAGRequest");
  macedbg(1) << "Require latest DAGs for " << contexts << Log::endl;

  mace::vector< mace::pair<mace::string, mace::string> > ownerships = contextStructure.getOwnershipOfContexts(contexts);
  mace::map<mace::string, uint64_t> vers;
  for( uint64_t i=0; i<ownerships.size(); i++ ) {
    mace::pair<mace::string, mace::string>& ownership = ownerships[i];
    uint64_t ver = contextStructure.getDAGNodeVersion(ownership.first);
    vers[ownership.first] = ver;
    
    ver = contextStructure.getDAGNodeVersion(ownership.second);
    vers[ownership.second] = ver;
  }

  this->send__ownership_contextDAGReply( srcAddr, src_context, contexts, ownerships, vers );
}

void ContextService::handle__ownership_contextDAGReply( const mace::string& dom_context, const mace::set<mace::string>& contexts, 
    const mace::vector<mace::pair<mace::string, mace::string > >& ownershipPairs, const mace::map<mace::string, uint64_t>& vers ) {
  ADD_SELECTORS("ContextService::handle__ownership_contextDAGReply");
  mace::ContextBaseClass* ctxObj = this->getContextObjByName(dom_context);
  ASSERT(ctxObj != NULL);
  ctxObj->handleContextDAGReply( this, contexts, ownershipPairs, vers );
}


void ContextService::handle__ownership_updateDominators( const mace::vector< mace::pair<mace::string, mace::string> >& ownerships, 
    const mace::map<mace::string, uint64_t>& vers, const mace::string& src_context, const mace::set<mace::string>& update_doms, 
    const mace::vector<mace::EventOperationInfo>& eops ) {
  ADD_SELECTORS("ContextService::handle__ownership_updateOwnershipAndDominators");
  
  this->contextStructure.getLock( ContextStructure::WRITER );
  this->contextStructure.updateOwnerships( ownerships, vers );
  this->contextStructure.releaseLock( ContextStructure::WRITER );
  
  mace::vector<mace::EventOperationInfo> reply_eops;
  for( mace::set<mace::string>::const_iterator cIter=update_doms.begin(); cIter!=update_doms.end(); cIter++ ){
    const mace::string& ctxName = *cIter;
    mace::ContextBaseClass* ctxObj = this->getContextObjByName( ctxName );
    ASSERT( ctxObj != NULL );
    mace::vector<mace::EventOperationInfo> r_eops = ctxObj->updateDominator(this, src_context, eops);
    for( uint32_t i=0; i<r_eops.size(); i++ ) {
      reply_eops.push_back(r_eops[i]);
    }
    // ctxObj->updateDominator(this, src_context, eops);
  }
  this->send__ownership_updateOwnershipReply( src_context, update_doms, reply_eops );
}

void ContextService::handle__ownership_updateReply( const mace::string& dest_contextName, 
    const mace::set<mace::string>& src_contextNames, const mace::vector<mace::EventOperationInfo>& eops ) {
  mace::ContextBaseClass* ctxObj = this->getContextObjByName( dest_contextName );
  ASSERT( ctxObj != NULL );
  ctxObj->handleOwnershipUpdateDominatorReply(this, src_contextNames, eops);
}

void ContextService::handle__event_ContextMigrationRequest( MaceAddr const& src, MaceAddr const& dest, mace::Event const& event, 
    uint64_t const& prevContextMapVersion, mace::set<uint32_t> const& migrateContextIds, mace::ContextMapping const& ctxMapping ){
  
  ADD_SELECTORS("ContextService::handle__event_ContextMigrationRequest");
  ThreadStructure::setEvent( event );
  //ThreadStructure::ScopedServiceInstance si( instanceUniqueID );
    
  ThreadStructure::setEventContextMappingVersion( prevContextMapVersion );
  //const mace::ContextMapping& ctxmapSnapshot = contextMapping.getLatestContextMapping() ;
  
  mace::set<uint32_t>::const_iterator mctxIter = migrateContextIds.begin();
  const mace::ContextMapping& ctxmapSnapshot = contextMapping.getLatestContextMapping();
  for(; mctxIter != migrateContextIds.end(); mctxIter++) {
    macedbg(1) << "ContextID = " << *mctxIter << Log::endl;
    mace::ContextBaseClass *thisContext = getContextObjByID( *mctxIter ); // assuming context object already exists and this operation does not create new object.
    macedbg(1) << "ContextID = " << *mctxIter << Log::endl;
    

    ASSERT(thisContext != NULL);
    // adding migrating context information
        
    ASSERT( thisContext->requireExecuteTicket(event.eventId) > 0 );
    thisContext->setMigratingFlag(true);
    mace::ContextLock ctxlock( *thisContext, event.eventId, false, mace::ContextLock::MIGRATION_MODE );
    macedbg(1) << "Migration Event("<< event.eventId <<") enter migrating Context("<< thisContext->contextName<<")" << Log::endl;
    this->addMigratingContextName( thisContext->getName() );
    macedbg(1) << "After add migration context names!" << Log::endl;
    mace::map<uint32_t, mace::string> migrate_contexts;
  
    send__event_MigrationControlMsg( mace::ContextMapping::getHead(ctxmapSnapshot), mace::MigrationControl_Message::MIGRATION_RELEASE_CONTEXTMAPPING,
        event.eventId.ticket, migrate_contexts, ctxMapping);
    send__event_MigrationControlMsg( dest, mace::MigrationControl_Message::MIGRATION_UPDATE_CONTEXTMAPPING,
        event.eventId.ticket, migrate_contexts, ctxMapping);

    macedbg(1) << "Prepare to halt!!" << Log::endl;
    thisContext->prepareHalt();

    macedbg(1) << "Now Migration Event("<< event.eventId <<") could migrate Context("<< thisContext->contextName<<")!" << Log::endl;
    mace::string contextData;
    copyContextData( thisContext, contextData );
    mace::ContextBaseClassParams* ctxParams = new mace::ContextBaseClassParams();
    ctxParams->initialize(thisContext);

    eraseContextData( thisContext );
    send__event_TransferContext( dest, ctxParams, contextData, event.getEventID() );
    
      // If the entire context subtree will be migrated, send message to child contexts
    // erase the context from this node.
  }
}

void ContextService::handle__event_create( MaceAddr const& src, __asyncExtraField const& extra, 
    uint64_t const& counter, uint32_t const& ctxID ){

  if( mace::Event::isExit ) {
    wasteTicket();
    return;
  }
  mace::InternalMessage m(mace::create_head, extra, counter, ctxID, src);
  //HeadEventDispatch::HeadEventTP::executeEvent( const_cast<ContextService*>(this), (HeadEventDispatch::eventfunc)&ContextService::handleInternalMessagesWrapper, new mace::InternalMessage(m), true );
  mace::ContextBaseClass* ctx_obj = getContextObjByID(ctxID);
  ctx_obj->enqueueCreateEvent(const_cast<ContextService*>(this), 
    (HeadEventDispatch::eventfunc)&ContextService::handleInternalMessagesWrapper, new mace::InternalMessage(m), true);
}
void ContextService::handle__event_commit( mace::Event& event, mace::InternalMessage const& message, mace::MaceAddr const& src) const {
  ADD_SELECTORS("ContextService::handle__event_commit");
  if( event.eventType == mace::Event::MIGRATIONEVENT ) {
    const mace::ContextMapping& ctxMapping = contextMapping.getLatestContextMapping();
    uint32_t ctxId = mace::ContextMapping::hasContext2(ctxMapping, event.target_ctx_name);
    HeadEventDispatch::HeadEventTP::commitMigrationEvent(this, event.eventId, ctxId);
    return;
  } else {
    ASSERT(false);
  }
}

void ContextService::handle__event_commit_contexts( mace::vector< mace::string > const& ctxNames, mace::string const& targetContextName,
      mace::OrderID const& eventId, mace::MaceAddr const& src, mace::MaceAddr const& orig_src ) {
    // recursively downgrade contexts until it reaches exceptionContextID or reaches the bottom of context lattice
    ADD_SELECTORS("ContextService::handle__event_commit_context");
    macedbg(1) << "ctxNames = "<<ctxNames<< " eventId = " << eventId << Log::endl;
    ASSERT( !ctxNames.empty() );

    ThreadStructure::ScopedServiceInstance si( instanceUniqueID );
    
    //const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
    for(  uint32_t i=0; i<ctxNames.size(); i++ ){
      const mace::string& thisContextName = ctxNames[i];
      mace::ContextBaseClass * thisContext = getContextObjByName( thisContextName);

      mace::InternalMessageID msgId( orig_src, thisContextName, 0);
      mace::vector<mace::string> contextNames; 
      mace::vector<mace::EventOperationInfo> opInfos; 
      uint32_t newContextId=0;
      mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::COMMIT_CONTEXT,  
        thisContextName, targetContextName, eventId, contextNames, opInfos, newContextId);
      
      if( handleMessageToMigratingAndNullContext(thisContext, msg, src) ){
        thisContext->commitContext(this, eventId);
        thisContext->decreaseHandlingMessageNumber();
      }
   }   
}

void ContextService::handle__event_create_response( mace::Event const& event, uint32_t const& counter, MaceAddr const& targetAddress){
  ADD_SELECTORS("ContextService::handle__event_create_response");
  // read from buffer
  
  ScopedLock sl( eventRequestBufferMutex );
  maceout<<"Event "<< event.eventId << ", counter = "<< counter <<" is sent to "<< targetAddress <<Log::endl;
  std::map< uint32_t, std::pair<mace::string*, mace::string > >::iterator ueIt = unfinishedEventRequest.find( counter );
  ASSERT( ueIt != unfinishedEventRequest.end() );
  std::pair< mace::string*, mace::string >& eventreq = ueIt->second;
  eventreq.first->erase(  eventreq.first->size() - eventreq.second.size() ); //remove the Event field from message
  /*__asyncExtraField extra;
  mace::deserialize( eventreq.second, &extra);*/
  /*extra.event = event;
  extra.isRequest = false;*/
  mace::string event_str;
  mace::serialize( event_str , &event );
  eventreq.first->append( event_str );

  mace::string* eventmsg = eventreq.first;

  unfinishedEventRequest.erase( ueIt );
  sl.unlock();

  const mace::MaceKey destNode( mace::ctxnode, targetAddress  );
  //sender->routeEventRequest( destNode, *eventmsg );

  delete eventmsg;

}

void ContextService::handle__event_enter_context( mace::Event const& event, mace::vector< uint32_t > const& contextIDs ){
  ThreadStructure::setEvent( event );
  for( mace::vector< uint32_t >::const_iterator ctxidIt = contextIDs.begin(); ctxidIt != contextIDs.end(); ctxidIt++ ){
    mace::ContextBaseClass * thisContext = getContextObjByID( *ctxidIt );
    mace::ContextLock __contextLock( *thisContext, event.eventId, false, mace::ContextLock::WRITE_MODE); // acquire context lock. 
  }

}
void ContextService::handle__event_exit_committed( ){
  // this message is supposed to be received by non-head nodes.
  // if the main thread is blocking in maceExit(), wake it up
  proceedExit();
  // if maceExit() has not been called at this node....?
}
void ContextService::handle__event_create_head(__asyncExtraField const& extra, uint64_t const& counter, 
    MaceAddr const& src){
  if( mace::Event::isExit ) {
    //mace::AgentLock::skipTicket();
    return;
  }

  //const mace::string& target_ctx_name = extra.targetContextID;
  //const mace::string& start_ctx_name = this->getStartContextName(target_ctx_name);
  // contextID;
  //asyncHead( ThreadStructure::myEvent(), extra, mace::Event::ASYNCEVENT, contextID );


  const MaceAddr& targetContextAddr = contextMapping.getNodeByContext( extra.targetContextID );
  send__event_create_response( src, ThreadStructure::myEvent(), counter, targetContextAddr );
}
void ContextService::handle__event_snapshot( mace::Event const& event, mace::string const& ctxID, 
    mace::string const& snapshotContextID, mace::string const& snapshot){
  // store the snapshoeventt
  pthread_mutex_lock(&mace::ContextBaseClass::eventSnapshotMutex );
  std::pair< mace::OrderID, mace::string > key( event.eventId, ctxID );
  std::map<mace::string, mace::string>& snapshots = mace::ContextBaseClass::eventSnapshotStorage[ key ];
  snapshots[ snapshotContextID ] = snapshot;
  // if the event is waiting in the target context, notify it.
  std::map<mace::OrderID, pthread_cond_t*>::iterator condIt = mace::ContextBaseClass::eventSnapshotConds.find( event.eventId );
  if( condIt !=  mace::ContextBaseClass::eventSnapshotConds.end() ){
      pthread_cond_signal( condIt->second );
  }
  pthread_mutex_unlock(&mace::ContextBaseClass::eventSnapshotMutex );
}
void ContextService::handle__event_downgrade_context( uint32_t const& contextID, mace::OrderID const& eventID, 
    bool const& isresponse ){
  if( isresponse ){
    mace::ScopedContextRPC::wakeup( eventID );
  }else{
    mace::Event currentEvent( eventID );
    ThreadStructure::setEvent( currentEvent );
    mace::ContextBaseClass *thisContext = getContextObjByID( contextID);
    mace::ContextLock cl( *thisContext, eventID, false, mace::ContextLock::READ_MODE );
  }
}
void ContextService::handle__event_routine_return( mace::string const& returnValue, mace::Event const& event){

  ThreadStructure::setEventContextMappingVersion ( event.eventContextMappingVersion );
  mace::ScopedContextRPC::wakeupWithValue( returnValue, event );
}
void ContextService::handle__event_appupcall_return( mace::string const& returnValue, mace::Event const& event){

  ThreadStructure::setEventContextMappingVersion ( event.eventContextMappingVersion );
  mace::ScopedContextRPC::wakeupWithValue( returnValue, event );
}
void ContextService::handle__event_evict( MaceAddr const& src ){
  
  // TODO: determine the contexts on the node
  mace::list< mace::string > contexts;
  // use the latest context mapping version
  contextMapping.getContextsOfNode( src, contexts );

  // TODO: call requestContextMigration() to migrate the contexts
  for( mace::list< mace::string >::iterator ctxIt = contexts.begin(); ctxIt != contexts.end(); ctxIt++ ){
    // app.getServiceObject()->requestContextMigration( serviceID, migctxIt->first, migctxIt->second, false );
    requestContextMigrationCommon( instanceUniqueID, *ctxIt,src, false );
  }

  // go to the lower services
}
/**
 * TODO: unfinished
 *
 * */
void ContextService::handle__event_new_head_ready( MaceAddr const& src ){
  

}
void ContextService::handle__event_migrate_context( mace::MaceAddr const& newNode, mace::string const& contextName, uint64_t const delay ){

}
#include "StrUtil.h"
void ContextService::handle__event_migrate_param( mace::string const& paramid ){
  // 1. split paramid into an array of parameter id
  StringList paramlist = StrUtil::split( " ", paramid);

  for (StringList::const_iterator i = paramlist.begin(); i != paramlist.end(); i++) {
    std::string const& param_id = *i;
    MaceAddr dest = MaceKey(ipv4, params::get<std::string>( param_id + ".dest" ) ).getMaceAddr();
    StringList contexts = StrUtil::split(" \n", params::get<mace::string>( param_id + ".contexts" ));
    uint8_t service = static_cast<uint8_t>(params::get<uint32_t>( param_id + ".service" ));
    ASSERT( service == instanceUniqueID );
    for( StringList::iterator ctxIt = contexts.begin(); ctxIt != contexts.end(); ctxIt ++ ){
      mace::string contextName = *ctxIt;
      std::cout << " migrate context "<< contextName <<" of service "<< service <<std::endl;
      requestContextMigrationCommon(service, contextName, dest , true);
    }
  }
}
void ContextService::handle__event_delete_context( mace::string const& contextName ){
  doDeleteContext( contextName );
}

void ContextService::handle__event_CommitDone( mace::string const& target_ctx_name, mace::OrderID const& eventId, 
    mace::set<mace::string> const& coaccess_contexts ) {
  ADD_SELECTORS("ContextService::handle__event_CommitDone");
  mace::ContextBaseClass* ctxObj = this->getContextObjByName(target_ctx_name);
  macedbg(1) << "To commit Event("<< eventId <<") in " << target_ctx_name << Log::endl;
  ctxObj->handleEventCommitDone(this, eventId, coaccess_contexts);
}

void ContextService::handle__event_contextmapping_update_req( const uint64_t& expectVer, const mace::MaceAddr& src) {
  ADD_SELECTORS("ContextService::handle__event_contextmapping_update_req");
  macedbg(1) << "Receive context mapping update request("<< expectVer <<") from " << src << Log::endl;
  mace::string ctxName = "";
  notifyContextMappingUpdate(ctxName, src);
}

void ContextService::handle__event_ExternalCommControl( const mace::ExternalCommControl_Message* msg) {
  if( msg->control_type == mace::ExternalCommControl_Message::ADD_CLIENT_MAPPING) {
    ScopedLock sl(externalCommMutex);
    if( externalCommClassMap.find(msg->externalCommId) == externalCommClassMap.end() ){
      ExternalCommClass* exCommClass = new ExternalCommClass(msg->externalCommContextId, msg->externalCommId);
      externalCommClassMap[msg->externalCommId] = exCommClass;
      externalCommClasses.push_back(msg->externalCommId);
    }
  }
}

void ContextService::handle__event_NewContextCreation( const mace::string& create_ctx_name, const mace::string& created_ctx_type, 
    const mace::EventOperationInfo& op_info) {
  uint32_t newContextId = 0;
  ScopedLock sl(serviceSharedDataMutex);
  mace::map<mace::string, uint32_t>::iterator iter = contextIds.find(created_ctx_type);
  if( iter != contextIds.end() ) {
    newContextId = iter->second ++;
  } else {
    contextIds[created_ctx_type] = 2;
    newContextId = 1;
  }
  sl.unlock();

  // mace::string new_ctx_name = Util::generateContextName(created_ctx_type, newContextId);

  // mace::ElasticityRule rule = elasticityConfig.getContextInitialPlacement( created_ctx_type );
  // mace::string colocate_ctx_name = "";
  // if( rule.ruleType != mace::ElasticityRule::NO_RULE ) {
  //   if( rule.includeCreateCondition() ) {
  //     mace::ElasticityCondition cond( mace::ElasticityCondition::CREATE, Util::extractContextType(create_ctx_name), created_ctx_type );

  //     mace::vector< mace::ElasticityCondition > conds;
  //     conds.push_back( cond );
      
  //     if( rule.satisfyConditions(conds) ) {
  //       if( rule.behavior.behaviorType == mace::ElasticityBehavior::COLOCATE ){
  //         contextMapping.colocateContexts( create_ctx_name, new_ctx_name );
  //       } else if( rule.behavior.behaviorType == mace::ElasticityBehavior::SEPARATE ) {
  //         contextMapping.separateContexts( create_ctx_name, new_ctx_name );
  //       }
  //     }
  //   } else if( rule.includeReferenceCondition() ) {
  //     mace::string ref_from_context = contextStructure.getParentContextName( new_ctx_name );
  //     if( ref_from_context != "" ){
  //       mace::ElasticityCondition cond( mace::ElasticityCondition::REFERENCE, Util::extractContextType(ref_from_context), created_ctx_type );

  //       mace::vector< mace::ElasticityCondition > conds;
  //       conds.push_back( cond );
      
  //       if( rule.satisfyConditions(conds) ) {
  //         if( rule.behavior.behaviorType == mace::ElasticityBehavior::COLOCATE ){
  //           contextMapping.colocateContexts( ref_from_context, new_ctx_name );
  //         } else if( rule.behavior.behaviorType == mace::ElasticityBehavior::SEPARATE ) {
  //           contextMapping.separateContexts( ref_from_context, new_ctx_name );
  //         }
  //       }
  //     }
  //   } else if( rule.behavior.behaviorType == mace::ElasticityBehavior::ISOLATE ){
  //     contextMapping.isolateContext( new_ctx_name );
  //   } 
  // }
  
  send__event_createNewContextReply( create_ctx_name, op_info, newContextId);
}

/************************************* ContextService::handle__event *********************************************************/

mace::MaceAddr const& ContextService::asyncHead( mace::OrderID& eventId, mace::string const& targetContextName, 
    int8_t const eventType, uint32_t& contextId){
  ADD_SELECTORS("ContextService::asyncHead #2");
  if( eventType == mace::Event::UNDEFEVENT ){
    return SockUtil::NULL_MACEADDR;
  }

  const mace::ContextMapping& snapshotContext = contextMapping.getLatestContextMapping();
  contextId = mace::ContextMapping::hasContext2( snapshotContext, targetContextName );
  if( contextId > 0 ){ // the context exists
    return mace::ContextMapping::getNodeByContext( snapshotContext, contextId );    
  }else{// create a new context
    macedbg(1) << "Try to create context object(" << targetContextName << ")"<< Log::endl;
       
    mace::vector< mace::pair<mace::string, mace::string> > ownershipPairs;
    mace::map< mace::string, uint64_t> vers;
    trytoCreateNewContextObject( targetContextName, eventId, ownershipPairs, vers );
    
    const mace::ContextMapping& newSnapshotContext = contextMapping.getLatestContextMapping();
    contextId = mace::ContextMapping::hasContext2( newSnapshotContext, targetContextName );
    ASSERTMSG(contextId > 0, "Fail to create context!");
    return mace::ContextMapping::getNodeByContext( newSnapshotContext, contextId );
  }
}

mace::MaceAddr const& ContextService::asyncHead( const mace::OrderID& eventId, const mace::string& targetContextName, 
    const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, 
    const mace::map< mace::string, uint64_t>& vers ){
  ADD_SELECTORS("ContextService::asyncHead #3");
  
  const mace::ContextMapping& snapshotContext = this->getLatestContextMapping();
  uint32_t contextId = mace::ContextMapping::hasContext2( snapshotContext, targetContextName );
  if( contextId > 0 ){ // the context exists
    return mace::ContextMapping::getNodeByContext( snapshotContext, contextId );    
  }else{// create a new context
    trytoCreateNewContextObject( targetContextName, eventId, ownershipPairs, vers );
    const mace::ContextMapping& newSnapshotContext = contextMapping.getLatestContextMapping();
    contextId = mace::ContextMapping::hasContext2( newSnapshotContext, targetContextName );
    ASSERTMSG(contextId > 0, "Fail to create context!");
    return mace::ContextMapping::getNodeByContext( newSnapshotContext, contextId );
  }
}

mace::MaceAddr const& ContextService::asyncHead( mace::Message* msg, uint32_t& contextId){
  ADD_SELECTORS("ContextService::asyncHead #1");
  mace::AsyncEvent_Message* msgObj = static_cast<mace::AsyncEvent_Message*>( msg );
  ASSERT( msgObj != NULL );
  mace::Event& event = msgObj->getEvent();
  
  uint8_t eventType = event.eventType;
  if( eventType == mace::Event::UNDEFEVENT ){
    return SockUtil::NULL_MACEADDR;
  }
  event.eventContextMappingVersion = contextMapping.getCurrentVersion();
  const mace::ContextMapping& snapshotContext = contextMapping.getLatestContextMapping();
  contextId = mace::ContextMapping::hasContext2( snapshotContext, event.target_ctx_name );
  if( contextId > 0 ){ // the context exists
    send__event_asyncEvent( mace::ContextMapping::getNodeByContext( snapshotContext, contextId),  msgObj, event.target_ctx_name);
    return mace::ContextMapping::getNodeByContext( snapshotContext, contextId );
  }else{// create a new context
		mace::vector< mace::pair<mace::string, mace::string> > ownershipPairs;
    mace::map< mace::string, uint64_t> vers;
    trytoCreateNewContextObject( event.target_ctx_name, event.eventId, ownershipPairs, vers );
    macedbg(1) << "Wake up after creating context " << event.target_ctx_name << Log::endl;
    const mace::ContextMapping& newSnapshotContext = contextMapping.getLatestContextMapping();
    contextId = mace::ContextMapping::hasContext2( newSnapshotContext, event.target_ctx_name );
    ASSERTMSG(contextId > 0, "Fail to create context!");
    send__event_asyncEvent( mace::ContextMapping::getNodeByContext( newSnapshotContext, contextId),  msgObj, event.target_ctx_name);
    return mace::ContextMapping::getNodeByContext( newSnapshotContext, contextId );
  }
  
}

void ContextService::broadcastHead( mace::Message* msg ){
  ADD_SELECTORS("ContextService::broadcastHead");
  mace::AsyncEvent_Message* msgObj = static_cast<mace::AsyncEvent_Message*>( msg );

  mace::Event& event = msgObj->getEvent();
  
  uint8_t eventType = event.eventType;
  if( eventType == mace::Event::UNDEFEVENT ){
    return;
  }

  const mace::ContextMapping& snapshotContext = contextMapping.getLatestContextMapping();
  uint32_t contextId = mace::ContextMapping::hasContext2( snapshotContext, event.eventOpInfo.toContextName );
  if( contextId > 0 ){ // the context exists
    macedbg(1) << "Forward broadcast event: " << event.eventOpInfo << Log::endl;
    send__event_asyncEvent( mace::ContextMapping::getNodeByContext( snapshotContext, contextId),  msgObj, event.eventOpInfo.toContextName);
  }else{// create a new context
    mace::vector< mace::pair<mace::string, mace::string> > ownershipPairs;
    mace::map< mace::string, uint64_t> vers;
    trytoCreateNewContextObject( event.eventOpInfo.toContextName, event.eventId, ownershipPairs, vers );

    const mace::ContextMapping& newSnapshotContext = contextMapping.getLatestContextMapping();
    contextId = mace::ContextMapping::hasContext2( newSnapshotContext, event.eventOpInfo.toContextName );
    ASSERTMSG(contextId > 0, "Fail to create context!");
    macedbg(1) << "Forward broadcast event: " << event.eventOpInfo << Log::endl;
    send__event_asyncEvent( mace::ContextMapping::getNodeByContext( newSnapshotContext, contextId),  msgObj, event.eventOpInfo.toContextName);
    
  }
  //macedbg(1) << "After event("<< event.eventId <<") broadcast request is send out!" << Log::endl;
}

void ContextService::__beginTransition( const uint32_t targetContextID, mace::vector<uint32_t> const& snapshotContextIDs, bool isRelease = true, bool newExecuteTicket = false  ) const {
  ThreadStructure::pushServiceInstance( instanceUniqueID ); 
  __beginMethod( targetContextID, snapshotContextIDs, isRelease, newExecuteTicket );
}
void ContextService::__beginMethod( const uint32_t targetContextID, mace::vector<uint32_t> const& snapshotContextIDs, bool isRelease = true, bool newExecuteTicket = false ) const {
  ADD_SELECTORS("ContextService::__beginMethod");
  ThreadStructure::pushContext( targetContextID );
  ThreadStructure::insertEventContext( targetContextID );
  mace::ContextBaseClass * thisContext = getContextObjByID( targetContextID );
  ASSERT( thisContext != NULL );
  ThreadStructure::setMyContext( thisContext );

  if( newExecuteTicket ) {
    bool execute_flag = thisContext->waitingForExecution(this, ThreadStructure::myEvent());
    ASSERTMSG( execute_flag, "The first event shouldn't wait any previous event!");
    mace::EventExecutionInfo ee_info(thisContext->contextName, thisContext->contextName, mace::Event::EVENT_OP_OWNERSHIP);
    thisContext->setEventExecutionInfo(ThreadStructure::myEventID(), ee_info);
  }

  mace::Event& event = ThreadStructure::myEvent();
  if( contextInfoCollectFlag>0 ){
      (thisContext->runtimeInfo).runEvent(event);
  }
  macedbg(1) << "Try to execute "<< event.eventId<<" in " << thisContext->contextName << Log::endl;
  if( event.eventOpType == mace::Event::EVENT_OP_READ ) {
    mace::ContextLock __contextLock( *thisContext, ThreadStructure::myEventID(), isRelease, mace::ContextLock::READ_MODE);
  } else {
    mace::ContextLock __contextLock( *thisContext, ThreadStructure::myEventID(), isRelease, mace::ContextLock::WRITE_MODE); // acquire context lock.
  } 

  thisContext->setCurrentEventOp( event.eventOpInfo );
  uint64_t ver = (this->contextStructure).getDAGNodeVersion( thisContext->contextName );
  if( ver > 0 ){
    event.eventOpInfo.setContextDAGVersion(thisContext->contextName, ver);
  }
}

void ContextService::__beginCommitContext( const uint32_t targetContextID ) const {
  ADD_SELECTORS("ContextService::__beginCommitContext");
  ThreadStructure::pushContext( targetContextID );
  ThreadStructure::insertEventContext( targetContextID );
  mace::ContextBaseClass * thisContext = getContextObjByID( targetContextID );
  ThreadStructure::setMyContext( thisContext );

  mace::ContextLock __contextLock( *thisContext, ThreadStructure::myEventID(), true, mace::ContextLock::COMMIT_MODE);  
}
void ContextService::__finishBroadcastTransition() const {
  ADD_SELECTORS("ContextService::__finishBroadcastTransition");
  mace::Event& currentEvent = ThreadStructure::myEvent();
  ASSERT( currentEvent.eventType == mace::Event::BROADCASTEVENT);

  mace::ContextBaseClass* ctxObj = ThreadStructure::myContext();
  ASSERT( ctxObj != NULL);
  macedbg(1) << "Event("<< currentEvent.eventId <<") is executed in context("<< ctxObj->contextName <<")!" << Log::endl;
  if( contextInfoCollectFlag>0 ){
    (ctxObj->runtimeInfo).stopEvent(currentEvent.eventId);
  }

  mace::vector<mace::EventOperationInfo> ownershipOpInfos = ctxObj->extractOwnershipOpInfos(currentEvent.eventId);
  if( ownershipOpInfos.size() > 0 ) {
    mace::EventOperationInfo& eop = currentEvent.eventOpInfo;
    ctxObj->applyOwnershipOperations( this, eop, ownershipOpInfos);
    for( uint32_t i=0; i<ownershipOpInfos.size(); i++ ) {
      eop.newCreateContexts.erase( ownershipOpInfos[i].toContextName );
    }
  }
  // macedbg(1) << "Event("<< currentEvent.eventId <<") in context("<< ctxObj->contextName <<") has accessed: " << currentEvent.eventOpInfo.accessedContexts << Log::endl;
  ctxObj->handleEventOperationDone( this, currentEvent.eventOpInfo );
}

void ContextService::__finishTransition(mace::ContextBaseClass* oldContext) const {
  ADD_SELECTORS("ContextService::__finishTransition");
 
  mace::Event& currentEvent = ThreadStructure::myEvent();
  mace::ContextBaseClass * thisContext = ThreadStructure::myContext();
  macedbg(1) << "Event("<< currentEvent.eventId <<") finishes event transition in " << thisContext->contextName << Log::endl;

  if( contextInfoCollectFlag > 0 ){
    (thisContext->runtimeInfo).stopEvent(currentEvent.eventId);
  }

  mace::vector<mace::EventOperationInfo> ownershipOpInfos = thisContext->extractOwnershipOpInfos(currentEvent.eventId);
  if( ownershipOpInfos.size() > 0 ) {
    mace::EventOperationInfo& eop = currentEvent.eventOpInfo;
    thisContext->applyOwnershipOperations( this, eop, ownershipOpInfos);
    for( uint32_t i=0; i<ownershipOpInfos.size(); i++ ) {
      eop.newCreateContexts.erase( ownershipOpInfos[i].toContextName );
    }
  }
  
  ContextService* _nc_service = const_cast<ContextService*>(this);
  
  ScopedLock sl1(thisContext->contextMigratingMutex);
  mace::vector<mace::string> locked_contexts;
  mace::vector<mace::EventOperationInfo> local_lock_requests;
  thisContext->releaseContext(_nc_service, currentEvent.eventId, thisContext->contextName, thisContext->contextName, local_lock_requests, 
    locked_contexts);
  sl1.unlock();

  __finishMethod(oldContext);
  ThreadStructure::popServiceInstance( ); 
}

void ContextService::__finishMethod(mace::ContextBaseClass* oldContext) const {
  ADD_SELECTORS("ContextService::__finishMethod");
  macedbg(1) << "Pop current context!" << Log::endl;
  ThreadStructure::popContext( );
  ThreadStructure::setMyContext( oldContext );
}

void ContextService::__finishCommitContext(mace::ContextBaseClass* oldContext) const {
  ThreadStructure::popContext( );
  ThreadStructure::setMyContext( oldContext );
}
void ContextService::enterInnerService (mace::string const& targetContextID ) const{
  ADD_SELECTORS("ContextService::enterInnerService");
  macedbg(1) << "Enter ContextService::enterInnerService"<<Log::endl;
  
}
void ContextService::notifyNewEvent( mace::Event & he, const uint8_t serviceID ) {
    ADD_SELECTORS("ContextService::notifyNewEvent");

    if( serviceID == instanceUniqueID ) { return; }

    if( he.getEventType() == mace::Event::MIGRATIONEVENT ){
      // if it's a migration event and is not initiated in this service, don't update context event record
      // because the migration event will not enter this service at all.
      return;
    }

    // If the event is not created in this service, it is guaranteed the all the contexts in this service will be
    // explicitly downgraded by this event. So all later events entering this service should wait for this event
    // i.e. It is as if the event in this service starts from the global context
    //const mace::string globalContext = ""; 

    //contextEventRecord.updateContext( globalContext, he.eventId, he.getPreEventInfosStorage( instanceUniqueID ) );
}
void ContextService::notifyNewContext(mace::Event & he,  const uint8_t serviceID ) {
    ADD_SELECTORS("ContextService::notifyNewContext");

    if( serviceID == instanceUniqueID ) { return; }
    if( he.eventType == mace::Event::STARTEVENT ){
      const mace::string globalContextID = "";
      // if it's a start event, the head has to create a mapping to global context
      // to prevent race condition, the global context of every service in the composition has to be created explicitly in the first event (that is, maceInit)
      mace::ElasticityRule rule;
      mace::string pContextName = "";
      std::pair< mace::MaceAddr, uint32_t > newMappingReturn = contextMapping.newMapping( globalContextID, rule, pContextName );
      const mace::ContextMapping* ctxmapCopy =  contextMapping.snapshot( he.eventContextMappingVersion ) ; // create ctxmap snapshot
      ASSERT( ctxmapCopy != NULL );
      contextEventRecord.createContextEntry( globalContextID, newMappingReturn.second, he.eventId );
      //mace::Event::EventSkipRecordType & skipIDStorage = he.getPreEventIdsStorage( instanceUniqueID );
      //skipIDStorage.set( newMappingReturn.second, he.eventId );

      if( isLocal( newMappingReturn.first ) ){ // the new context co-locates with the head
        mace::ContextBaseClass *globalContext = createContextObjectWrapper( he.eventId, globalContextID, newMappingReturn.second, he.eventContextMappingVersion, false );
        ASSERT( globalContext != NULL );
        const mace::vector<mace::string> dominateContexts = contextStructure.getDominateContexts(globalContext->contextName);
        globalContext->initializeDominator(globalContext->contextName, globalContext->contextName, 1, dominateContexts);
      }else{
        remoteAllocateGlobalContext( globalContextID, newMappingReturn, ctxmapCopy );
      }
      return;
    }

    ASSERTMSG( !contextMapping.hasSnapshot( he.eventContextMappingVersion ), "The new context is not created in this service, why does it have this version of context mapping?" );
    const mace::ContextMapping& ctxmapCopy =  contextMapping.getLatestContextMapping() ; // create ctxmap snapshot
    //ASSERT( ctxmapCopy != NULL );

    mace::map< uint32_t, mace::string > contextSet; // empty set
    mace::vector< mace::pair<mace::string, mace::string> > ownershipPairs; 
    mace::map<mace::string, uint64_t> vers;
    send__event_AllocateContextObjectMsg( he.eventId, ctxmapCopy, SockUtil::NULL_MACEADDR, contextSet, 0, he.eventContextMappingVersion,
      ownershipPairs, vers );
}

void ContextService::downgradeEventContext( ){
  ADD_SELECTORS("ContextService::downgradeEventContext");
}

void ContextService::trytoCreateNewContextObject( mace::string const& ctx_name, mace::OrderID const& eventId, 
    const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, const mace::map< mace::string, uint64_t>& vers) {
  ADD_SELECTORS("ContextService::trytoCreateNewContextObject");
  ScopedLock sl_relase(releaseContextMappingUpdateMutex);
  ScopedLock sl(getContextObjectMutex);

  this->send__event_AllocateContextObjReqMsg( ctx_name, eventId, ownershipPairs, vers );
  waitForContextObjectCreate(ctx_name, sl_relase);
}

void ContextService::waitForContextObjectCreate( const mace::string& contextName, ScopedLock& release_lock ) {
  ADD_SELECTORS("ContextService::waitForContextObjectCreate");
  pthread_cond_t cond;
  pthread_cond_init( &cond, NULL );
  macedbg(1) << "Waiting for Context("<< contextName<<") creating!" << Log::endl;
  contextWaitingThreads2[ contextName ].insert( &cond );
  release_lock.unlock();
  pthread_cond_wait( &cond, &getContextObjectMutex );
  macedbg(1) << "Wakeup for Context " << contextName << Log::endl; 
  pthread_cond_destroy( &cond );
}

void ContextService::requestContextMigrationCommon(const uint8_t serviceID, const mace::string& contextName, const MaceAddr& destNode, 
    const bool rootOnly){
  ADD_SELECTORS("ContextService::requestContextMigrationCommon");
  ASSERTMSG( contextMapping.getHead() == Util::getMaceAddr(), "Context migration is requested, but this physical node is not head node." );
  macedbg(1) << "Migrating context " << contextName << Log::endl;
  HeadEventDispatch::HeadEventTP::executeContextMigrationEvent(const_cast<ContextService*>(this), serviceID, contextName, destNode, rootOnly );
}


void ContextService::handle__event_MigrateContext( void *p ){
  // This function must be executed on headnode
  ADD_SELECTORS("ContextService::handle__event_MigrateContext");


  mace::__event_MigrateContext *msg = static_cast< mace::__event_MigrateContext * >( p );

  ThreadStructure::setEventID( msg->eventId );
  const mace::string& contextName = msg->contextName;
  const MaceAddr& destNode = msg->destNode;
  const bool rootOnly = msg->rootOnly;

  ASSERTMSG( rootOnly, "Now it only support to migrate context one by one!");

  mace::Event& newEvent = ThreadStructure::myEvent( );
  newEvent.newEventID( mace::Event::MIGRATIONEVENT );
  
  // 3. Then initializes the migration event

  newEvent.initialize3(contextName, contextMapping.getCurrentVersion()  );
  newEvent.addServiceID(instanceUniqueID);
  // 4. Get the latest contextmap snapshot to determine the existence of context.
  //    If it doesn't exist, just store the context and the destination node as the default mapping
  //    so that when the context is created in the future, it will be created at that node.
  const ContextMapping& ctxmapSnapshot = contextMapping.getLatestContextMapping( );
  if( !contextMapping.hasContext( contextName ) ){
    maceerr<<"Requested context does not exist. Ignore it but set it as the default mapping when the context is created in the future."<<Log::endl;
    mace::map<mace::MaceAddr ,mace::list<mace::string > > servContext;
    servContext[ destNode ].push_back( contextName );
    contextMapping.loadMapping( servContext );
    HeadEventDispatch::HeadEventTP::commitGlobalEvent( newEvent.eventId.ticket ); // commit
    return;
  }
  const MaceAddr& origNode = mace::ContextMapping::getNodeByContext( ctxmapSnapshot, contextName );
  // ignore the requests that migrate a context to its original physical node
  if( origNode == destNode ){
    maceerr << "ContextService::handle__event_MigrateContext: origNode=" << origNode << " destNode=" <<destNode << "for context " << contextName << Log::endl;
    HeadEventDispatch::HeadEventTP::commitGlobalEvent( newEvent.eventId.ticket ); // commit
    return;
  }
  // 5. Ok. Let's roll.
  //    Create a new version of context map. Update the event skip id
  //mace::Event::setLastContextMappingVersion( newEvent.eventId );
  const uint64_t prevContextMappingVersion = newEvent.eventContextMappingVersion;
  
  mace::map< uint32_t, mace::string > offsprings;
  std::pair<bool, uint32_t>  updatedContext;
  mace::map< mace::MaceAddr, mace::set<mace::string> > origContextAddrs;
  mace::map< mace::MaceAddr, mace::set<uint32_t> > origContextIdAddrs;
  const mace::ContextMapping& ctxMapping = contextMapping.getLatestContextMapping();
  mace::map<mace::MaceAddr ,mace::list<mace::string > > servContext;
  if( rootOnly ){
    //updatedContext = contextMapping.updateMapping( destNode, contextName ); 
    const uint32_t ctxId = mace::ContextMapping::hasContext2(ctxmapSnapshot, contextName);
    offsprings[ ctxId ] =  contextName;
    origContextAddrs[origNode].insert(contextName);
    origContextIdAddrs[origNode].insert(ctxId);
  }else{ // TODO: also update the mapping of child & all offspring contexts.
    // right now: support migrating the entire context subtree only if they all reside on the same physical node.
    mace::set< mace::string > offspringContextNames = contextStructure.getAllDecendantContexts(contextName);
    contextMapping.updateMapping< mace::set<mace::string> >( destNode, offspringContextNames );
    
    mace::set< mace::string >::const_iterator cIter = offspringContextNames.begin();
    
    for(; cIter != offspringContextNames.end(); cIter ++) {
      uint32_t ctxId = mace::ContextMapping::hasContext2(ctxMapping, *cIter);
      if( ctxId > 0) {
        const mace::MaceAddr& orig_addr = mace::ContextMapping::getNodeByContext(ctxMapping, *cIter);
        if( orig_addr != destNode ) {
          offsprings[ctxId] = *cIter;
          origContextAddrs[orig_addr].insert(*cIter);
          origContextIdAddrs[orig_addr].insert(ctxId);
          //contextMapping.updateMapping( destNode, *cIter );
        }
      }else {
        servContext[ destNode ].push_back( *cIter );
        //contextMapping.loadMapping( servContext );
      }
    } 
  }

  // Step 5, to notify the new node to accept possible messages for migration contexts

  HeadEventDispatch::HeadEventTP::setMigratingContexts( this, newEvent.eventId, offsprings, origContextAddrs, destNode );
  
  contextMapping.setUpdateFlag(false);
  macedbg(1) << "Now available ContextMapping("<< contextMapping.getCurrentVersion() <<")!" << Log::endl;
  if( servContext.size() > 0 ) {
    contextMapping.loadMapping( servContext );
  }

  // update contextMapping of head node
  mace::map< uint32_t, mace::string >::const_iterator offIter =  offsprings.begin();
  for(; offIter != offsprings.end(); offIter++ ) {
    contextMapping.updateMapping( destNode, offIter->second );
  }
  


  newEvent.eventContextMappingVersion = contextMapping.getCurrentVersion();
  const mace::ContextMapping* ctxmapCopy = contextMapping.getCtxMapCopy( ); 
  
  //macedbg(1)<<" The new version "<< newEvent.eventContextMappingVersion << " context map: "<< ctxmapCopy << Log::endl;
  // Step 6: to create Context Objects on new nodes 
  if( isLocal( destNode ) ){
    for( mace::map< uint32_t, mace::string >::const_iterator osIt = offsprings.begin(); osIt != offsprings.end(); osIt++ ){
      mace::ContextBaseClass* thisContext = createContextObjectWrapper( newEvent.eventId, osIt->second  , osIt->first, contextMapping.getLatestMappingVersion(), true );
      const mace::vector<mace::string> dominateContexts = contextStructure.getDominateContexts(thisContext->contextName);
      mace::string dominator = contextStructure.getUpperBoundContextName(thisContext->contextName);
      uint64_t ver = contextStructure.getDAGNodeVersion(thisContext->contextName);
      ASSERT( ver > 0 );
      thisContext->initializeDominator( thisContext->contextName, dominator, ver, dominateContexts);
    }
  } else {
    mace::vector< mace::pair<mace::string, mace::string> > ownershipPairs; 
    mace::map<mace::string, uint64_t> vers;
    send__event_AllocateContextObjectMsg( newEvent.eventId, *ctxmapCopy, destNode, offsprings, mace::Event::MIGRATIONEVENT, 
      contextMapping.getLatestMappingVersion(), ownershipPairs, vers ); 
  }
  
  HeadEventDispatch::HeadEventTP::waitingForMigrationContextMappingUpdate(this, origContextIdAddrs, destNode, 
    ThreadStructure::myEvent(), prevContextMappingVersion, *ctxmapCopy, newEvent.eventId);
  HeadEventDispatch::HeadEventTP::deleteMigrationContext( contextName );
  // Step 7: it could only update ContextMapping when migration event is executed
  macedbg(1) << "Release the latest ContextMapping("<< contextMapping.getCurrentVersion() <<")!" << Log::endl;
  contextMapping.setUpdateFlag(true);
  
  delete ctxmapCopy;

  /*
  static bool recordFinishTime = params::get("EVENT_LIFE_TIME",false);
  if( recordFinishTime ){
    HeadEventDispatch::insertEventStartTime(newEvent.getEventID());
  }
  */

  delete msg;
}
void ContextService::sendAsyncSnapshot( __asyncExtraField const& extra, mace::string const& thisContextID, mace::ContextBaseClass* const& thisContext ){
  //ThreadStructure::myEvent().eventID = extra.event.eventID;
  mace::Event& myEvent = ThreadStructure::myEvent();
  mace::set<mace::string>::iterator snapshotIt = extra.snapshotContextIDs.find( thisContextID );
  if( snapshotIt != extra.snapshotContextIDs.end() ){
      mace::ContextLock ctxlock( *thisContext, ThreadStructure::myEventID(), false, mace::ContextLock::READ_MODE );// get read lock
      mace::string snapshot;// get snapshot
      mace::serialize(snapshot, thisContext );
      // send to the target context node.
      send__event_snapshot( contextMapping.getNodeByContext( extra.targetContextID ), myEvent,extra.targetContextID, *snapshotIt, snapshot );

      ctxlock.downgrade( mace::ContextLock::NONE_MODE );
  }else{
      mace::ContextLock ctxlock( *thisContext, ThreadStructure::myEventID(), false, mace::ContextLock::NONE_MODE );// get read lock
  }
}
// helper functions for maintaining context mapping
void ContextService::loadContextMapping(const mace::map<mace::MaceAddr ,mace::list<mace::string > >& servContext){
    contextMapping.setDefaultAddress ( Util::getMaceAddr() );
    contextMapping.loadMapping( servContext );
    contextMapping.snapshot( static_cast<uint64_t>( 0 ) );
}
void ContextService::downgradeContext( mace::string const& contextName ) {
  // TODO: 
  //(1) assert: the event has acquired the context before.
  const mace::ContextMapping& currentMapping = contextMapping.getSnapshot();
  const mace::Event::EventServiceContextType& eventContexts = ThreadStructure::getCurrentServiceEventContexts();
  const uint32_t contextID = currentMapping.findIDByName( contextName );
  ASSERTMSG( eventContexts.find( contextID ) != eventContexts.end(), "The event does not have the context" );   
  mace::AccessLine::checkDowngradeContext( instanceUniqueID, contextID, currentMapping );
  //(2) figure out the physical address of the context
  //(3) if it's local, call it. If not, send message and wait for response
  send__event_downgrade_context( mace::ContextMapping::getNodeByContext( currentMapping, contextID ), ThreadStructure::getCurrentContext(), ThreadStructure::myEvent().eventId, false );

  ThreadStructure::removeEventContext( ThreadStructure::getCurrentContext() );
}

void ContextService::migrateContext( mace::string const& paramid ){
  send__event_migrate_param( paramid );
}
void ContextService::deleteContext( mace::string const& contextName ){
  send__event_delete_context( contextName );
}
void ContextService::__beginRemoteMethod( mace::Event const& event) const {
  ADD_SELECTORS("ContextService::__beginRemoteMethod");
  macedbg(1) << "Event("<< event.eventId <<") with op("<< event.eventOpInfo <<") start to execute!" << Log::endl;
  ThreadStructure::setEvent( event );
}
void ContextService::__finishRemoteMethodReturn( mace::MaceAddr const& src, mace::string const& returnValueStr ) const{
  ADD_SELECTORS("ContextService::__finishRemoteMethodReturn")
  send__event_routine_return( src, returnValueStr );
}
void ContextService::__appUpcallReturn( mace::MaceKey const& src, mace::string const& returnValueStr ) const{
  send__event_routine_return( src.getMaceAddr(), returnValueStr );
}
void ContextService::nullEventHead( void *p ){
  mace::NullEventMessage* nullEventMessage = static_cast< mace::NullEventMessage* >( p );
  __asyncExtraField extra;
  //uint32_t contextID;
  //asyncHead( nullEventMessage->getEvent(), extra, mace::Event::UNDEFEVENT, contextID );
  HeadEventDispatch::HeadEventTP::commitEvent( nullEventMessage->getEvent() ); // commit

  delete nullEventMessage;
}
void ContextService::wasteTicket( void ) const{
  mace::NullEventMessage* nullEventMessage = new mace::NullEventMessage( ThreadStructure::myEventID() );
  HeadEventDispatch::HeadEventTP::executeEvent( const_cast<ContextService*>(this), (HeadEventDispatch::eventfunc)&ContextService::nullEventHead, nullEventMessage, true ); 
}
void ContextService::notifyHeadExit(){
  bool isOuterMostTransition = ( instanceUniqueID == instanceID.size()-1  )?true: false;
  if( isOuterMostTransition ){
    if( isLocal( mace::ContextMapping::getHead(contextMapping) ) ){
      //mace::Event& myEvent = ThreadStructure::myEvent();
      //HeadEventDispatch::HeadEventTP::commitEvent( myEvent );
      // wait to confirm the event is committed.
      // remind other physical nodes the exit event has committed.
      const mace::map< MaceAddr, uint32_t >& nodes = contextMapping.getAllNodes();
      for( mace::map< MaceAddr, uint32_t >::const_iterator nodeIt = nodes.begin(); nodeIt != nodes.end(); nodeIt ++ ){
        if( isLocal( nodeIt->first ) ) continue;
        mace::InternalMessage msg( mace::exit_committed );
        sender->sendInternalMessage( nodeIt->first, msg );
      }
    }else{
      // wait for exit event to commit.
      waitExit();
    }
  }
}
void ContextService::processRPCApplicationUpcall( mace::ApplicationUpcall_Message* msg, MaceAddr const& src){
  // make sure this is the head node.
  ASSERT( contextMapping.getHead() == Util::getMaceAddr() );
  ThreadStructure::ScopedContextID sci( mace::ContextMapping::HEAD_CONTEXT_ID );

  mace::OrderID commitEventId;
  commitEventId.ctxId = ThreadStructure::myEventID().ctxId;
  commitEventId.ticket = ThreadStructure::myEventID().ticket-1;

  mace::AgentLock::waitAfterCommit( commitEventId );
  // wait until this event becomes the next to commit
  //
  // set up the current event
  //ThreadStructure::setEvent( msg->getEvent() );
  //
  // execute unprocessed application upcalls (which do not have return value)
  // and clear upcalls in the event
  ThreadStructure::myEvent().executeApplicationUpcalls();
  //
  // return back ( return value and update event )
  mace::string returnValue;
  this->executeDeferredUpcall( msg, returnValue );
  mace::InternalMessage m( mace::appupcall_return, returnValue, ThreadStructure::myEvent() );
  sender->sendInternalMessage( src, m);
}
void ContextService::processLocalRPCApplicationUpcall( mace::ApplicationUpcall_Message* msg, mace::string& returnValue ){
  ASSERT( contextMapping.getHead() == Util::getMaceAddr() );
  ThreadStructure::ScopedContextID sci( mace::ContextMapping::HEAD_CONTEXT_ID );

  mace::OrderID commitEventId;
  commitEventId.ctxId = ThreadStructure::myEventID().ctxId;
  commitEventId.ticket = ThreadStructure::myEventID().ticket-1;
  mace::AgentLock::waitAfterCommit( commitEventId );
  // wait until this event becomes the next to commit
  //
  // set up the current event
  //ThreadStructure::setEvent( msg->getEvent() );
  //
  // execute unprocessed application upcalls (which do not have return value)
  // and clear upcalls in the event
  ThreadStructure::myEvent().executeApplicationUpcalls();
  //
  // return back ( return value and update event )
  this->executeDeferredUpcall( msg, returnValue );
}



void ContextService::addTransportEventRequest( mace::AsyncEvent_Message* reqObj, mace::MaceKey const& src){
  
  ADD_SELECTORS("ContextService::addTransportEventRequest");
  checkAndWaitExternalMessageHandle();
  ExternalCommClass* exCommClass = getExternalCommClass();
  // macedbg(1) << "Context("<< src <<")'s external communication context: " << commCtxName << Log::endl;
  
  if(reqObj->getExtra().targetContextID == "") {
    reqObj->getExtra().targetContextID = mace::ContextMapping::GLOBAL_CONTEXT_NAME;
  } 
  signalExternalMessageThread();
  
  mace::Event& event = reqObj->getEvent();
  exCommClass->createEvent(this, event, reqObj->getExtra().targetContextID, reqObj->getExtra().methodName,
    reqObj->getExtra().lockingType, instanceUniqueID);

  uint32_t contextId;
  asyncHead( reqObj, contextId );
}

void ContextService::snapshotContext( const mace::string& fileName ) const {
  mace::ContextBaseClass* thisContext = ThreadStructure::myContext();
  ASSERT( thisContext != NULL );

  mace::string contextSnapshot;
  copyContextData( thisContext, contextSnapshot );

  // thisContext->snapshot( contextSnapshot );

  std::ofstream writer(fileName.c_str());
  writer << contextSnapshot;
  writer.close();
}

void ContextService::addTimerEvent( mace::AsyncEvent_Message* reqObject){
  HeadEventDispatch::HeadEventTP::executeEvent(this,(HeadEventDispatch::eventfunc)&ContextService::createEvent, reqObject, false );
}
void ContextService::forwardHeadTransportThread( mace::MaceAddr const& dest, mace::AsyncEvent_Message* const eventObject ){
    HeadEventDispatch::HeadTransportTP::sendEvent( sender, dest, eventObject, instanceUniqueID );
}

bool ContextService::addNewCreateContext( mace::string const& ctx_name, const mace::MaceAddr& src) {
  ADD_SELECTORS("ContextService::addNewCreateContext");
  macedbg(1) << "Add new create context: " << ctx_name << Log::endl;
  ScopedLock sl(createNewContextMutex);
  mace::map< mace::string, mace::vector<mace::MaceAddr> >::iterator iter = processContextCreate.find(ctx_name);
  if( iter == processContextCreate.end() ) {
    macedbg(1) << "The context creation process is not started: " << ctx_name << Log::endl;
    mace::vector<mace::MaceAddr> addrs;
    addrs.push_back(src);
    processContextCreate[ctx_name] = addrs;
    return true;
  } else {
    iter->second.push_back(src);
    macedbg(1) << "The context creation process is already started: " << ctx_name << Log::endl;
    return false;
  }
}

bool ContextService::hasNewCreateContext( const mace::string& ctxName ) {
  ADD_SELECTORS("ContextService::hasNewCreateContext");
  macedbg(1) << "Check existence for " << ctxName << Log::endl;
  ScopedLock sl(createNewContextMutex);
  mace::map< mace::string, mace::vector<mace::MaceAddr> >::iterator iter = processContextCreate.find(ctxName);
  if( iter == processContextCreate.end() ) {
    macedbg(1) << "Exists! " << ctxName << Log::endl;
    return false;
  } else {
    macedbg(1) << "no Exists! " << ctxName << Log::endl;
    return true;
  }
}

void ContextService::notifyContextMappingUpdate(mace::string const& ctxName) {
  ADD_SELECTORS("ContextService::notifyContextMappingUpdate #1");
  ScopedLock sl(createNewContextMutex);

  mace::map< mace::string, mace::vector<mace::MaceAddr> >::iterator iter = processContextCreate.find(ctxName);
  if( iter == processContextCreate.end() ) {
    return;
  } else {
    mace::vector< mace::MaceAddr >& addrs = iter->second;
    const mace::ContextMapping& currentMapping = contextMapping;
    for( mace::vector<mace::MaceAddr>::const_iterator addr_iter = addrs.begin(); addr_iter != addrs.end(); addr_iter ++) {
      mace::InternalMessage msg(mace::UpdateContextMapping, currentMapping, ctxName);
      forwardInternalMessage( *addr_iter, msg);
    }
    macedbg(1) << "Notify waiting thread context(" << ctxName << ") has been created!" << Log::endl;
    processContextCreate.erase(iter);    
  }
}

void ContextService::notifyContextMappingUpdate(mace::string const& ctxName, const mace::MaceAddr& src) {
  const mace::ContextMapping& currentMapping = contextMapping.getLatestContextMapping();
  mace::InternalMessage msg(mace::UpdateContextMapping, currentMapping, ctxName);
  forwardInternalMessage( src, msg);
}


void ContextService::executeRoutineGrap( mace::Routine_Message* routineobject, mace::MaceAddr const& source ) {
  
}

void ContextService::executeCommitContext( mace::commit_single_context_Message* const msg ) {
  
}

void ContextService::executeBroadcastCommitContext( mace::commit_single_context_Message* const msg ) {
  
}

void ContextService::executeStartEvent( mace::AsyncEvent_Message* async_msg ) {
  ADD_SELECTORS("ContextService::executeStartEvent");
  ASSERTMSG(false, "In ContextService::executeStartEvent");
  
}

void ContextService::checkAndUpdateContextMapping(const uint64_t contextMappingVer) {
  ADD_SELECTORS("ContextService::checkAndUpdateContextMapping");
  ScopedLock sl(contextMappingUpdateMutex);
  if( contextMappingVer <= contextMapping.getCurrentVersion() || contextMappingUpdatingFlag ) {
    return;
  }
  macedbg(1) << "Current version=" << contextMapping.getCurrentVersion() << " expectVersion=" << contextMappingVer << Log::endl;
  contextMappingUpdatingFlag = true;
  sl.unlock();
  
  //getUpdatedContextMapping(contextMappingVer);
  getUpdatedContextMapping(0);
  macedbg(1) << "Get updated context mapping!" << Log::endl;
  sl.lock();
  contextMappingUpdatingFlag = false;
}

void ContextService::getUpdatedContextMapping(const uint64_t ver ) {
  ADD_SELECTORS("ContextService::getUpdatedContextMapping");
  macedbg(1) << "Try to get ContextMapping("<< ver <<")!" << Log::endl;
  ScopedLock sl_relase(releaseContextMappingUpdateMutex);
  ScopedLock sl(contextMappingUpdateMutex);
  send__event_contextMappingUpdateReqMsg(ver);
  waitForContextMappingUpdate(ver, sl);
  macedbg(1) << "Wakeup for ContextMapping("<< ver <<")!" << Log::endl;
}

void ContextService::checkAndUpdateContextStructure(const uint64_t contextStructureVer) {
  // ScopedLock sl(contextStructureUpdateMutex);
  // if( contextStructureVer <= contextStructure.getCurrentVersion() || contextStructureUpdatingFlag ) {
  //   return;
  // }
  // contextStructureUpdatingFlag = true;
  // sl.unlock();
  // getUpdatedContextStructure(0);

  // sl.lock();
  // contextStructureUpdatingFlag = false;
}

void ContextService::modifyOwnership( const uint8_t opType, mace::string const& parentContextName, mace::string const& childContextName ) {
  ADD_SELECTORS("ContextService::modifyOwnership");
  mace::ContextBaseClass* ctxObj = ThreadStructure::myContext();
  mace::Event& myEvent = ThreadStructure::myEvent();
  macedbg(1) << "Event("<< myEvent.eventId <<") modify ownership: type="<< (uint32_t)opType<<", p="<<parentContextName << ", c="<<childContextName<<Log::endl; 
  

  mace::EventOperationInfo opInfo(myEvent.eventId, opType, childContextName, parentContextName, 0, false);
  //myEvent.enqueueOwnershipOpInfo(opInfo);
  ctxObj->enqueueOwnershipOpInfo(myEvent.eventId, opInfo);
}



void ContextService::newBroadcastEventID() {
  /*
  ADD_SELECTORS("ContextService::newBroadcastEventID");
  mace::ContextBaseClass* ctxObj = ThreadStructure::myContext();
  mace::Event& event = ThreadStructure::myEvent();
  
  mace::OrderID oldBroadcastId = event.getBroadcastEventID();
  mace::OrderID newBroadcastId = event.newBroadcastEventID( ctxObj->getID() );
  event.setBroadcastEventID(newBroadcastId);
  event.setPreBroadcastEventID(oldBroadcastId);

  macedbg(1) << "oldBroadcastId=" << oldBroadcastId << " newBroadcastId=" << newBroadcastId << Log::endl;
  
  if( oldBroadcastId.ticket > 0 ) {
    downgradeBroadcastEvent(ctxObj->contextName, event.eventId, oldBroadcastId, contextStructure.getBroadcastCPRelations(ctxObj->contextName), 
      ctxObj->contextBroadcastEventsInfo.getBroadcastTargetContextNames(event.eventId, oldBroadcastId) );
  }
  */
}

void ContextService::downgradeBroadcastEvent( mace::string const& ctxName, mace::OrderID const& eventId, mace::OrderID const& bEventId, 
    mace::map<mace::string, mace::set<mace::string> > const& cpRelations, mace::set<mace::string> const& targetContextNames ) const {

  ADD_SELECTORS("ContextService::downgradeBroadcastEvent");
}

void ContextService::executeEventBroadcastRequest( mace::AsyncEvent_Message* reqObj ) {
  ADD_SELECTORS("ContextService::executeEventBroadcastRequest");

  mace::__asyncExtraField& extra = reqObj->getExtra();
  mace::Event& event = ThreadStructure::myEvent();
  mace::Event& reqEvent = reqObj->getEvent();
  ASSERT( !extra.targetContextID.empty() );
  mace::ContextBaseClass* ctxObj = ThreadStructure::myContext();
  ASSERT( ctxObj != NULL );

  if( contextInfoCollectFlag>0 ){
      (ctxObj->runtimeInfo).addCalleeContext( extra.targetContextID, extra.methodName );
  }
  macedbg(1) << "Event("<< event.eventId <<") make a broadcast call to context("<< extra.targetContextID <<") from context("<< ctxObj->contextName <<")!" << Log::endl;

  mace::vector<mace::EventOperationInfo> ownershipOpInfos = ctxObj->extractOwnershipOpInfos(event.eventId);
  if( ownershipOpInfos.size() > 0 ) {
    mace::EventOperationInfo& eop = event.eventOpInfo;
    ctxObj->applyOwnershipOperations( this, eop, ownershipOpInfos);
    for( uint32_t i=0; i<ownershipOpInfos.size(); i++ ) {
      eop.newCreateContexts.erase( ownershipOpInfos[i].toContextName );
    }
  }

  if( !contextStructure.checkParentChildRelation(ctxObj->contextName, extra.targetContextID) ) {
    maceerr << "Context("<< ctxObj->contextName <<") is not Context("<< extra.targetContextID <<")'s parent!!" << Log::endl;
    ASSERT(false);
  }

  const uint32_t op_ticket = ctxObj->getNextOperationTicket(event.eventId);
  
  mace::EventOperationInfo op_info( event.eventId, mace::EventOperationInfo::BROADCAST_OP, extra.targetContextID, ctxObj->contextName, op_ticket, event.eventOpType);
  reqEvent = event;
  reqEvent.eventType = mace::Event::BROADCASTEVENT;
  reqEvent.eventOpInfo = op_info;
  reqEvent.eventOpInfo.accessedContexts = event.eventOpInfo.accessedContexts;
  reqEvent.eventOpInfo.addAccessedContext(extra.targetContextID);
  reqEvent.eventOpInfo.requireContextName = ctxObj->contextName;
  reqEvent.eventOpInfo.permitContexts = ctxObj->getEventPermitContexts(event.eventId);
  reqEvent.ownershipOps.clear();
  reqEvent.eventOpInfo.newCreateContexts = event.eventOpInfo.newCreateContexts;

  ctxObj->addEventToContext( reqEvent.eventId, extra.targetContextID );
  if( ctxObj->checkBroadcastRequestExecutePermission(this, reqEvent.eventOpInfo, reqObj) ) {
    broadcastHead(reqObj);
  } 
}

void ExternalCommClass::createEvent(BaseMaceService* sv, mace::Event& event, const mace::string& targetContextID, 
    const mace::string& methodType, const uint8_t& event_op_type, const uint8_t& instanceUniqueID) {
  ADD_SELECTORS("ExternalCommClass::createEvent");

  const ContextService* _service = static_cast<ContextService*>(sv); 
  ScopedLock sl(mutex);
  mace::OrderID eventId( contextId, createTicket);
  createTicket ++;
  sl.unlock();

  uint64_t ver = _service->getLatestContextMapping().getCurrentVersion();
  uint64_t cver = _service->contextStructure.getCurrentVersion();

  event.initialize2(eventId, event_op_type, this->contextName, targetContextID, methodType, mace::Event::ASYNCEVENT, ver, cver);
  
  event.addServiceID(instanceUniqueID);
  mace::EventOperationInfo opInfo(eventId, mace::EventOperationInfo::ASYNC_OP, targetContextID, targetContextID, 1, event.eventOpType);
  event.eventOpInfo = opInfo;
  event.eventOpInfo.addAccessedContext(targetContextID);
}

mace::MaceAddr ContextService::getExternalCommContextAddr( const MaceKey& src, const mace::string& identifier ) {
  ADD_SELECTORS("ContextService::getExternalCommContextAddr");
  const mace::ContextMapping& ctxmapSnapshot = contextMapping.getLatestContextMapping();
  ASSERTMSG( Util::getMaceAddr() == mace::ContextMapping::getHead(ctxmapSnapshot), "It's not the headnode!" );

  ScopedLock sl(externalCommMutex);
  mace::map<mace::string, MaceAddr>::const_iterator citer = headClientExCommContextMap.find(identifier);
  
  if( citer != headClientExCommContextMap.end() ) {
    macedbg(1) << "Node("<< identifier <<") has already been assigned an proxy node " << citer->second << Log::endl;
    return citer->second;
  } 
    
  uint32_t externalCommId = nextExternalCommContextId;
  nextExternalCommContextId = (nextExternalCommContextId+1) % externalCommContextNumber;
  std::ostringstream oss;
  oss << mace::ContextMapping::EXTERNAL_COMM_CONTEXT_NAME<< "[" << externalCommId << "]";
  mace::string ctxName = oss.str();

  mace::MaceAddr addr = contextMapping.getExternalCommContextNode( ctxName );

  headClientExCommContextMap[identifier] = addr;
  macedbg(1) << "Assign external communication context("<< ctxName <<") on node("<< addr <<") to " << identifier << Log::endl;

  uint32_t externalCommContextId = getExternalCommContextID(externalCommId);
  
  send__event_externalCommControlMsg( addr, 0, externalCommId, externalCommContextId );
    
  return addr;
}

void ContextService::checkAndWaitExternalMessageHandle() {
  ADD_SELECTORS("ContextService::checkAndWaitExternalMessageHandle");
  const mace::Event& event = ThreadStructure::myEvent();
  const uint64_t externalMessageTicket = event.externalMessageTicket;
  ASSERT( externalMessageTicket > 0 );
  macedbg(1) << "now_serving_external_message_ticket=" << now_serving_external_message_ticket << " message_ticket=" << externalMessageTicket << Log::endl;
  ScopedLock sl(externalCommMutex);
  ASSERT( externalMessageTicket >= now_serving_external_message_ticket );
  if( externalMessageTicket > now_serving_external_message_ticket) {
    pthread_cond_t cond;
    pthread_cond_init( &cond, NULL );
    externalMsgWaitingThread[ externalMessageTicket ] = &cond;
    pthread_cond_wait( &cond, &externalCommMutex );
    pthread_cond_destroy( &cond );
  }
  ASSERT( externalMessageTicket == now_serving_external_message_ticket );
  macedbg(1) << "Now serving external ticket " << externalMessageTicket << Log::endl;
}

void ContextService::signalExternalMessageThread() {
  ADD_SELECTORS("ContextService::signalExternalMessageThread");
  ScopedLock sl(externalCommMutex);
  macedbg(1) << "Ticket " << now_serving_external_message_ticket << " signal next external message!" << Log::endl;
  now_serving_external_message_ticket ++;
  std::map<uint64_t, pthread_cond_t*>::iterator iter = externalMsgWaitingThread.find(now_serving_external_message_ticket);
  if( iter != externalMsgWaitingThread.end() ) {
    macedbg(1) << "Signal waiting external message " << now_serving_external_message_ticket << Log::endl;
    pthread_cond_signal( iter->second );
    externalMsgWaitingThread.erase(iter);
  }
}

ExternalCommClass* ContextService::getExternalCommClass() {
  ADD_SELECTORS("ContextService::getExternalCommClass");
  ScopedLock sl(externalCommMutex);
  if( externalCommClasses.size() == 0 ) {
    const ContextMapping& ctxmapSnapshot = contextMapping.getLatestContextMapping();
    ASSERTMSG( Util::getMaceAddr() == mace::ContextMapping::getHead(ctxmapSnapshot), "It's not the headnode!" );

    uint32_t externalCommId = nextExternalCommContextId;
    nextExternalCommContextId = (nextExternalCommContextId+1) % externalCommContextNumber;
    uint32_t externalCommContextId = getExternalCommContextID(externalCommId);

    ExternalCommClass* exCommClass = new ExternalCommClass(externalCommContextId, externalCommId);
    externalCommClassMap[externalCommId] = exCommClass;
    externalCommClasses.push_back(externalCommId);
  }

  uint32_t externalCommId = externalCommClasses[nextExternalCommClassId];
  macedbg(1) << "externalCommContext["<< externalCommId <<"] will serve!" << Log::endl;
  nextExternalCommClassId = (nextExternalCommClassId+1) % externalCommClasses.size();

  return externalCommClassMap[externalCommId];
}

void ContextService::send__event_commit( MaceAddr const& destNode, mace::Event const& event, mace::string const& ctxName ){
  mace::InternalMessageID msgId( Util::getMaceAddr(), ctxName, 0);
  mace::InternalMessage msg( mace::commit, msgId, event );
  forwardInternalMessage( destNode, msg );
}

  void ContextService::const_send__event_commit( MaceAddr const& dest, mace::Event const& event, mace::string const& ctxName ) const{
    ADD_SELECTORS("ContextService::const_send__event_commit");
    ContextService *self = const_cast<ContextService *>( this );
    mace::InternalMessageID msgId( Util::getMaceAddr(), ctxName, 0);
    mace::InternalMessage msg( mace::commit, msgId, event );

    self->forwardInternalMessage( dest, msg );
  }
  void ContextService::send__event_snapshot( MaceAddr const& dest, mace::Event const& event, mace::string const& targetContextID, mace::string const& snapshotContextID, mace::string const& snapshot ){
    mace::InternalMessage msg( mace::snapshot, event, targetContextID, snapshotContextID, snapshot );
    forwardInternalMessage( dest, msg );
  }
  void ContextService::send__event_create_response( MaceAddr const& dest, mace::Event const& event, uint32_t const& counter, MaceAddr const& targetAddress){
    mace::InternalMessage msg( mace::create_response, event, counter, targetAddress );
    forwardInternalMessage( dest, msg );
  }
  void ContextService::const_send__event_create( MaceAddr const& dest, __asyncExtraField const& extra, uint64_t const& counter, uint32_t const& ctxId ) const {
    ContextService *self = const_cast<ContextService *>( this );
    
    mace::InternalMessage msg( mace::create, extra, counter, ctxId);
    self->forwardInternalMessage( dest, msg );
  }
  void ContextService::send__event_downgrade_context( MaceAddr const& dest, uint32_t const contextID, mace::OrderID const& eventID, bool const isresponse ){
    mace::InternalMessage msg( mace::downgrade_context, contextID, eventID, isresponse );
    forwardInternalMessage( dest, msg );
  }
  void ContextService::send__event_AllocateContextObjectMsg( mace::OrderID const& eventID, mace::ContextMapping const& ctxmapCopy, 
      MaceAddr const newHead, mace::map< uint32_t, mace::string > const& contextSet, int8_t const eventType, 
      const uint64_t& version, const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, 
      const mace::map<mace::string, uint64_t>& vers ){

    mace::InternalMessage msg( mace::AllocateContextObject, newHead, contextSet, eventID, ctxmapCopy, eventType, version, 
      ownershipPairs, vers );

    for( mace::map<uint32_t, mace::string>::const_iterator iter = contextSet.begin(); iter!=contextSet.end(); iter++ ) {
      const MaceAddr& dest = mace::ContextMapping::getNodeByContext(ctxmapCopy, iter->first);
      forwardInternalMessage( dest, msg );
    }
  }

  //bsang message sending
  void ContextService::send__event_AllocateContextObjReqMsg(mace::string const& ctx_name, mace::OrderID const& eventId,
    const mace::vector< mace::pair<mace::string, mace::string> >& ownershipPairs, const mace::map< mace::string, uint64_t>& vers ) {

    ADD_SELECTORS("ContextService::send__event_AllocateContextObjReqMsg");
    macedbg(1) << "Try to create context: " << ctx_name << Log::endl;
    mace::InternalMessage msg( mace::AllocateContextObjReq, ctx_name, eventId, ownershipPairs, vers );
    forwardInternalMessage( contextMapping.getHead(), msg);
  }

  void ContextService::send__event_contextMappingUpdateReqMsg(const uint64_t expectVer) {
    ADD_SELECTORS("ContextService::send__event_contextMappingUpdateReqMsg");
    macedbg(1) << "Request ContextMapping of version " << expectVer << Log::endl;
    mace::InternalMessage msg( mace::ContextMappingUpdateReq, expectVer );
    forwardInternalMessage( contextMapping.getHead(), msg);
  }

   void ContextService::send__event_AllocateContextObjectResponseMsg(mace::string const& ctx_name, mace::OrderID const& eventId, bool const& isCreateContextEvent) {
    mace::InternalMessage msg( mace::AllocateContextObjectResponse, ctx_name, eventId, isCreateContextEvent);
    forwardInternalMessage( contextMapping.getHead(), msg);
  }

  void ContextService::send__event_contextMappingUpdateSuggest( const mace::MaceAddr& dest, const uint64_t ver ) const {
    mace::InternalMessage msg( mace::ContextMappingUpdateSuggest, ver);
    forwardInternalMessage(dest, msg);
  }

  void ContextService::send__event_migrate_context(mace::MaceAddr const& newNode, mace::string const& contextName, uint64_t const delay ){
    mace::InternalMessage msg( mace::migrate_context, newNode, contextName, delay );
    forwardInternalMessage( contextMapping.getHead(), msg );
  }
  void ContextService::send__event_migrate_param(mace::string const& paramid ){
    mace::InternalMessage msg( mace::migrate_param, paramid );
    forwardInternalMessage( contextMapping.getHead(), msg );
  }
  void ContextService::send__event_routine_return( mace::MaceAddr const& src, mace::string const& returnValueStr ) const{
    ADD_SELECTORS("ContextService::send__event_routine_return");
    macedbg(1) << "Send event(" << ThreadStructure::myEventID() << ") back!" << Log::endl;
    mace::InternalMessage msg( mace::routine_return, returnValueStr, ThreadStructure::myEvent() );

    forwardInternalMessage(src, msg);
  }
  void ContextService::send__event_RemoveContextObject( mace::OrderID const& eventID, mace::ContextMapping const& ctxmapCopy, MaceAddr const& dest, uint32_t contextID ){
    mace::InternalMessage msg( mace::RemoveContextObject, eventID, ctxmapCopy, dest, contextID );

    const mace::map < MaceAddr,uint32_t >& physicalNodes = mace::ContextMapping::getAllNodes( ctxmapCopy);
    for( mace::map<MaceAddr, uint32_t>::const_iterator nodeIt = physicalNodes.begin(); nodeIt != physicalNodes.end(); nodeIt ++ ){
      sender->sendInternalMessage( nodeIt->first ,  msg );
    }
    //std::for_each( physicalNodes.begin(), physicalNodes.end(), std::bind2nd( sendInternalMessage, msg) );
  }
  void ContextService::send__event_delete_context( mace::string const& contextName ){
    mace::InternalMessage msg( mace::delete_context, contextName );
    sender->sendInternalMessage( contextMapping.getHead() ,  msg );
  }

  void ContextService::send__event_asyncEvent( mace::MaceAddr const& dest, mace::AsyncEvent_Message* const eventObject, mace::string const& ctxName ) {
    ADD_SELECTORS("ContextService::send__event_asyncEvent");
    mace::InternalMessageID msgId( Util::getMaceAddr(), ctxName, 0);
    mace::InternalMessage msg( eventObject, msgId, instanceUniqueID );
    forwardInternalMessage(dest, msg);
  }

  void ContextService::send__event_externalCommControlMsg( mace::MaceAddr const& dest, const uint8_t& control_type, const uint32_t& externalCommId, 
      const uint32_t& externalCommContextId ) {
    ADD_SELECTORS("ContextService::send__event_externalCommControlMsg");
    mace::InternalMessage msg( mace::ExternalCommControl, control_type, externalCommId, externalCommContextId );
    forwardInternalMessage( dest, msg );
  }

void ContextService::send__event_commitContextsMsg( MaceAddr const& destNode, mace::vector< mace::string > const& cctxNames, 
    mace::string const& src_contextName, mace::OrderID const& eventId ) const {
  mace::InternalMessageID msgId( Util::getMaceAddr(), "", 0);

  mace::string dest_contextName;
  mace::vector<mace::EventOperationInfo> opInfos;
  uint32_t newContextId = 0;
  mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::COMMIT_CONTEXTS,  
    src_contextName, dest_contextName, eventId, cctxNames, opInfos, newContextId);
  forwardInternalMessage(destNode, msg);
}

void ContextService::send__event_releaseContext( mace::string const& dominator, mace::OrderID const& eventId, 
    mace::string const& releaseContext, mace::vector<mace::EventOperationInfo> const& localLockRequests,
    mace::vector< mace::string > const& lockedContexts, const mace::string& src_contextName ) {
  
  ADD_SELECTORS("ContextService::send__event_releaseContext");
  
  mace::ContextBaseClass* ctxObj = this->getContextObjByName(dominator);
  if( ctxObj != NULL ) {
    ctxObj->releaseContext(this, eventId, releaseContext, src_contextName, localLockRequests, lockedContexts );
    return;
  }

  const mace::ContextMapping& snapshot = this->getLatestContextMapping();
  const mace::MaceAddr& destAddr = mace::ContextMapping::getNodeByContext( snapshot, dominator );

  mace::InternalMessageID msgId( Util::getMaceAddr(), dominator, 0);

  uint32_t newContextId = 1;
  
  mace::vector<mace::string> contextNames;
  contextNames.push_back(releaseContext);
  for(uint32_t i=0; i<lockedContexts.size(); i++) {
    contextNames.push_back(lockedContexts[i]);
  }
  
  mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::RELEASE_CONTEXTS, 
    src_contextName, dominator, eventId, contextNames, localLockRequests, newContextId);

  // if( fromDominator ){
    forwardInternalMessage(destAddr, msg);
  // } else {
  //  eventExecutionControlChannel->enqueueControlMessage( src_contextName, msg );
  //}
}

void ContextService::send__event_unlockContext( mace::string const& destContext, mace::EventOperationInfo const& eop, 
    mace::vector<mace::EventOperationInfo> const& localLockRequests, mace::vector<mace::string> const& lockedContexts,
    const mace::string& src_contextName ){
  ADD_SELECTORS("ContextService::send__event_unlockContext");
  mace::ContextBaseClass* ctxObj = this->getContextObjByName( destContext );
  if( ctxObj != NULL ) {
    ctxObj->unlockContext(this, eop, localLockRequests, lockedContexts );
    return;
  }

  const mace::ContextMapping& snapshot = this->getLatestContextMapping();
  mace::MaceAddr destAddr = mace::ContextMapping::getNodeByContext(snapshot, destContext);
  
  mace::InternalMessageID msgId( Util::getMaceAddr(), destContext, 0);

  uint32_t newContextId = 1;
  mace::vector<mace::EventOperationInfo> eops;
  
  eops.push_back(eop);
  for( mace::vector<mace::EventOperationInfo>::const_iterator iter=localLockRequests.begin(); iter!=localLockRequests.end(); iter++ ){
    eops.push_back( *iter );
  }
  
  mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::UNLOCK_CONTEXT,  
    src_contextName, destContext, eop.eventId, lockedContexts, eops, newContextId);

  forwardInternalMessage(destAddr, msg);
  // eventExecutionControlChannel->enqueueControlMessage(src_contextName, msg);
}

mace::set<mace::string> ContextService::send__event_requireEventExecutePermission( mace::string const& dominator, 
    mace::EventOperationInfo const& eventOpInfo ) {

  mace::set<mace::string> permitContexts;
  mace::ContextBaseClass* ctxObj = this->getContextObjByName(dominator);
  if( ctxObj != NULL ) {
    ctxObj->checkEventExecutePermission(this, eventOpInfo, false, permitContexts );
    return permitContexts;
  }

  mace::InternalMessageID msgId( Util::getMaceAddr(), dominator, 0);

  const mace::ContextMapping& snapshot = this->getLatestContextMapping();
  mace::MaceAddr destAddr = mace::ContextMapping::getNodeByContext(snapshot, dominator);

  uint32_t newContextId = 0;
  mace::vector<mace::string> contextNames;
  mace::vector<mace::EventOperationInfo> eops;
  eops.push_back(eventOpInfo);
  
  mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::TO_LOCK_CONTEXT,  
    eventOpInfo.requireContextName, dominator, eventOpInfo.eventId, contextNames, eops, newContextId );

  forwardInternalMessage(destAddr, msg);
  // eventExecutionControlChannel->enqueueControlMessage( eventOpInfo.requireContextName, msg );
  return permitContexts;
}

void ContextService::send__event_enqueueLockRequests( mace::string const& dominator, mace::string const& requireContextName, 
    mace::vector<mace::EventOperationInfo> const& eops) {
  if( eops.size() == 0 ){
    return;
  }

  mace::OrderID eId = eops[0].eventId;
  const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
  const MaceAddr& destAddr = ContextMapping::getNodeByContext(snapshot, dominator);
        
  mace::InternalMessageID msgId( Util::getMaceAddr(), dominator, 0);

  uint32_t newContextId = 0;
  mace::vector<mace::string> contextNames;
    
  mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::ENQUEUE_LOCK_CONTEXTS,  
    requireContextName, dominator, eId, contextNames, eops, newContextId );

  forwardInternalMessage(destAddr, msg);
}

void ContextService::send__event_getReadyToCommit( mace::OrderID const& eventId, mace::string const& notifyContext, mace::string const& src_contextName ) const {
  const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
  const MaceAddr& destAddr = ContextMapping::getNodeByContext(snapshot, notifyContext);
  mace::InternalMessageID msgId( Util::getMaceAddr(), notifyContext, 0);

  uint32_t newContextId;
  mace::vector<mace::EventOperationInfo> eops;
  mace::vector<mace::string> contextNames;
  
  mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::CHECK_COMMIT,  
    src_contextName, notifyContext, eventId, contextNames, eops, newContextId);
  forwardInternalMessage(destAddr, msg);
}

void ContextService::send__event_notifyReadyToCommit( const MaceAddr& destAddr, const mace::OrderID& eventId, const mace::string& src_contextName,
    mace::vector<mace::string> const& notifyContexts, mace::vector<mace::string> const& executedContexts) const {

  mace::vector<mace::string> contextNames;
  for( uint32_t i=0; i<notifyContexts.size(); i++ ){
    contextNames.push_back( notifyContexts[i] );
  }
  contextNames.push_back("#");
  for( uint32_t i=0; i<executedContexts.size(); i++ ){
    contextNames.push_back( executedContexts[i] );
  }

  mace::string dest_contextName = "";
  mace::InternalMessageID msgId( Util::getMaceAddr(), dest_contextName, 0);

  uint32_t newContextId;
  mace::vector<mace::EventOperationInfo> eops;
    
  mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::READY_TO_COMMIT,  
    src_contextName, dest_contextName, eventId, contextNames, eops, newContextId);
  forwardInternalMessage(destAddr, msg);
}

void ContextService::send__event_EventOperationInfo( const uint8_t type, mace::string const& dest_contextName, mace::string const& src_contextName, 
    mace::EventOperationInfo const& opInfo, mace::OrderID const& eventId ) const {

  const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
  const MaceAddr& destAddr = ContextMapping::getNodeByContext(snapshot, dest_contextName);
  mace::InternalMessageID msgId( Util::getMaceAddr(), dest_contextName, 0);

  mace::vector<mace::string> contextNames; 
  uint32_t newContextId;
  mace::vector<mace::EventOperationInfo> eops;
  
  eops.push_back( opInfo );
  mace::InternalMessage msg(mace::EventExecutionControl, msgId, type, src_contextName, dest_contextName, eventId, contextNames, 
    eops, newContextId );
  forwardInternalMessage(destAddr, msg);
}

void ContextService::send__event_releaseLockOnContext( mace::string const& dest_contextName, mace::string const& src_contextName, 
    mace::OrderID const& eventId ) {

  mace::ContextBaseClass* ctxObj = this->getContextObjByName(dest_contextName);
  if( ctxObj != NULL ) {
    ctxObj->releaseLock(this, eventId );
    return;
  }

  const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
  const MaceAddr& destAddr = ContextMapping::getNodeByContext(snapshot, dest_contextName);
  mace::InternalMessageID msgId( Util::getMaceAddr(), dest_contextName, 0);

  uint32_t newContextId;
  mace::vector<mace::EventOperationInfo> eops;
  mace::vector<mace::string> context_names;
  
  mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::RELEASE_LOCK_ON_CONTEXT, 
    src_contextName, dest_contextName, eventId, context_names, eops, newContextId );

  forwardInternalMessage(destAddr, msg);
}

void ContextService::send__event_enqueueOwnershipOps( mace::EventOperationInfo const& eventOpInfo, 
    mace::string const& dest_contextName, mace::string const& src_contextName, 
    mace::vector<mace::EventOperationInfo> const& ownershipOpInfos) const {

  const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
  const MaceAddr& destAddr = ContextMapping::getNodeByContext(snapshot, dest_contextName);
  mace::InternalMessageID msgId( Util::getMaceAddr(), dest_contextName, 0);

  mace::vector<mace::string> contextNames; 
  uint32_t newContextId;

  mace::vector<mace::EventOperationInfo> eops;
  eops.push_back( eventOpInfo);
  for( uint32_t i=0; i<ownershipOpInfos.size(); i++ ){
    eops.push_back(ownershipOpInfos[i]);
  }
    
  mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::ENQUEUE_OWNERSHIP_OPS, 
    src_contextName, dest_contextName, eventOpInfo.eventId, contextNames, eops, newContextId );

  forwardInternalMessage(destAddr, msg);
}

void ContextService::send__event_enqueueOwnershipOpsReply(mace::EventOperationInfo const& eventOpInfo, mace::string const& dest_contextName, 
    mace::string const& src_contextName) const {

  const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
  const MaceAddr& destAddr = ContextMapping::getNodeByContext(snapshot, dest_contextName);
  mace::InternalMessageID msgId( Util::getMaceAddr(), dest_contextName, 0);

  mace::vector<mace::string> contextNames; 
  uint32_t newContextId;

  mace::vector<mace::EventOperationInfo> eops;
  eops.push_back(eventOpInfo);
  
  mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::ENQUEUE_OWNERSHIP_OPS_REPLY, 
    src_contextName, dest_contextName, eventOpInfo.eventId, contextNames, eops, newContextId );

  forwardInternalMessage(destAddr, msg);
}

void ContextService::send__event_enqueueSubEvent( mace::EventOperationInfo const& eventOpInfo, mace::string const& dest_contextName, 
    mace::string const& src_contextName, mace::EventRequestWrapper const& eventRequest) const {
  ADD_SELECTORS("ContextService::send__event_enqueueSubEvent");
  macedbg(1) << "Send a subevent of "<< eventOpInfo.eventId << " to " << dest_contextName << " from " << src_contextName << Log::endl;
  const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
  const MaceAddr& destAddr = ContextMapping::getNodeByContext( snapshot, dest_contextName );
  mace::InternalMessageID msgId( Util::getMaceAddr(), dest_contextName, 0 );
  mace::InternalMessage msg( mace::EnqueueEventRequest, msgId, eventOpInfo, dest_contextName, src_contextName, eventRequest);
  forwardInternalMessage( destAddr, msg );
}

void ContextService::send__event_enqueueSubEventReply( mace::EventOperationInfo const& eventOpInfo, mace::string const& dest_contextName, 
    mace::string const& src_contextName) const {
  ADD_SELECTORS("ContextService::send__event_enqueueSubEventReply");
  const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
  const MaceAddr& destAddr = ContextMapping::getNodeByContext( snapshot, dest_contextName );
  mace::InternalMessageID msgId( Util::getMaceAddr(), dest_contextName, 0 );
  mace::InternalMessage msg( mace::EnqueueEventReply, msgId, eventOpInfo, dest_contextName, src_contextName);
  forwardInternalMessage( destAddr, msg );
}

void ContextService::send__event_enqueueExternalMessage( mace::EventOperationInfo const& eventOpInfo, mace::string const& dest_contextName, 
    mace::string const& src_contextName, mace::EventMessageRecord const& msgRecord) const {
  ADD_SELECTORS("ContextService::send__event_enqueueExternalMessage");
  macedbg(1) << "Send a externalmessage of "<< eventOpInfo.eventId << " to " << dest_contextName << " from " << src_contextName << Log::endl;
  const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
  const MaceAddr& destAddr = ContextMapping::getNodeByContext( snapshot, dest_contextName );
  mace::InternalMessageID msgId( Util::getMaceAddr(), dest_contextName, 0 );
  mace::InternalMessage msg( mace::EnqueueMessageRequest, msgId, eventOpInfo, dest_contextName, src_contextName, msgRecord);
  forwardInternalMessage( destAddr, msg );
}

void ContextService::send__event_enqueueExternalMessageReply( mace::EventOperationInfo const& eventOpInfo, mace::string const& dest_contextName, 
    mace::string const& src_contextName) const {
  ADD_SELECTORS("ContextService::send__event_enqueueExternalMessageReply");
  const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
  const MaceAddr& destAddr = ContextMapping::getNodeByContext( snapshot, dest_contextName );
  mace::InternalMessageID msgId( Util::getMaceAddr(), dest_contextName, 0 );
  mace::InternalMessage msg( mace::EnqueueMessageReply, msgId, eventOpInfo, dest_contextName, src_contextName);
  forwardInternalMessage( destAddr, msg );
}

void ContextService::send__event_CommitDoneMsg(mace::string const& create_ctx_name, mace::string const& target_ctx_name, 
    mace::OrderID const& eventId, mace::set<mace::string> const& coaccess_contexts) {
  // const mace::ContextMapping& snapshotMapping = contextMapping.getLatestContextMapping();
  mace::InternalMessageID msgId(Util::getMaceAddr(), target_ctx_name, 0);
  mace::InternalMessage msg( mace::CommitDone, msgId, create_ctx_name, target_ctx_name, eventId, coaccess_contexts);

  forwardInternalMessage( this->getNodeByContext(target_ctx_name), msg);
}

void ContextService::send__ownership_ownershipOperations( const mace::string& dominator, const mace::EventOperationInfo& eop, 
    const mace::string& contextName, const mace::vector<mace::EventOperationInfo>& ownershipOpInfos) const {

  ADD_SELECTORS("ContextService::send__event_ownershipOperations");
  macedbg(1) << "Forward ownerships of "<< eop <<" from "<< contextName <<" to " << dominator << Log::endl;

  mace::vector<mace::EventOperationInfo> eops;
  eops.push_back(eop);
  for( uint32_t i=0; i<ownershipOpInfos.size(); i++ ){
    eops.push_back(ownershipOpInfos[i]);
  }


  const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
  const MaceAddr& destAddr = ContextMapping::getNodeByContext(snapshot, dominator);

  mace::string dest_contextName = dominator;
  mace::string src_contextName = contextName;
  mace::OrderID eventId = eop.eventId;
  mace::vector< mace::pair<mace::string, mace::string> > ownerships;
  mace::map<mace::string, uint64_t> vers; 
  mace::set<mace::string> contextSet;
  mace::vector<mace::string> contextVector;

  mace::InternalMessageID msgId(Util::getMaceAddr(), dest_contextName, 0);

  mace::InternalMessage msg( mace::ContextOwnershipControl, msgId, mace::ContextOwnershipControl_Message::OWNERSHIP_MODIFY,
    eops, dest_contextName, src_contextName, eventId, ownerships, vers, contextSet, contextVector);
  forwardInternalMessage(destAddr, msg);
}

// void ContextService::send__event_updateOwnership(const mace::MaceAddr& destAddr, mace::set<mace::string> const& ctxNames, 
//     mace::vector< mace::pair<mace::string, mace::string> > const& ownerships, const uint64_t ver ) const {

//   mace::vector<mace::EventOperationInfo> ownershipOpInfos; 
//   mace::string dest_contextName;
//   mace::string src_contextName;
//   mace::OrderID eventId;
//   mace::ContextEventDominator::EventLockQueue eventLockQueues;
//   mace::map<mace::string, mace::string> prePDominators;
//   mace::map<mace::string, mace::string> preDominators; 

//   mace::InternalMessageID msgId(Util::getMaceAddr(), "", 0);
  
//   mace::InternalMessage msg( mace::ContextOwnershipControl, msgId, mace::ContextOwnershipControl_Message::OWNERSHIP_UPDATE,
//     ownershipOpInfos, dest_contextName, src_contextName, eventId, ownerships, ver, ctxNames, eventLockQueues, prePDominators, preDominators);
//   forwardInternalMessage(destAddr, msg);
// }

void ContextService::send__ownership_updateOwnershipReply( const mace::string& destContext, const mace::set<mace::string>& srcContexts,
    const mace::vector<mace::EventOperationInfo>& reply_eops ) const {

  const mace::ContextMapping& snapshot = getLatestContextMapping();
  const mace::MaceAddr& destAddr = mace::ContextMapping::getNodeByContext(snapshot, destContext);

  mace::InternalMessageID msgId(Util::getMaceAddr(), destContext, 0);

  mace::OrderID eventId; 
  mace::string src_contextName; 
  mace::vector< mace::pair<mace::string, mace::string> > ownerships;
  mace::map<mace::string, uint64_t> vers; 
  mace::vector< mace::string >  contextVector;

  mace::InternalMessage msg( mace::ContextOwnershipControl, msgId, mace::ContextOwnershipControl_Message::OWNERSHIP_UPDATE_DONE,
    reply_eops, destContext, src_contextName, eventId, ownerships, vers, srcContexts, contextVector);
  forwardInternalMessage(destAddr, msg);
}

void ContextService::send__event_modifyOwnershipReply( mace::MaceAddr const& destAddr, mace::string const& ctxName, 
    mace::EventOperationInfo const& eop ) const {

  mace::InternalMessageID msgId( Util::getMaceAddr(), ctxName, 0 );

  mace::vector<mace::EventOperationInfo> ownershipOpInfos; 
  ownershipOpInfos.push_back(eop);
  
  mace::string src_contextName = "globalContext";
  mace::vector< mace::pair<mace::string, mace::string> > ownerships;
  mace::map<mace::string, uint64_t> vers; 
  mace::set<mace::string> contextSet;
  mace::vector<mace::string> contextVector;
  
  mace::InternalMessage msg( mace::ContextOwnershipControl, msgId, mace::ContextOwnershipControl_Message::OWNERSHIP_MODIFY_DONE,
    ownershipOpInfos, ctxName, src_contextName, eop.eventId, ownerships, vers, contextSet, contextVector);
  forwardInternalMessage(destAddr, msg);
}

void ContextService::send__ownership_updateOwnershipAndDominators(const mace::MaceAddr& destAddr, const mace::string& src_contextName,
    const mace::set<mace::string>& update_doms, const mace::vector<mace::EventOperationInfo>& forward_eops,
    const mace::vector< mace::pair<mace::string, mace::string> >& ownerships, const mace::map<mace::string, uint64_t>& vers ) const {

  mace::string dest_contextName;
  mace::OrderID eventId;  
  mace::vector<mace::string> contextVector;
  
  mace::InternalMessageID msgId(Util::getMaceAddr(), "", 0);
  
  mace::InternalMessage msg( mace::ContextOwnershipControl, msgId, 
    mace::ContextOwnershipControl_Message::OWNERSHIP_AND_DOMINATOR_UPDATE,
    forward_eops, dest_contextName, src_contextName, eventId, ownerships, vers, update_doms, contextVector );
  forwardInternalMessage(destAddr, msg);
}

void ContextService::send__ownership_contextDAGRequest( const mace::string& src_contextName, const mace::MaceAddr& destAddr, 
    const mace::set<mace::string>& context_set ) const {

  mace::vector< mace::EventOperationInfo > ownershipOpInfos;
  mace::string dest_contextName;
  mace::OrderID eventId;
  mace::vector< mace::pair<mace::string, mace::string> > ownerships;
  mace::map< mace::string, uint64_t > versions;
  mace::vector< mace::string >  contextVector;
  
  mace::InternalMessageID msgId(Util::getMaceAddr(), "", 0);
  
  mace::InternalMessage msg( mace::ContextOwnershipControl, msgId, 
    mace::ContextOwnershipControl_Message::OWNERSHIP_UPDATE_REQUEST,
    ownershipOpInfos, dest_contextName, src_contextName, eventId, ownerships, versions, context_set, contextVector );
  forwardInternalMessage(destAddr, msg);
}

void ContextService::send__ownership_contextDAGReply( const mace::MaceAddr& destAddr, const mace::string& dest_contextName,
    const mace::set<mace::string>& contextSet, const mace::vector< mace::pair<mace::string, mace::string> >& ownerships,
    const mace::map<mace::string, uint64_t>& versions ) const {

  mace::vector< mace::EventOperationInfo > ownershipOpInfos;
  mace::string src_contextName;
  mace::OrderID eventId;
  mace::vector< mace::string >  contextVector;
  
  mace::InternalMessageID msgId(Util::getMaceAddr(), "", 0);
  
  mace::InternalMessage msg( mace::ContextOwnershipControl, msgId, 
    mace::ContextOwnershipControl_Message::OWNERSHIP_UPDATE_REPLY,
    ownershipOpInfos, dest_contextName, src_contextName, eventId, ownerships, versions, contextSet, contextVector );
  forwardInternalMessage(destAddr, msg);
}

void ContextService::createNewOwnership(mace::string const& pContextName, mace::string const& cContextName) {
  ADD_SELECTORS("ContextService::createNewOwnership");
  macedbg(1) << "Create new ownership("<< pContextName<<", " << cContextName<<")!" << Log::endl;
  modifyOwnership( mace::EventOperationInfo::ADD_OWNERSHIP_OP, pContextName, cContextName);
}

void ContextService::removeOwnership(mace::string const& pContextName, mace::string const& cContextName) {
  ADD_SELECTORS("ContextService::removeOwnership");
  macedbg(1) << "Remove ownership("<< pContextName<<", " << cContextName<<")!" << Log::endl;
  modifyOwnership( mace::EventOperationInfo::DELETE_OWNERSHIP_OP, pContextName, cContextName);
}

uint32_t ContextService::createNewContext(mace::string const& contextTypeName) {
  mace::ContextBaseClass* ctxObj = ThreadStructure::myContext();
  mace::EventOperationInfo& eop = (ThreadStructure::myEvent()).eventOpInfo;
  return ctxObj->createNewContext(this, contextTypeName, eop);
}

void ContextService::addNewContextName( const mace::string& new_ctx_name ) {
  mace::EventOperationInfo& eop = (ThreadStructure::myEvent()).eventOpInfo;
  eop.newCreateContexts.insert(new_ctx_name);
}

void ContextService::send__event_createNewContext( mace::string const& src_contextName, mace::string const& contextTypeName, 
    mace::EventOperationInfo const& eventOpInfo ) const {
  
  const mace::ContextMapping& snapshot = getLatestContextMapping();
  const mace::MaceAddr& headAddr = mace::ContextMapping::getHead(snapshot);
  mace::InternalMessageID msgId( Util::getMaceAddr(), mace::ContextMapping::GLOBAL_CONTEXT_NAME, 0);

  mace::vector<mace::string> contextNames; 
  contextNames.push_back( contextTypeName );

  mace::vector<mace::EventOperationInfo> opInfos;
  opInfos.push_back(eventOpInfo);

  uint32_t newContextId = 0;
  mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::CREATE_NEW_CONTEXT, 
    src_contextName, mace::ContextMapping::GLOBAL_CONTEXT_NAME, eventOpInfo.eventId, contextNames, opInfos, newContextId );
  forwardInternalMessage(headAddr, msg);
}

void ContextService::send__event_createNewContextReply( mace::string const& dest_contextName, mace::EventOperationInfo const& eventOpInfo, const uint32_t& newContextId) const {
  const mace::ContextMapping& snapshot = getLatestContextMapping();
  const mace::MaceAddr& destAddr = mace::ContextMapping::getNodeByContext(snapshot, dest_contextName);
  mace::InternalMessageID msgId( Util::getMaceAddr(), dest_contextName, 0);

  mace::string src_contextName = mace::ContextMapping::GLOBAL_CONTEXT_NAME;
  mace::vector<mace::string> contextNames;

  mace::vector<mace::EventOperationInfo> opInfos;
  opInfos.push_back(eventOpInfo);

  mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::CREATE_NEW_CONTEXT_REPLY, 
    src_contextName, dest_contextName, eventOpInfo.eventId, contextNames, opInfos, newContextId);
  forwardInternalMessage(destAddr, msg);
}

void ContextService::addEventRequest( mace::AsyncEvent_Message* reqObject){
  ADD_SELECTORS("ContextService::addEventRequest");
  mace::ContextBaseClass* ctxObj = ThreadStructure::myContext();
  mace::Event& event = ThreadStructure::myEvent();
  mace::EventRequestWrapper request(instanceUniqueID, reqObject);
  ctxObj->enqueueSubEvent(this, event.eventOpInfo, event.target_ctx_name, request);
}

// Locking methods
void ContextService::send__event_replyEventExecutePermission( mace::string const& dest_contextName, 
    mace::string const& src_contextName, mace::OrderID const& eventId, mace::vector<mace::string> const& permittedContextNames, 
    mace::vector<mace::EventOperationInfo> const& eventOpInfos ) {
  
  mace::ContextBaseClass* ctxObj = this->getContextObjByName(dest_contextName);
  if( ctxObj != NULL ){
    ctxObj->handleEventExecutePermission(this, eventOpInfos, permittedContextNames );
    return;
  }

  const ContextMapping& snapshot = contextMapping.getLatestContextMapping();
  const MaceAddr& destAddr = ContextMapping::getNodeByContext(snapshot, dest_contextName);

  mace::InternalMessageID msgId( Util::getMaceAddr(), dest_contextName, 0);
  uint32_t newContextId;
  
  mace::InternalMessage msg(mace::EventExecutionControl, msgId, mace::EventExecutionControl_Message::LOCK_CONTEXT_PERMISSION,  
    src_contextName, dest_contextName, eventId, permittedContextNames, eventOpInfos, newContextId);

  forwardInternalMessage(destAddr, msg);
}

// Elasticity methods
uint32_t ContextService::getConnectionStrength( const uint32_t handler_thread_id, mace::string const& context_name, 
    mace::MaceAddr const& addr ) {
  mace::ContextBaseClass* ctx_obj = getContextObjByName( context_name );
  if( ctx_obj != NULL ) {
    const mace::ContextMapping& snapshot = this->getLatestContextMapping();
    return (ctx_obj->runtimeInfo).getConnectionStrength( snapshot, addr );
  } else {
    return getRemoteConnectionStrength( handler_thread_id, context_name, addr );
  }
}

uint32_t ContextService::getRemoteConnectionStrength( const uint32_t handler_thread_id, mace::string const& context_name, 
    mace::MaceAddr const& addr ) {
  ADD_SELECTORS("ContextService::getRemoteConnectionStrength");
  ScopedLock sl(elasticityHandlerMutex);

  std::map< mace::string, RemoteContextRuntimeInfo>::iterator iter = remoteContextRuntimeInfos.find( context_name );
  if( iter != remoteContextRuntimeInfos.end() && (iter->second).getConnectionStrength(addr) > 0 ) { 
    return (iter->second).getConnectionStrength(addr);
  }
  macedbg(1) << "Try to get connection strength between node("<< addr <<") and context("<< context_name <<")!" << Log::endl;

  this->send__elasticity_ContextNodeStrengthRequest( handler_thread_id, context_name, addr );
  pthread_cond_t cond;
  pthread_cond_init( &cond, NULL );
  elasticityHandlerConds[ handler_thread_id ] = &cond;
  pthread_cond_wait( &cond, &elasticityHandlerMutex );
  pthread_cond_destroy( &cond );
  elasticityHandlerConds[ handler_thread_id ] = NULL;
  elasticityHandlerConds.erase( handler_thread_id );

  iter = remoteContextRuntimeInfos.find( context_name );
  ASSERT( iter != remoteContextRuntimeInfos.end() );
  return (iter->second).getConnectionStrength(addr);
}

void ContextService::send__elasticity_ContextNodeStrengthRequest( const uint32_t& handler_thread_id, mace::string const& context_name, 
    mace::MaceAddr const& addr) const {
  const mace::ContextMapping& snapshot = getLatestContextMapping();
  const mace::MaceAddr& dest = mace::ContextMapping::getNodeByContext( snapshot, context_name );

  uint64_t value = 0;
  mace::vector<mace::string> context_names;
  mace::map<mace::MaceAddr, mace::ServerRuntimeInfo> servers_info;
  mace::vector< mace::MigrationContextInfo> m_contexts_info;

  mace::InternalMessageID msgId( Util::getMaceAddr(), context_name, 0 );

  mace::InternalMessage msg( mace::ElasticityControl, msgId, mace::ElasticityControl_Message::CONTEXT_NODE_STRENGTH_REQ, 
    handler_thread_id, context_name, addr, value, context_names, servers_info, m_contexts_info);
  forwardInternalMessage(dest, msg);
}

void ContextService::send__elasticity_ContextNodeStrengthReply( mace::MaceAddr const& dest, const uint32_t& handler_thread_id, mace::string const& context_name, 
    mace::MaceAddr const& addr, const uint64_t& value ) const {

  mace::InternalMessageID msgId( Util::getMaceAddr(), "", 0 );
  mace::vector<mace::string> context_names;
  mace::map<mace::MaceAddr, mace::ServerRuntimeInfo> servers_info;
  mace::vector< mace::MigrationContextInfo> m_contexts_info;

  mace::InternalMessage msg( mace::ElasticityControl, msgId, mace::ElasticityControl_Message::CONTEXT_NODE_STRENGTH_REP, 
    handler_thread_id, context_name, addr, value, context_names, servers_info, m_contexts_info );
  forwardInternalMessage(dest, msg);
}

void ContextService::send__elasticity_contextsColocateRequest( mace::string const& colocate_dest_context, 
    mace::vector<mace::string> const& colocate_src_contexts ) const {

  const mace::ContextMapping& snapshot = getLatestContextMapping();
  
  uint32_t handler_thread_id = 0;
  mace::MaceAddr addr;
  uint64_t value = 0;
  mace::map<mace::MaceAddr, mace::ServerRuntimeInfo> servers_info;
  mace::vector< mace::MigrationContextInfo> m_contexts_info;

  mace::InternalMessageID msgId( Util::getMaceAddr(), "", 0 );

  mace::InternalMessage msg( mace::ElasticityControl, msgId, mace::ElasticityControl_Message::CONTEXTS_COLOCATE_REQ, 
    handler_thread_id, colocate_dest_context, addr, value, colocate_src_contexts, servers_info, m_contexts_info);
  forwardInternalMessage( mace::ContextMapping::getHead(snapshot), msg);
}

void ContextService::send__elasticity_informationReport( const mace::ServerRuntimeInfo& server_info ) const {
  const mace::ContextMapping& snapshot = getLatestContextMapping();
  
  uint32_t handler_thread_id = 0;
  mace::string ctx_name;
  mace::MaceAddr addr;
  uint64_t value;
  mace::vector<mace::string> ctx_names;
  mace::map<mace::MaceAddr, mace::ServerRuntimeInfo> servers_info;
  servers_info[ Util::getMaceAddr() ] = server_info;
  mace::vector< mace::MigrationContextInfo> m_contexts_info;

  mace::InternalMessageID msgId( Util::getMaceAddr(), "", 0 );

  mace::InternalMessage msg( mace::ElasticityControl, msgId, mace::ElasticityControl_Message::SERVER_INFO_REPORT, 
    handler_thread_id, ctx_name, addr, value, ctx_names, servers_info, m_contexts_info);
  forwardInternalMessage( mace::ContextMapping::getHead(snapshot), msg);
}

void ContextService::send__elasticity_serverInfoRequest( const uint32_t& handler_thread_id ) const {
  const mace::ContextMapping& snapshot = getLatestContextMapping();
  
  mace::string ctx_name;
  mace::MaceAddr addr;
  uint64_t value = 0;
  mace::vector<mace::string> ctx_names;
  mace::map<mace::MaceAddr, mace::ServerRuntimeInfo> servers_info;
  mace::vector< mace::MigrationContextInfo> m_contexts_info;

  mace::InternalMessageID msgId( Util::getMaceAddr(), "", 0 );

  mace::InternalMessage msg( mace::ElasticityControl, msgId, mace::ElasticityControl_Message::SERVER_INFO_REQ, 
    handler_thread_id, ctx_name, addr, value, ctx_names, servers_info, m_contexts_info);
  forwardInternalMessage( mace::ContextMapping::getHead(snapshot), msg);
}

void ContextService::send__elasticity_serverInfoReply( const mace::MaceAddr& dest, const uint32_t& handler_thread_id, 
    const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info, const uint64_t& value, const mace::MaceAddr& addr ) const {
    
  mace::string ctx_name;
  mace::vector<mace::string> ctx_names;
  mace::vector< mace::MigrationContextInfo> m_contexts_info;

  mace::InternalMessageID msgId( Util::getMaceAddr(), "", 0 );

  mace::InternalMessage msg( mace::ElasticityControl, msgId, mace::ElasticityControl_Message::SERVER_INFO_REPLY, 
    handler_thread_id, ctx_name, addr, value, ctx_names, servers_info, m_contexts_info);
  forwardInternalMessage( dest, msg);
}

void ContextService::send__elasticity_contextMigrationQuery( const mace::MaceAddr& dest, 
    const mace::vector< mace::MigrationContextInfo>& m_contexts_info ) const {

  uint64_t handler_thread_id = 0;
  mace::string ctx_name;
  mace::MaceAddr addr;
  uint64_t value = 0;
  mace::vector<mace::string> ctx_names;
  mace::map< mace::MaceAddr, mace::ServerRuntimeInfo > servers_info;

  mace::InternalMessageID msgId( Util::getMaceAddr(), "", 0 );

  mace::InternalMessage msg( mace::ElasticityControl, msgId, mace::ElasticityControl_Message::CONTEXTS_MIGRATION_QUERY, 
    handler_thread_id, ctx_name, addr, value, ctx_names, servers_info, m_contexts_info);
  forwardInternalMessage( dest, msg);
}

void ContextService::send__elasticity_contextMigrationQueryReply( const mace::MaceAddr& dest, 
    const mace::vector< mace::string>& accept_m_ctxs ) const {

  uint64_t handler_thread_id = 0;
  mace::string ctx_name;
  mace::MaceAddr addr;
  uint64_t value = 0;
  mace::map< mace::MaceAddr, mace::ServerRuntimeInfo > servers_info;
  mace::vector< mace::MigrationContextInfo> m_contexts_info;

  mace::InternalMessageID msgId( Util::getMaceAddr(), "", 0 );

  mace::InternalMessage msg( mace::ElasticityControl, msgId, mace::ElasticityControl_Message::CONTEXTS_MIGRATION_QUERY_REPLY, 
    handler_thread_id, ctx_name, addr, value, accept_m_ctxs, servers_info, m_contexts_info);
  forwardInternalMessage( dest, msg);
}

uint64_t ContextService::getContextCPUTime( const mace::string& ctx_name ) {
  mace::ContextBaseClass* ctxObj = this->getContextObjByName( ctx_name );
  if( ctxObj == NULL ){
    return 0;
  }

  return ctxObj->getCPUTime();
}

double ContextService::getContextCPUUsage( const mace::string& ctx_name ) {
  mace::ContextBaseClass* ctxObj = this->getContextObjByName( ctx_name );
  if( ctxObj == NULL ){
    return 0;
  }

  uint64_t cpu_time = ctxObj->getCPUTime();

  std::vector<mace::ContextBaseClass*> contextObjs;

  mace::set<mace::string> all_types;
  this->getContextObjectsByTypes( contextObjs, all_types );
  uint64_t total_cpu_time = 0;
  for( uint32_t i=0; i<contextObjs.size(); i++ ) {
    total_cpu_time = contextObjs[i]->getCPUTime();
    contextObjs[i] = NULL;
  }

  double ctx_cpu_usage = elasticityMonitor->getCurrentCPUUsage() * ( cpu_time / total_cpu_time );
  return ctx_cpu_usage;
}

// mace::map< mace::MaceAddr, double > ContextService::getServersCPUUsage() {
//   const mace::ContextMapping& snapshot = this->getLatestContextMapping();
//   if( elasticityMonitor->hasLatestServersInfo() || Util::getMaceAddr() == mace::ContextMapping::getHead(snapshot) ) {
//     return elasticityMonitor->getServersCPUUsage();
//   }

//   uint32_t handler_thread_id = 0;
//   this->send__elasticity_serverInfoRequest( handler_thread_id );
//   pthread_cond_t cond;
//   pthread_cond_init( &cond, NULL );
//   elasticityHandlerConds[ handler_thread_id ] = &cond;
//   pthread_cond_wait( &cond, &elasticityHandlerMutex );
//   pthread_cond_destroy( &cond );
//   elasticityHandlerConds[ handler_thread_id ] = NULL;
//   elasticityHandlerConds.erase( handler_thread_id );

//   return elasticityMonitor->getServersCPUUsage();
// }

void ContextService::wakeupHandlerThreadForContextStrength( const uint32_t& handler_thread_id, mace::string const& context_name, 
    mace::MaceAddr const& addr, const uint64_t& value ) {
  ADD_SELECTORS("ContextService::wakeupHandlerThreadForContextStrength");
  macedbg(1) << "Received context("<< context_name <<") with server(" << addr << ") strength value=" << value << Log::endl;
  ScopedLock sl( elasticityHandlerMutex );
  if( elasticityHandlerConds.find(handler_thread_id) != elasticityHandlerConds.end() ) {
    if( remoteContextRuntimeInfos.find(context_name) == remoteContextRuntimeInfos.end() ) {
      RemoteContextRuntimeInfo contextRuntimeInfo(context_name);
      contextRuntimeInfo.connectionStrengths[addr] = value;

      remoteContextRuntimeInfos[ context_name ] = contextRuntimeInfo;
    } else {
      remoteContextRuntimeInfos[ context_name ].connectionStrengths[addr] = value;
    }

    pthread_cond_signal( elasticityHandlerConds[handler_thread_id] );
  }
}

void ContextService::wakeupHandlerThreadForServersInfo( const uint32_t& handler_thread_id, 
    const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info ) {
  ADD_SELECTORS("ContextService::wakeupHandlerThreadForServersInfo");
  ScopedLock sl( elasticityHandlerMutex );
  if( elasticityHandlerConds.find(handler_thread_id) != elasticityHandlerConds.end() ) {
    const mace::ContextMapping& snapshot = this->getLatestContextMapping();
    if( Util::getMaceAddr() != mace::ContextMapping::getHead(snapshot) ){
      for( mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>::const_iterator iter = servers_info.begin(); iter != servers_info.end();
          iter ++ ) {
        elasticityMonitor->updateServerInfo( iter->first, iter->second );
      }
    }
    pthread_cond_signal( elasticityHandlerConds[handler_thread_id] );
  } else {
    ASSERT(false);
  }
}

void ContextService::clearRemoteContextRuntimeInfos() { 
  ScopedLock sl(elasticityHandlerMutex);
  remoteContextRuntimeInfos.clear(); 
}

void ContextService::getContextObjectsByTypes( std::vector<mace::ContextBaseClass*>& contextObjs, const mace::set<mace::string>& context_types ) {
  ScopedLock sl(getContextObjectMutex); 
  mace::hash_map< mace::string, mace::ContextBaseClass*, mace::SoftState >::const_iterator cpIt;
  for( cpIt = ctxobjNameMap.begin(); cpIt != ctxobjNameMap.end(); cpIt ++ ){
    mace::string context_type = Util::extractContextType( cpIt->first );
    if( context_type == "globalContext" || context_type == "externalCommContext") {
      ((cpIt->second)->runtimeInfo).clear();
      continue;
    }
    if( context_types.size() == 0 || context_types.count(context_type) > 0 ) {
      contextObjs.push_back( cpIt->second );
    }
  } 
}

void ContextService::colocateContexts( const mace::string& ctx_name1, const mace::string& ctx_name2 ) {
  const mace::ContextMapping& snapshot = this->getLatestContextMapping();
  mace::MaceAddr dest = mace::ContextMapping::getNodeByContext(snapshot, ctx_name2);
  elasticityMonitor->requestContextMigration( ctx_name1, dest );
}

mace::set<uint32_t> ContextService::getLocalContextIDs( const mace::string& context_type ) {
  mace::set<uint32_t> ctx_ids;

  ScopedLock sl(getContextObjectMutex); 
  mace::hash_map< mace::string, mace::ContextBaseClass*, mace::SoftState >::const_iterator cpIt;
  for( cpIt = ctxobjNameMap.begin(); cpIt != ctxobjNameMap.end(); cpIt ++ ){
    mace::string ctx_name = cpIt->first;
    if( Util::isContextType(ctx_name, context_type) ){
      uint32_t ctx_id = Util::extractContextID( ctx_name );
      ctx_ids.insert(ctx_id);
    }
  }

  return ctx_ids;
}

void ContextService::waitForServerInformationUpdate(const uint32_t handler_thread_id, const mace::ServerRuntimeInfo& server_info) {
  ScopedLock sl(elasticityHandlerMutex);

  this->send__elasticity_informationReport(server_info);
  pthread_cond_t cond;
  pthread_cond_init( &cond, NULL );
  elasticityHandlerConds[ handler_thread_id ] = &cond;
  pthread_cond_wait( &cond, &elasticityHandlerMutex );
  pthread_cond_destroy( &cond );
  elasticityHandlerConds[ handler_thread_id ] = NULL;
  elasticityHandlerConds.erase( handler_thread_id );
}

void ContextService::handleServerInformationReport( mace::MaceAddr const& src, const mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>& servers_info) {
  ADD_SELECTORS("ContextService::handleServerInformationReport");
  macedbg(1) << "Recv server information report from " << src << Log::endl;
  ASSERT( servers_info.size() > 0 );
  mace::map<mace::MaceAddr, mace::ServerRuntimeInfo>::const_iterator iter = servers_info.begin();
  elasticityMonitor->setIsGEM(true);
  elasticityMonitor->updateServerInfo( iter->first, iter->second );
}

void ContextService::markStartTimestamp(const mace::string& marker) {
  mace::ContextBaseClass* ctxObj = ThreadStructure::myContext();
  ASSERT( ctxObj != NULL);
  ctxObj->markStartTimestamp(marker);
} 

void ContextService::markEndTimestamp( const mace::string& marker ) {
  mace::ContextBaseClass* ctxObj = ThreadStructure::myContext();
  ASSERT( ctxObj != NULL);
  ctxObj->markEndTimestamp( marker );
} 

double ContextService::getMarkerAvgLatency( const mace::string& marker ) {
  mace::ContextBaseClass* ctxObj = ThreadStructure::myContext();
  ASSERT( ctxObj != NULL);
  return ctxObj->getMarkerAvgLatency( marker );
} 

void ContextService::inactivateContext( const mace::string& context_name) {
  const mace::ContextMapping& snapshot = this->getLatestContextMapping();

  mace::MaceAddr addr;
  uint64_t value = 0;
  mace::vector<mace::string> ctx_names;
  mace::map< mace::MaceAddr, mace::ServerRuntimeInfo > servers_info;
  mace::vector< mace::MigrationContextInfo> m_contexts_info;

  mace::InternalMessageID msgId( Util::getMaceAddr(), "", 0 );

  mace::InternalMessage msg( mace::ElasticityControl, msgId, mace::ElasticityControl_Message::INACTIVATE_CONTEXT, 
    0, context_name, addr, value, ctx_names, servers_info, m_contexts_info);
  forwardInternalMessage( mace::ContextMapping::getHead(snapshot), msg);
}

void ContextService::activateContext( const mace::string& context_name) {
  const mace::ContextMapping& snapshot = this->getLatestContextMapping();

  mace::MaceAddr addr;
  uint64_t value = 0;
  mace::vector<mace::string> ctx_names;
  mace::map< mace::MaceAddr, mace::ServerRuntimeInfo > servers_info;
  mace::vector< mace::MigrationContextInfo> m_contexts_info;

  mace::InternalMessageID msgId( Util::getMaceAddr(), "", 0 );

  mace::InternalMessage msg( mace::ElasticityControl, msgId, mace::ElasticityControl_Message::ACTIVATE_CONTEXT, 
    0, context_name, addr, value, ctx_names, servers_info, m_contexts_info);
  forwardInternalMessage( mace::ContextMapping::getHead(snapshot), msg);
}

// Migration methods
void ContextService::send__migration_contextMigrationRequest( const mace::string& context_name, const mace::MaceAddr& dest_node ) const {
  const mace::ContextMapping& snapshot = this->getLatestContextMapping();
  uint32_t context_id = mace::ContextMapping::hasContext2( snapshot, context_name );
  ASSERT( context_id > 0 );
  mace::MaceAddr dest = mace::ContextMapping::getNodeByContext(snapshot, context_name);
  mace::map<uint32_t, mace::string> migrate_contexts;
  migrate_contexts[context_id] = context_name;

  uint64_t ticket = 0;

  mace::InternalMessage msg( mace::MigrationControl, mace::MigrationControl_Message::MIGRATION_CTX_REQUEST, ticket, migrate_contexts, 
    snapshot, dest_node );
  forwardInternalMessage(dest, msg);
}

void ContextService::send__event_MigrationControlMsg(mace::MaceAddr const& dest, const uint8_t control_type, const uint64_t ticket, 
    mace::map<uint32_t, mace::string> const& migrate_contexts, mace::ContextMapping const& ctxMapping) const {
  ADD_SELECTORS("ContextService::send__event_MigrationControlMsg");
  macedbg(1) << "Ticket = " << ticket << Log::endl;
  mace::MaceAddr srcAddr;
  mace::InternalMessage msg( mace::MigrationControl, control_type, ticket, migrate_contexts, ctxMapping, srcAddr );
  forwardInternalMessage(dest, msg);
}

void ContextService::send__ContextMigration_UpdateContextMapping( mace::MaceAddr const& dest, 
    mace::map<uint32_t, mace::string> const& migrate_contexts, mace::ContextMapping const& ctxMapping, mace::MaceAddr const& srcAddr ) const {

  mace::InternalMessage msg( mace::MigrationControl, mace::MigrationControl_Message::MIGRATION_UPDATE_CONTEXTMAPPING, 0, migrate_contexts, 
    ctxMapping, srcAddr );
  forwardInternalMessage(dest, msg);
}

void ContextService::send__ContextMigration_Done( mace::MaceAddr const& dest, 
    mace::map<uint32_t, mace::string> const& migrate_contexts, mace::ContextMapping const& ctxMapping ) const {


  mace::InternalMessage msg( mace::MigrationControl, mace::MigrationControl_Message::MIGRATION_DONE, 0, migrate_contexts, 
    ctxMapping, Util::getMaceAddr() );
  forwardInternalMessage(dest, msg);
}

void ContextService::send__event_ContextMigrationRequest( MaceAddr const& destNode, MaceAddr const& dest, mace::Event const& event, 
    uint64_t const& prevContextMapVersion, mace::set< uint32_t > const& migrateContextIds, mace::ContextMapping const& ctxMapping ){
  mace::InternalMessage msg( mace::ContextMigrationRequest, dest, event, prevContextMapVersion, migrateContextIds, ctxMapping );
  forwardInternalMessage( destNode, msg );
}

void ContextService::send__event_TransferContext( MaceAddr const& dest, const mace::ContextBaseClassParams* ctxParams, 
    mace::string const& checkpoint, mace::OrderID const eventId ){
  mace::InternalMessage msg( mace::TransferContext, ctxParams, checkpoint, eventId );
  forwardInternalMessage( dest, msg );
}

void ContextService::send__commit_context_migration( mace::OrderID const& eventId, mace::string const& contextName, mace::MaceAddr const& srcAddr, 
    mace::MaceAddr const& destAddr ) const {

  const mace::ContextMapping& snapshot = this->getLatestContextMapping();

  mace::InternalMessageID msgId( Util::getMaceAddr(), "", 0);
  mace::InternalMessage msg(mace::CommitContextMigration, msgId, eventId, contextName, srcAddr, destAddr );
  forwardInternalMessage( mace::ContextMapping::getHead(snapshot) , msg);
}

void ContextService::handle__event_MigrationControl( const mace::MigrationControl_Message* msg) {
  ADD_SELECTORS("ContextService::handle__event_MigrationControl");
  uint8_t type = msg->control_type;
  macedbg(1) << "msg type = " << (uint32_t)type << Log::endl;
  if( type == mace::MigrationControl_Message::MIGRATION_DONE ){
    const mace::ContextMapping& ctxMapping =  msg->ctxMapping;
    if( contextMapping.getCurrentVersion() < ctxMapping.getCurrentVersion() && Util::getMaceAddr() != contextMapping.getHead() ) {
      macedbg(1) << "Update context mapping to " << ctxMapping.getCurrentVersion() << Log::endl;
      contextMapping.updateToNewMapping(ctxMapping);
    }

    mace::set<mace::string> migrateContexts;
    const mace::map<uint32_t, mace::string>& contexts = msg->migrate_contexts;
    mace::map<uint32_t, mace::string>::const_iterator cIter = contexts.begin();
    for(; cIter != contexts.end(); cIter++) {
      macedbg(1) << "Migration of context("<< cIter->second <<") is done!" << Log::endl;
      migrateContexts.insert(cIter->second);
    }

    releaseBlockedMessageForMigration(migrateContexts);
    elasticityMonitor->wrapupCurrentContextMigration();
  } else if (type == mace::MigrationControl_Message::MIGRATION_PREPARE_RECV_CONTEXTS ) {
    macedbg(1) << "Migrating event ticket = " << msg->ticket << Log::endl;
    const mace::map<uint32_t, mace::string>& migrate_contexts = msg->migrate_contexts;
    mace::map<uint32_t, mace::string>::const_iterator ctxIter = migrate_contexts.begin();
    ScopedLock sl(migratingContextMutex);
    receivedExternalMsgCount += 2;
    for(; ctxIter != migrate_contexts.end(); ctxIter++) {
      commingContexts.insert(ctxIter->second);
      commingContextsMap[ctxIter->first] = ctxIter->second;
      macedbg(1) << "Context("<< ctxIter->first<<", "<< ctxIter->second <<") is on the way!" << Log::endl;
    }

    isContextComming = true;

    send__event_MigrationControlMsg(contextMapping.getHead(), mace::MigrationControl_Message::MIGRATION_PREPARE_RECV_CONTEXTS_RESPONSE,
      msg->ticket, msg->migrate_contexts, msg->ctxMapping);
    receivedExternalMsgCount -= 2;
  } else if ( type == mace::MigrationControl_Message::MIGRATION_PREPARE_RECV_CONTEXTS_RESPONSE ) {
    HeadEventDispatch::HeadEventTP::signalMigratingContextThread(msg->ticket);
  } else if ( type == mace::MigrationControl_Message::MIGRATION_UPDATE_CONTEXTMAPPING) {
    if( Util::getMaceAddr() != contextMapping.getHead() && contextMapping.getCurrentVersion() < msg->ctxMapping.getCurrentVersion() ) {
      contextMapping.updateToNewMapping(msg->ctxMapping);
      macedbg(1) << "Update ContextMapping version to " << contextMapping.getCurrentVersion() << Log::endl;
    }
    this->send__ContextMigration_Done( msg->srcAddr, msg->migrate_contexts, msg->ctxMapping );

  } else if( type == mace::MigrationControl_Message::MIGRATION_RELEASE_CONTEXTMAPPING ){ 
    ASSERT(Util::getMaceAddr() == contextMapping.getHead());
    HeadEventDispatch::HeadEventTP::signalMigratingContextThread(msg->ticket);
  } else if( type == mace::MigrationControl_Message::MIGRATION_CTX_REQUEST ){ 
    const mace::map<uint32_t, mace::string>& migrate_contexts = msg->migrate_contexts;
    mace::map<uint32_t, mace::string>::const_iterator ctxIter = migrate_contexts.begin();
    for(; ctxIter != migrate_contexts.end(); ctxIter++) {
      //this->handleContextMigrationRequest( msg->srcAddr, ctxIter->second, ctxIter->first );
      elasticityMonitor->requestContextMigration( ctxIter->second, msg->srcAddr );
    }
  } else {
    ASSERTMSG(false, "Unkonw message type!");
  }
}

void ContextService::handleContextMigrationRequest( MaceAddr const& dest, mace::string const& contextName, 
    const uint32_t& contextId ){
  ADD_SELECTORS("ContextService::handleContextMigrationRequest");
  macedbg(1) << "To migrate context("<< contextName <<") to node("<< dest <<")." << Log::endl;
  const mace::ContextMapping& snapshot = this->getLatestContextMapping();
  if( mace::ContextMapping::getNodeByContext(snapshot, contextName) != Util::getMaceAddr() ) {
    macedbg(1) << "Context("<< contextName <<") is not on this server("<< Util::getMaceAddr() <<")!" << Log::endl;
    this->send__migration_contextMigrationRequest( contextName, dest );
    return;
  }

  mace::Event mEvent;
  mEvent.newEventID( mace::Event::MIGRATIONEVENT );
  
  mEvent.initialize3( contextName, contextMapping.getCurrentVersion()  );
  mEvent.addServiceID( instanceUniqueID );

  mace::ContextBaseClass *thisContext = getContextObjByName( contextName ); // assuming context object already exists and this operation does not create new object.
  ASSERT(thisContext != NULL);

  mEvent.eventId = thisContext->newCreateTicket();
  thisContext->markMigrationTicket( mEvent.eventId.ticket );
  ThreadStructure::setEvent( mEvent );

  ASSERT( thisContext->requireExecuteTicket(mEvent.eventId) > 0 );
  thisContext->setMigratingFlag(true);
  mace::ContextLock ctxlock( *thisContext, mEvent.eventId, false, mace::ContextLock::MIGRATION_MODE );
  this->addMigratingContextName( contextName );
  
  macedbg(1) << "Prepare to halt!!" << Log::endl;
  thisContext->prepareHalt();

  macedbg(1) << "Now Migration Event("<< mEvent.eventId <<") could migrate Context("<< thisContext->contextName<<")!" << Log::endl;
  mace::string contextData;
  copyContextData( thisContext, contextData );
  mace::ContextBaseClassParams* ctxParams = new mace::ContextBaseClassParams();
  ctxParams->initialize(thisContext);

  eraseContextData( thisContext );
  this->send__event_TransferContext( dest, ctxParams, contextData, mEvent.eventId );
}

void ContextService::handle__event_TransferContext( MaceAddr const& src, const mace::ContextBaseClassParams* ctxParams, 
      mace::string const& checkpoint, mace::OrderID const& eventId ){

  ADD_SELECTORS("ContextService::handle__event_TransferContext");
  macedbg(1) << "Received a context from " << src << Log::endl;

  const mace::string& contextName = ctxParams->contextName;
  const uint32_t& contextId = ctxParams->contextId;

  // getUpdatedContextStructure(0);
  mace::ContextBaseClass* thisContext = createContextObjectWrapper( eventId, contextName , contextId, 
    contextMapping.getLatestMappingVersion(), true );
  ASSERT( thisContext != NULL );

  const mace::vector<mace::string> dominateContexts = contextStructure.getDominateContexts(thisContext->contextName);
  mace::string dominator = contextStructure.getUpperBoundContextName(thisContext->contextName);
  uint64_t ver = contextStructure.getDAGNodeVersion(thisContext->contextName);
  ASSERT(ver > 0);
  thisContext->initializeDominator( thisContext->contextName, dominator, ver, dominateContexts);
      
  ThreadStructure::ScopedServiceInstance si( instanceUniqueID );
  mace::Event& myEvent = ThreadStructure::myEvent();
  myEvent.eventType = mace::Event::MIGRATIONEVENT;
  myEvent.eventId = eventId;
  myEvent.target_ctx_name = contextName;
  myEvent.create_ctx_name = contextName;
  myEvent.createCommitFlag = false;
    
  // create object using name string
  mace::deserialize( checkpoint, thisContext );
  thisContext->resumeParams(this, ctxParams);
  ASSERT( thisContext->getNowServing() == eventId );
  thisContext->setMigratingFlag(false);
  macedbg(1) << "Migrating context(" << contextName <<") with eventId("<< myEvent.eventId <<") is done!" << Log::endl;
  // local commit.
  // notice that the same event has also already downgraded the original context object copy.
  mace::ContextLock c_lock( *thisContext, eventId, false, mace::ContextLock::MIGRATION_RELEASE_MODE );
  thisContext->commitEvent(ThreadStructure::myEventID());
  thisContext->resumeExecution();
  myEvent.clearEventRequests();
  myEvent.clearEventUpcalls();
  send__commit_context_migration( eventId, contextName, src, Util::getMaceAddr() );
  // TODO: send response
}

void ContextService::addMigratingContextName( mace::string const& ctx_name ) {
  ADD_SELECTORS("ContextService::addMigratingContextName");
  macedbg(1) << "Try to add context: " << ctx_name <<" count=" << receivedExternalMsgCount << Log::endl;
  ScopedLock sl(migratingContextMutex);
  receivedExternalMsgCount += 4;
  if( migratingContextNames.count(ctx_name) == 0 ) {
    migratingContextNames.insert(ctx_name);
    macedbg(1) << "Adding migrating context " << ctx_name << Log::endl;
  }
  isContextMigrating = true;
  receivedExternalMsgCount -= 4;
}

void ContextService::releaseBlockedMessageForMigration( mace::set<mace::string> const& migrate_contexts) {
  ADD_SELECTORS("ContextService::releaseBlockedMessageForMigration");
  macedbg(1) << "Migrating contexts=" << migrate_contexts << Log::endl;
  ScopedLock sl(migratingContextMutex);
  receivedExternalMsgCount += 3;

  const mace::ContextMapping& ctxMapping = contextMapping.getLatestContextMapping();
  mace::set<mace::string>::const_iterator iter = migrate_contexts.begin();
  for(; iter != migrate_contexts.end(); iter++) {
    const mace::string& ctxName = *iter;
    migratingContextNames.erase(*iter);
    std::map<mace::string, std::set<mace::InternalMessage*> >::iterator mIter = holdingMessageForMigration.find(*iter);
    if( mIter != holdingMessageForMigration.end()) {

      const MaceAddr& dest = mace::ContextMapping::getNodeByContext(ctxMapping, *iter);
      std::set<mace::InternalMessage*>& msgs = mIter->second;
      macedbg(1) << "Migrating context("<< mIter->first<<") has " << msgs.size() << " blocking messages!" << Log::endl;
      std::set<mace::InternalMessage*>::const_iterator msg_iter = msgs.begin();
      for(; msg_iter != msgs.end(); msg_iter ++) {
        forwardInternalMessage(dest, *(*msg_iter));
      }
      holdingMessageForMigration.erase(mIter);
    } else {
      macedbg(1) << "Migrating context("<< *iter <<") has no blocking messages!" << Log::endl;
    }

    mIter = holdingMessageForCommingContexts.find(*iter);
    commingContexts.erase(ctxName);
    macedbg(1) << "Delete incomming Context " << ctxName << Log::endl;

    uint32_t ctxId = mace::ContextMapping::hasContext2(ctxMapping, ctxName);
    commingContextsMap.erase(ctxId);

    if( mIter != holdingMessageForCommingContexts.end() ) {
      const MaceAddr& dest = mace::ContextMapping::getNodeByContext(ctxMapping, *iter);
      std::set<mace::InternalMessage*>& msgs = mIter->second;
      macedbg(1) << "Incomming context("<< mIter->first<<") has " << msgs.size() << " blocking messages!" << Log::endl;
      
      std::set<mace::InternalMessage*>::const_iterator msg_iter = msgs.begin();
      for(; msg_iter != msgs.end(); msg_iter ++) {
        forwardInternalMessage(dest, *(*msg_iter));
      }
      holdingMessageForCommingContexts.erase(mIter);
    } else {
      macedbg(1) << "Incomig context("<< *iter <<") has no blocking messages!" << Log::endl;
    }
  }

  if( commingContexts.size() == 0) {
    isContextComming = false;
  }

  if( migratingContextNames.size() == 0) {
    isContextMigrating = false;
  }
  macedbg(1) << "Done unblock messages!" << Log::endl;

  receivedExternalMsgCount -= 3;
}

bool ContextService::checkMigratingContext(mace::InternalMessage const& message) {
  ADD_SELECTORS("ContextService::checkMigratingContext");
  
  const mace::string& ctxName = message.getTargetContextName();
  if( migratingContextNames.count(ctxName) ) {
    mace::InternalMessage* mptr = new mace::InternalMessage(message);
    message.unlinkHelper();

    holdingMessageForMigration[ctxName].insert(mptr);
    macedbg(1) << "Keep a message for migrating context "<< ctxName << Log::endl;
    return true;
  }
  return false;
}
bool ContextService::checkCommingContext(mace::InternalMessage const& message) {
  ADD_SELECTORS("ContextService::checkCommingContext");
  
  const mace::string& ctxName = message.getTargetContextName();
  if( commingContexts.count(ctxName) ) {
    mace::InternalMessage* mptr = new mace::InternalMessage( message );
    message.unlinkHelper();

    holdingMessageForCommingContexts[ctxName].insert(mptr);
    macedbg(1) << "Keep a message("<< (void*)mptr <<") for incomming context "<< ctxName << Log::endl;
    return true;
  }
  return false;
}

