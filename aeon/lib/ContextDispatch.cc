#include "ContextDispatch.h"
#include "ContextService.h"
#include "InternalMessage.h"

bool mace::ContextEventTP::runDeliverExecuteCondition(ExecuteThreadPoolType* tp, uint threadId) {
	ADD_SELECTORS("ContextEventTP::runDeliverExecuteCondition");
  return false;
}

void mace::ContextEventTP::runDeliverExecuteSetup(ExecuteThreadPoolType* tp, uint threadId) {
}
void mace::ContextEventTP::runDeliverExecuteProcessUnlocked(ExecuteThreadPoolType* tp, uint threadId) {
	ADD_SELECTORS("ContextEventTP::runDeliverExecuteProcessUnlocked");
	if( !skipFlag ) {
		ContextEvent& ce = tp->data(threadId);
		ce.fire();
	}
	//macedbg(1) << "Launch one event in Context " << context->contextName << Log::endl;
}
void mace::ContextEventTP::runDeliverExecuteProcessFinish(ExecuteThreadPoolType* tp, uint threadId){
	
}

bool mace::ContextEventTP::runDeliverCreateCondition(CreateThreadPoolType* tp, uint threadId) {
	ADD_SELECTORS("ContextEventTP::runDeliverCreateCondition");
	return true;
} 


void mace::ContextEventTP::runDeliverCreateProcessUnlocked(CreateThreadPoolType* tp, uint threadId) {
	ADD_SELECTORS("ContextEventTP::runDeliverCreateProcessUnlocked");
}


void mace::ContextEventTP::runDeliverCreateSetup(CreateThreadPoolType* tp, uint threadId) {

}

void mace::ContextEventTP::runDeliverCreateProcessFinish(CreateThreadPoolType* tp, uint threadId) {

}

mace::ContextEventTP::~ContextEventTP() {
	ADD_SELECTORS("ContextEventTP::destory");
	haltAndWaitCreate();
	macedbg(1) << "Stop creating threads!" << Log::endl;
  	haltAndWaitExecute();
  	macedbg(1) << "Stop executing threads!" << Log::endl;
  	haltAndWaitCommit();
  	macedbg(1) << "Stop committing threads!" << Log::endl;
  	CreateThreadPoolType *ctp = ctpptr;
  	ExecuteThreadPoolType *etp = etpptr;
  	ctpptr = NULL;
  	etpptr = NULL;
  	delete ctp;
  	delete etp;  
  	
  	pthread_mutex_destroy(&commitExitMutex);
}

void mace::ContextEventTP::signalCreateThread() {
	ADD_SELECTORS("ContextEventTP::signalCreateThread");
  	if( ctpptr != NULL ) {
  		macedbg(1) << "signal() called - just one thread." << Log::endl;
		ctpptr->signalSingle();
  	}
}

void mace::ContextEventTP::haltAndWaitExecute() {
  ASSERTMSG(etpptr != NULL, "Please submit a bug report describing how this happened.  If you can submit a stack trace that would be preferable.");
  etpptr->halt();
  etpptr->waitForEmptySignal();
}

void mace::ContextEventTP::haltAndWaitCreate() {
  ASSERTMSG(ctpptr != NULL, "Please submit a bug report describing how this happened.  If you can submit a stack trace that would be preferable.");
  ctpptr->halt();
  ctpptr->waitForEmptySignal();
}

void* mace::ContextEventTP::startCommitThread(void* arg) {
	ContextEventTP* t = (ContextEventTP*)(arg);
	t->runCommit();
	return 0;
}

void mace::ContextEventTP::runCommit() {
	ADD_SELECTORS("ContextEventTP::runCommit");
	//ScopedLock sl(context->commitQueueMutex);
	while( !halting ) {
		if( !hasUncommittedEvents() ){
			busyCommit = false;
			commitWait();
			//macedbg(1) << "Wake up to commit event in "<< context->contextName << Log::endl;
			continue;
		}
		busyCommit = true;
		
		//sl.unlock();
		commitEventProcess();
		commitEventFinish();

		//sl.lock();
	}

	//sl.unlock();
	ScopedLock exit_sl(commitExitMutex);
	macedbg(1) << "Prepare to stop commit threads" << Log::endl;
	ASSERT(pthread_cond_destroy(&signalc) == 0);
	halted = true;
	pthread_cond_signal(&commitExitCond);
}

bool mace::ContextEventTP::hasUncommittedEvents() {
	ADD_SELECTORS("ContextEventTP::hasUncommittedEvents");
	return true;
}

void mace::ContextEventTP::commitEventProcess() {
	ADD_SELECTORS("ContextEventTP::commitEventProcess");
}

void mace::ContextEventTP::commitEventFinish() {
	ADD_SELECTORS("ContextEventTP::commitEventFinish");
}

void mace::ContextEventTP::signalCommitThread() {
	ADD_SELECTORS("ContextEventTP::signalCommitThread");
	//macedbg(1) << "Signal thread to handle commit event!" << Log::endl;
	pthread_cond_signal(&signalc);
}

