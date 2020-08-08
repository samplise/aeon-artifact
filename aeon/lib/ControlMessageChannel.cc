#include "ControlMessageChannel.h"
#include "ContextService.h"
#include "Util.h"

EventExecutionControlMessageChannel::EventExecutionControlMessageChannel( AsyncEventReceiver* sv, uint32_t nThread ): sv(sv) {
	threadKeys = new pthread_t[nThread];
	nextContextIter = 0;

	ASSERT( pthread_mutex_init(&channelMutex, NULL)==0 );
	ASSERT( pthread_cond_init( &channelCond, NULL ) == 0 );

	for( uint32_t i=0; i<nThread; i++ ){
		ASSERT(  pthread_create( &threadKeys[i] , NULL, EventExecutionControlMessageChannel::startChannelThread, static_cast<void*>(this) ) == 0 );
	}
}

void EventExecutionControlMessageChannel::enqueueControlMessage( const mace::string& ctx_name, const mace::InternalMessage& msg ) {
	mace::InternalMessage* mptr = new mace::InternalMessage(msg);
    msg.unlinkHelper();

    ScopedLock sl(channelMutex);
    if( isLocking.find(ctx_name) == isLocking.end() ) {
    	isLocking[ctx_name] = false;
    	contextNames.push_back(ctx_name);
    }
    controlMessages[ctx_name].push_back(mptr);
    sl.unlock();

    this->signalThread();
}

void* EventExecutionControlMessageChannel::startChannelThread(void* arg) {
	EventExecutionControlMessageChannel* channel = static_cast<EventExecutionControlMessageChannel*>(arg);
	channel->processNextControlMessage();
	return 0;
}

void EventExecutionControlMessageChannel::processNextControlMessage() {
	ADD_SELECTORS("EventExecutionControlMessageChannel::processNextControlMessage");
	ContextService* _service = static_cast<ContextService*>(sv);	

	ScopedLock sl(channelMutex);
	while( true ){
		mace::string src_context = "";
		mace::InternalMessage* msgptr = getNextPendingMessage(src_context);
		if( msgptr == NULL ){
			ASSERT(pthread_cond_wait(&channelCond, &channelMutex) == 0);
			continue;
      	}
      	ASSERT( src_context != "" );
      	sl.unlock();
		mace::string dest_context = msgptr->getTargetContextName();
		ASSERT(dest_context != "");
		const mace::ContextMapping& snapshot = _service->getLatestContextMapping();

		mace::EventExecutionControl_Message* m = static_cast<mace::EventExecutionControl_Message*>( msgptr->getHelper() );
		macedbg(1) << "Process event execution control message: " << *m << Log::endl;

		const mace::MaceAddr& destAddr = mace::ContextMapping::getNodeByContext( snapshot, dest_context );
		if( destAddr != Util::getMaceAddr() ) {
			_service->forwardInternalMessage(destAddr, *msgptr);
			labelContextChannelIsLocking( src_context, false );
			delete msgptr;
		} else {
			_service->handleInternalMessagesWrapper( static_cast<void* >(msgptr) );
			labelContextChannelIsLocking( src_context, false );
		}
		// macedbg(1) << "To process next message!" << Log::endl;

		sl.lock();
	} 
	sl.unlock();
}

mace::InternalMessage* EventExecutionControlMessageChannel::getNextPendingMessage( mace::string& src_context ) {
	ADD_SELECTORS("EventExecutionControlMessageChannel::getNextPendingMessage");
	mace::InternalMessage* mptr = NULL;
	if( contextNames.size() == 0 ) {
		return mptr;
	}

	uint32_t start_iter = nextContextIter;
	while( true ) {
		const mace::string ctx_name = contextNames[nextContextIter];
		if( !isLocking[ctx_name] && controlMessages[ctx_name].size() > 0 ) {
			mptr = controlMessages[ctx_name][0];
			src_context = ctx_name;
			controlMessages[ctx_name][0] = NULL;
			controlMessages[ctx_name].erase( controlMessages[ctx_name].begin() );
			isLocking[ctx_name] = true;
			nextContextIter = (nextContextIter+1) % contextNames.size();
			return mptr;
		}

		nextContextIter = (nextContextIter+1) % contextNames.size();
		if( nextContextIter == start_iter ) {
			break;
		}
	}
	return mptr;
}

void EventExecutionControlMessageChannel::labelContextChannelIsLocking( const mace::string& ctx_name, const bool flag ) {
	ScopedLock sl( channelMutex );
	ASSERT( isLocking.find(ctx_name) != isLocking.end() );
	isLocking[ctx_name] = flag;
}

void EventExecutionControlMessageChannel::signalThread(){
	ScopedLock sl(channelMutex);
	pthread_cond_signal(&channelCond);
}