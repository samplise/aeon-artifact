#ifndef __CONTEXT_DISPATCH_h
#define __CONTEXT_DISPATCH_h

#include "ContextBaseClass.h"
#include "Message.h"
#include "ThreadPool.h"
/**
 * \file ContextDispatch.h
 * \brief declares the ContextEventTP class
 */

namespace mace{

/**
 * thread pool for execution in the context
 *
 * uses ThreadPool class to manage the threads
 * */
struct ContextThreadArg {
  ContextEventTP* p;
  uint32_t i;
};

class ContextEventTP {
public:
    typedef mace::ThreadPool<ContextEventTP, mace::ContextEvent> ExecuteThreadPoolType;
    typedef mace::ThreadPool<ContextEventTP, HeadEventDispatch::HeadEvent> CreateThreadPoolType;
    typedef mace::ThreadPool<ContextEventTP, mace::ContextCommitEvent> CommitThreadPoolType;
private:
    //ContextBaseClass* context;
    ExecuteThreadPoolType *etpptr;
    CreateThreadPoolType *ctpptr;
    
    bool nextCommitCreateEvent;
    pthread_t contextCommitThread;
    pthread_cond_t signalc;
    mace::ContextCommitEvent* committingEvent;
    bool busyCommit;

    bool skipFlag;


    bool halting;
    bool halted;
    pthread_mutex_t commitExitMutex;
    pthread_cond_t commitExitCond;

    bool runDeliverExecuteCondition(ExecuteThreadPoolType* tp, uint threadId);
    void runDeliverExecuteSetup(ExecuteThreadPoolType* tp, uint threadId);
    void runDeliverExecuteProcessUnlocked(ExecuteThreadPoolType* tp, uint threadId);
    void runDeliverExecuteProcessFinish(ExecuteThreadPoolType* tp, uint threadId);

    bool runDeliverCreateCondition(CreateThreadPoolType* tp, uint threadId);
    void runDeliverCreateSetup(CreateThreadPoolType* tp, uint threadId);
    void runDeliverCreateProcessUnlocked(CreateThreadPoolType* tp, uint threadId);
    void runDeliverCreateProcessFinish(CreateThreadPoolType* tp, uint threadId);

  public:
    /**
     * constructor
     *
     * @param context the corresponding context object
     * @param minThreadSize the minimum size of thread pool
     * @param maxThreadSize the max size of the thread pool
     * */
    //ContextEventTP(ContextBaseClass* context, uint32_t minThreadSize, uint32_t maxThreadSize  );
    /// destructor
    ~ContextEventTP();

    bool isBusyCommit() {
        return this->busyCommit;
    }

    /// signal the thread in the pool
    void signalExecuteThread(){
      if (etpptr != NULL) {
        etpptr->signalSingle();
      }
    }

    void signalCreateThread();

    /// signal and wait for the thread to terminate
    void haltAndWaitExecute();
    void haltAndWaitCreate();
    void haltAndWaitCommit();
    /// lock 
    void lock(){
      //ASSERT(pthread_mutex_lock(&context->_context_ticketbooth) == 0);
    } // lock
 
    /// unlock 
    void unlock(){
      //ASSERT(pthread_mutex_unlock(&context->_context_ticketbooth) == 0);
    } // unlock

	static void* startCommitThread(void* arg);
	void runCommit();
	void commitEventProcess();
	void commitEventFinish();
	bool hasUncommittedEvents();
	void signalCommitThread();
  void commitWait();

  // New implementation for shared threadpool
private:
  mace::deque<mace::ContextEvent> readyExecuteEventQueue;
  std::deque<HeadEventDispatch::HeadEvent> readyCreateEventQueue;
  mace::deque<mace::ContextCommitEvent> readyCommitEventQueue;
  CommitThreadPoolType *mtpptr;

  pthread_mutex_t readyExecuteEventQueueMutex;
  pthread_mutex_t readyCreateEventQueueMutex;
  pthread_mutex_t readyCommitEventQueueMutex;

  bool runExecuteCondition(ExecuteThreadPoolType* tp, uint threadId);
  void runExecuteSetup(ExecuteThreadPoolType* tp, uint threadId) { }
  void runExecuteProcessUnlocked(ExecuteThreadPoolType* tp, uint threadId);
  void runExecuteProcessFinish(ExecuteThreadPoolType* tp, uint threadId) { }

  bool runCreateCondition(CreateThreadPoolType* tp, uint threadId);
  void runCreateSetup(CreateThreadPoolType* tp, uint threadId) { }
  void runCreateProcessUnlocked(CreateThreadPoolType* tp, uint threadId);
  void runCreateProcessFinish(CreateThreadPoolType* tp, uint threadId) { }

  bool runCommitCondition(CommitThreadPoolType* tp, uint threadId);
  void runCommitSetup(CommitThreadPoolType* tp, uint threadId) { }
  void runCommitProcessUnlocked(CommitThreadPoolType* tp, uint threadId);
  void runCommitProcessFinish(CommitThreadPoolType* tp, uint threadId);


public:
  ContextEventTP( uint32_t minThreadSize, uint32_t maxThreadSize  );
  void enqueueReadyCreateEvent( const HeadEventDispatch::HeadEvent& cEvent );
  void enqueueReadyExecuteEvent( const mace::ContextEvent& eEvent );
  void enqueueReadyCommitEvent( const mace::ContextCommitEvent& event );

  void signalSharedCreateThread();
  void signalSharedExecuteThread();
  void signalSharedCommitThread();
};

}
#endif