void mace::ContextEventTP::commitWait() {
	ADD_SELECTORS("ContextEventTP::commitWait");
	//ASSERT(pthread_cond_wait(&signalc, &context->commitQueueMutex) == 0);
}

void mace::ContextEventTP::haltAndWaitCommit() {
	//ScopedLock sl(context->commitQueueMutex);
	ScopedLock sl_exit(commitExitMutex);
    halting = true;
    //sl.unlock();
    if( !isBusyCommit() ) signalCommitThread();
      

    pthread_cond_wait( &commitExitCond, &commitExitMutex );
    //pthread_cond_wait( &commitExitCond, &(context->commitQueueMutex) );
    pthread_cond_destroy( &commitExitCond );
}


// New implementation for shared threadpool
mace::ContextEventTP::ContextEventTP( uint32_t minThreadSize, uint32_t maxThreadSize ) :
		etpptr (new ExecuteThreadPoolType(*this, &mace::ContextEventTP::runExecuteCondition, &mace::ContextEventTP::runExecuteProcessUnlocked,
  			NULL, NULL, ThreadStructure::ASYNC_THREAD_TYPE, minThreadSize, maxThreadSize) ),
  		ctpptr (new CreateThreadPoolType(*this, &mace::ContextEventTP::runCreateCondition, &mace::ContextEventTP::runCreateProcessUnlocked,
  			NULL, NULL, ThreadStructure::ASYNC_THREAD_TYPE, minThreadSize, maxThreadSize) ),
  		mtpptr (new CommitThreadPoolType(*this, &mace::ContextEventTP::runCommitCondition, &mace::ContextEventTP::runCommitProcessUnlocked,
  			NULL, &mace::ContextEventTP::runCommitProcessFinish, ThreadStructure::ASYNC_THREAD_TYPE, minThreadSize, maxThreadSize) )	 {

	pthread_mutex_init(&readyExecuteEventQueueMutex, NULL);
	pthread_mutex_init(&readyCreateEventQueueMutex, NULL);
	pthread_mutex_init(&readyCommitEventQueueMutex, NULL);
}

bool mace::ContextEventTP::runCreateCondition(CreateThreadPoolType* tp, uint threadId) {
	ADD_SELECTORS("ContextEventTP::runCreateCondition");
	ScopedLock sl(readyCreateEventQueueMutex);
	if( readyCreateEventQueue.empty() ) {
		return false;
	}

	HeadEventDispatch::HeadEvent& top = readyCreateEventQueue.front();
	tp->data(threadId) = top;
	top.param = NULL;
	readyCreateEventQueue.pop_front();
	ASSERT( tp->data(threadId).param != NULL );
	return true;
} 


void mace::ContextEventTP::runCreateProcessUnlocked(CreateThreadPoolType* tp, uint threadId) {
	ADD_SELECTORS("ContextEventTP::runCreateProcessUnlocked");
	mace::AsyncEvent_Message* message = static_cast<AsyncEvent_Message*>(tp->data(threadId).param);
	
	ASSERT(message != NULL);
		
	ContextService* _service = static_cast<ContextService*>( tp->data(threadId).cl );
	ContextBaseClass* context = _service->getContextObjByName( tp->data(threadId).contextName );
	const __asyncExtraField& extra = message->getExtra();
  	const mace::string& target_ctx_name = extra.targetContextID;
  	const mace::string& methodType = extra.methodName;

  	// (context->runtimeInfo).addToEventAccess(methodType, target_ctx_name);
  	
  	uint8_t event_op_type = mace::Event::EVENT_OP_WRITE;
  	if( extra.lockingType == 0 ) {
  		event_op_type = mace::Event::EVENT_OP_READ;
  	} else if( extra.lockingType == 1 ) {
  		event_op_type = mace::Event::EVENT_OP_WRITE;
  	} else if( extra.lockingType == 2 ) {
  		event_op_type = mace::Event::EVENT_OP_OWNERSHIP;
  	}

  	mace::Event& event = message->getEvent();
  	uint64_t ver = context->getContextMappingVersion();
  	uint64_t cver = context->getContextStructureVersion();
  	event.initialize2(tp->data(threadId).eventId, event_op_type, context->contextName, target_ctx_name, methodType, 
  		mace::Event::ASYNCEVENT, ver, cver);
  	event.addServiceID(context->serviceId);
  	
  	mace::EventOperationInfo eventInfo(event.eventId, mace::EventOperationInfo::ASYNC_OP, event.target_ctx_name, event.target_ctx_name, 0, event.eventOpType );
  	eventInfo.addAccessedContext(event.target_ctx_name);
  	event.eventOpInfo = eventInfo;

  	//macedbg(1) << "Create Event("<< eventInfo <<") in context("<< context->contextName <<")!" << Log::endl;
  	
  	uint32_t ctxId;
  	_service->asyncHead(message, ctxId);

  	ScopedLock sl(context->createEventMutex);
  	context->now_serving_create_ticket ++;
  	//macedbg(1) << "Now context("<< context->contextName<<")'s now_serving_create_ticket = " << context->now_serving_create_ticket << Log::endl;
  	context->handlingCreateEventNumber --;
  	context->enqueueReadyCreateEventQueue();

  	if( context->isWaitingForHandlingCreateEvents && context->handlingCreateEventNumber == 0 ){
  		macedbg(1) << "context("<< context->contextName <<") release waiting migrating event!" << Log::endl;
  		pthread_cond_signal( &(context->contextMigratingCond) );
  	}
  	sl.unlock();
	
	context->signalCreateThread();
}

