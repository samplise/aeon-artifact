#ifndef __CONTROL_MSG_CHANNEL_h
#define __CONTROL_MSG_CHANNEL_h

#include "mace.h"
#include "InternalMessage.h"
#include "pthread.h"
#include "ThreadPool.h"

class EventExecutionControlMessageChannel {
private:
	AsyncEventReceiver* sv;
	
	std::map<mace::string, std::vector<mace::InternalMessage*> > controlMessages;
	mace::map<mace::string, bool> isLocking;
	mace::vector<mace::string> contextNames;
	uint32_t nextContextIter;

	pthread_mutex_t channelMutex;
	pthread_cond_t channelCond;

	pthread_t* threadKeys;

public:
	EventExecutionControlMessageChannel( AsyncEventReceiver* sv, uint32_t nThread );
	~EventExecutionControlMessageChannel() { }

public:
	void enqueueControlMessage( const mace::string& ctx_name, const mace::InternalMessage& msg );

	static void* startChannelThread( void* arg );
	void processNextControlMessage();
	void handleControlMessageReply( const mace::string& ctx_name );
	mace::InternalMessage* getNextPendingMessage( mace::string& src_context );
	void labelContextChannelIsLocking( const mace::string& ctx_name, const bool flag );
	void signalThread();
};

#endif