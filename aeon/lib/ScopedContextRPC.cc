#include "ScopedContextRPC.h"

std::map< mace::OrderID, std::vector< mace::string > > mace::ScopedContextRPC::returnValueMapping;
std::map< mace::OrderID, std::vector< pthread_cond_t* > > mace::ScopedContextRPC::awaitingReturnMapping;
pthread_mutex_t mace::ScopedContextRPC::awaitingReturnMutex = PTHREAD_MUTEX_INITIALIZER;
uint32_t mace::ScopedContextRPC::waitTransportThreads = 0 ;
uint32_t mace::ScopedContextRPC::waitAsyncThreads = 0 ;
uint32_t mace::ScopedContextRPC::transportThreads = 0 ;
uint32_t mace::ScopedContextRPC::asyncThreads = 0 ;