bool mace::ContextEventTP::runExecuteCondition(ExecuteThreadPoolType* tp, uint threadId) {
	ADD_SELECTORS("ContextEventTP::runExecuteCondition");
	ScopedLock sl(readyExecuteEventQueueMutex);
	if( readyExecuteEventQueue.empty() ) {
		return false;
	}

	mace::ContextEvent& top = readyExecuteEventQueue.front();
	tp->data(threadId) = top;
	macedbg(1) << "Event("<< top.eventId <<") could execute in " << (top.contextObject)->contextName << Log::endl;
	top.param = NULL;
	readyExecuteEventQueue.pop_front();
	//sl.unlock();
	//signalSharedExecuteThread();
	return true;
}

void mace::ContextEventTP::runExecuteProcessUnlocked(ExecuteThreadPoolType* tp, uint threadId) {
	ADD_SELECTORS("ContextEventTP::runDeliverExecuteProcessUnlocked");
	ContextEvent& ce = tp->data(threadId);
	ce.fire();
}

bool mace::ContextEventTP::runCommitCondition(CommitThreadPoolType* tp, uint threadId) {
	ADD_SELECTORS("ContextEventTP::runCommitCondition");
	ScopedLock sl(readyCommitEventQueueMutex);
	if( readyCommitEventQueue.empty() ) {
		return false;
	}

	mace::ContextCommitEvent& top = readyCommitEventQueue.front();
	tp->data(threadId) = top;
	readyCommitEventQueue.pop_front();
	return true;
}

void mace::ContextEventTP::runCommitProcessUnlocked(CommitThreadPoolType* tp, uint threadId) {
	ADD_SELECTORS("ContextEventTP::runCommitProcessUnlocked");
	mace::ContextCommitEvent& committingEvent = tp->data(threadId);
	const mace::OrderID eventId = committingEvent.eventId;
	ContextBaseClass* context = committingEvent.contextObject;
	bool isAsyncEvent = committingEvent.isAsyncEvent;
	
	if( isAsyncEvent ){
		context->enqueueSubEvents(eventId);
		context->enqueueDeferredMessages(eventId);
		context->notifyExecutedContexts(eventId);
	}
	macedbg(1) << "Wrap up event commit for: " << eventId << Log::endl;
 	context->commitEvent(eventId);
  	context->enqueueReadyCommitEventQueue();
}

void mace::ContextEventTP::runCommitProcessFinish(CommitThreadPoolType* tp, uint threadId) {
	ADD_SELECTORS("ContextEventTP::runCommitProcessFinish");
}

void mace::ContextEventTP::enqueueReadyCreateEvent( const HeadEventDispatch::HeadEvent& event ) {
	ADD_SELECTORS("ContextEventTP::enqueueReadyCreateEvent");
	ScopedLock sl(readyCreateEventQueueMutex);
	macedbg(1) << "Put a new create event into the ready queue!" << Log::endl;
	readyCreateEventQueue.push_back(event);
	sl.unlock();
	signalSharedCreateThread();
}

void mace::ContextEventTP::enqueueReadyExecuteEvent( const mace::ContextEvent& event ) {
	ADD_SELECTORS("ContextEventTP::enqueueReadyExecuteEvent");
	ScopedLock sl(readyExecuteEventQueueMutex);
	// macedbg(1) << "Put a new execute event into the ready queue!" << Log::endl;
	readyExecuteEventQueue.push_back(event);
	sl.unlock();
	signalSharedExecuteThread();
}

void mace::ContextEventTP::enqueueReadyCommitEvent( const mace::ContextCommitEvent& event ) {
	ADD_SELECTORS("ContextEventTP::enqueueReadyCommitEvent");
	ScopedLock sl(readyCommitEventQueueMutex);
	macedbg(1) << "Put a new commit event into the ready queue!" << Log::endl;
	readyCommitEventQueue.push_back(event);
	sl.unlock();
	signalSharedCommitThread();
}

void mace::ContextEventTP::signalSharedExecuteThread(){
    if (etpptr != NULL) {
    	etpptr->signalSingle();
    }
}

void mace::ContextEventTP::signalSharedCreateThread(){
    if (ctpptr != NULL) {
    	ctpptr->signalSingle();
    }
}

void mace::ContextEventTP::signalSharedCommitThread(){
    if (mtpptr != NULL) {
    	mtpptr->signalSingle();
    }
}


