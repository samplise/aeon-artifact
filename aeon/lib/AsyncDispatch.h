#ifndef __ASYNC_DISPATCH_h
#define __ASYNC_DISPATCH_h

class AsyncEventReceiver;
#include "ScopedContextRPC.h"

namespace AsyncDispatch {

  typedef void (AsyncEventReceiver::*asyncfunc)(void*);

  void enqueueEvent(AsyncEventReceiver* sv, asyncfunc func, void* p);

  void init();
  void haltAndWait();

  extern uint32_t maxUncommittedEvents;
}

#endif
