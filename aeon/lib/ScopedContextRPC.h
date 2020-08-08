#ifndef _SCOPED_CONTEXT_RPC_H
#define _SCOPED_CONTEXT_RPC_H
/**
 * \file ScopedContextRPC.h
 * \brief declares the ScopedContextRPC class.
 */
#include <pthread.h>

#include "m_map.h"
#include "mstring.h"
#include "ThreadStructure.h"
namespace mace{
/**
 * \brief provides a scoped RPC mechanism
 *
 * In fullcontext Mace, a routine call may be initiated at a context located at 
 * remote node. Therefore RPC mechanism is required to retrieve the return value
 * of the routine call.
 *
 * Common usage:
 * \code
 * uint32_t srcContextID; // the id of the caller context
 * uint32_t rv; // return value
 * mace::ScopedContextRPC rpc(srcContextID);
 * downcall_route( destNode, msg, __ctx );
 * rpc.get( rv ); // blcks until the RPC returns
 * \endcode
 *
 * At the remote node, to respond to such a RPC:
 * \code
 * uint32_t srcContextID; 
 * uint32_t rv = 1;
 * mace::ScopedContextRPC::wakeupWithValue( srcContextID, rv );
 * \endcode
 * */
class ScopedContextRPC{
public:
  ScopedContextRPC():isReturned(false),eventId(ThreadStructure::myEventID()){
    ADD_SELECTORS("ScopedContextRPC::(constructor)");
    pthread_cond_init( &cond , NULL );
    pthread_mutex_lock(&awaitingReturnMutex);
    awaitingReturnMapping[eventId].push_back( &cond );
    macedbg(1)<<"event "<< eventId << " will be waiting for RPC return"<<Log::endl;
  }
  ~ScopedContextRPC(){
    ADD_SELECTORS("ScopedContextRPC::(destructor)");
    if( !isReturned ){
      wait();
      isReturned = true;
    }
    std::map< mace::OrderID, std::vector< mace::string > >::iterator retvalIt = returnValueMapping.find( eventId );
    if( retvalIt != returnValueMapping.end() ){
      retvalIt->second.pop_back();
      if( retvalIt->second.empty() ){
        returnValueMapping.erase( retvalIt );
      }
    }
    std::map< mace::OrderID, std::vector< pthread_cond_t* > >::iterator condIt = awaitingReturnMapping.find( eventId );
    condIt->second.pop_back();
    if( condIt->second.empty() ){
      awaitingReturnMapping.erase( condIt );
    }
    pthread_mutex_unlock(&awaitingReturnMutex);
    pthread_cond_destroy( &cond );
    macedbg(1)<<"finish rpc"<<Log::endl;
  }
  template<class T> void get(T& obj){
    ADD_SELECTORS("ScopedContextRPC::get");
    macedbg(1)<<"wait for return"<<Log::endl;
    if( !isReturned ){
      wait();
      isReturned = true;
      returnValue_iter = returnValueMapping.find( eventId );
      ASSERTMSG( returnValue_iter != returnValueMapping.end(), "Can't find the return value because event not found!" );
      in.str( returnValue_iter->second.back() );
    }
    mace::deserialize( in, &obj );
    macedbg(1)<<"returned "<< obj<<Log::endl;
  }
  static void wakeup( const mace::OrderID& eventId ){
    ADD_SELECTORS("ScopedContextRPC::wakeup");
    macedbg(1)<<"wake up event "<< eventId << " with no return value"<<Log::endl;
    pthread_mutex_lock(&awaitingReturnMutex);
    std::map< mace::OrderID, std::vector< pthread_cond_t* > >::iterator cond_iter = awaitingReturnMapping.find( eventId );
    ASSERTMSG( cond_iter != awaitingReturnMapping.end(), "Conditional variable not found" );
    ASSERTMSG( !cond_iter->second.empty(), "Conditional variable not found due to empty stack" );
    pthread_cond_signal( cond_iter->second.back() );
    pthread_mutex_unlock(&awaitingReturnMutex);
  }
  static void wakeupWithValue( const mace::OrderID& eventId, const mace::string& retValue ){
    ADD_SELECTORS("ScopedContextRPC::wakeupWithValue");
    macedbg(1)<<"wake up event "<< eventId << " with return value"<<Log::endl;
    pthread_mutex_lock(&awaitingReturnMutex);
    std::map< mace::OrderID, std::vector< pthread_cond_t* > >::iterator cond_iter = awaitingReturnMapping.find( eventId );
    ASSERTMSG( cond_iter != awaitingReturnMapping.end(), "Conditional variable not found" );
    ASSERTMSG( !cond_iter->second.empty(), "Conditional variable not found due to empty stack" );
    returnValueMapping[ eventId ].push_back( retValue );
    pthread_cond_signal( cond_iter->second.back() );
    pthread_mutex_unlock(&awaitingReturnMutex);
  }
  static void wakeupWithValue( const mace::string& retValue, mace::Event const& event ){
    ADD_SELECTORS("ScopedContextRPC::wakeupWithValue");
    macedbg(1)<<"wake up event "<< event.getEventID() << " with return value"<<Log::endl;
    mace::string event_str;
    mace::serialize( event_str, &event );
    pthread_mutex_lock(&awaitingReturnMutex);
    std::map< mace::OrderID, std::vector< pthread_cond_t* > >::iterator cond_iter = awaitingReturnMapping.find( event.getEventID() );
    ASSERTMSG( cond_iter != awaitingReturnMapping.end(), "Conditional variable not found" );
    ASSERTMSG( !cond_iter->second.empty(), "Conditional variable not found due to empty stack" );
    returnValueMapping[ event.getEventID() ].push_back( retValue );
    returnValueMapping[ event.getEventID() ].back().append( event_str );
    pthread_cond_signal( cond_iter->second.back() );
    pthread_mutex_unlock(&awaitingReturnMutex);
  }
  static void setTransportThreads(uint32_t threads ){
    transportThreads = threads;
  }
  static void setAsyncThreads(uint32_t threads ){
    asyncThreads = threads;
  }

private:
  void wait(){
    pthread_cond_wait( &cond, &awaitingReturnMutex );
  }
  bool isReturned;
  const mace::OrderID eventId;
  pthread_cond_t cond;
  std::map< mace::OrderID, std::vector< mace::string > >::iterator returnValue_iter;
  std::istringstream in;

  static std::map< mace::OrderID, std::vector< mace::string > > returnValueMapping;
  static std::map< mace::OrderID, std::vector< pthread_cond_t* > > awaitingReturnMapping;
  static pthread_mutex_t awaitingReturnMutex;
  static uint32_t waitTransportThreads;
  static uint32_t waitAsyncThreads;
  static uint32_t transportThreads;
  static uint32_t asyncThreads;
}; // ScopedContextRPC

}
#endif // _SCOPED_CONTEXT_RPC_H
