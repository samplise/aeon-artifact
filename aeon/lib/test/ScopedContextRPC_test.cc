#include <pthread.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "mace.h"
#include "ScopedContextRPC.h"
#include "ThreadStructure.h"
#include "Event.h"
#include "mstring.h"

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libmace
#include <boost/test/unit_test.hpp>
/**
 * \file ScopedContextRPC_test.cc
 * \brief unit test test cases for ScopedContextRPC class.
 * */

/**
 * \brief Use ScopedContextRPC to make sure RPC is sent * and successfully received/responded.
 * */
int local();
void* localReceiver1(void *p);
void* localReceiver2(void *p);
void* localReceiver1(void *p){
  mace::ScopedContextRPC::wakeup( 1 );

  pthread_exit(NULL);
  return NULL;
}
void* localReceiver2(void *p){
  uint32_t rv=10;
  mace::string rv_string;
  mace::serialize( rv_string, &rv );
  mace::ScopedContextRPC::wakeupWithValue( 1, rv_string );

  pthread_exit(NULL);
  return NULL;
}
void* localReceiver3(void *p){
  uint32_t rv=5566;
  mace::set< mace::string > contexts;
  contexts.insert("abc");
  contexts.insert("purduecs");

  mace::string rv_string;
  mace::serialize( rv_string, &rv );
  mace::serialize( rv_string, &contexts );
  mace::ScopedContextRPC::wakeupWithValue( 1, rv_string );

  pthread_exit(NULL);
  return NULL;
}

BOOST_AUTO_TEST_SUITE( lib_ScopedContextRPC )

pthread_t threads[ 2 ];
BOOST_AUTO_TEST_CASE( TestWakeup )
{
  {
    mace::Event he( static_cast<uint64_t>( 1 ) );
    ThreadStructure::setEvent( he );
    mace::ScopedContextRPC rpc;

    if( pthread_create( &threads[0], NULL, localReceiver1 , NULL ) != 0 ){
      perror("pthread_create");
    }
  }

  void *ret;
  pthread_join( threads[0], &ret  );
}
BOOST_AUTO_TEST_CASE( TestWakeupWithValue )
{
  if( pthread_create( &threads[0], NULL, localReceiver2 , NULL ) != 0 ){
    perror("pthread_create");
  }

  uint32_t rv; // return value
  mace::Event he( static_cast<uint64_t>( 1 ) );
  ThreadStructure::setEvent( he );
  mace::ScopedContextRPC rpc;
  rpc.get( rv ); // blcks until the RPC returns
  BOOST_REQUIRE_EQUAL( rv, (uint32_t)10 );

  void *ret;
  pthread_join( threads[0], &ret  );
}
BOOST_AUTO_TEST_CASE( TestWakeupWithMultipleValues )
{
  if( pthread_create( &threads[0], NULL, localReceiver3 , NULL ) != 0 ){
    perror("pthread_create");
  }

  uint32_t rv1; // return value
  mace::set<mace::string> rv2; // return value
  mace::Event he( static_cast<uint64_t>( 1 ) );
  ThreadStructure::setEvent( he );
  mace::ScopedContextRPC rpc;
  rpc.get( rv1 ); // blcks until the RPC returns
  rpc.get( rv2 ); // blcks until the RPC returns
  BOOST_REQUIRE_EQUAL( rv1, (uint32_t)5566 );
  mace::set<mace::string>::iterator rvit = rv2.begin();
  BOOST_REQUIRE( rvit->compare("abc") == 0  );
  rvit++;
  BOOST_REQUIRE( rvit->compare("purduecs") == 0  );

  void *ret;
  pthread_join( threads[0], &ret  );
}
BOOST_AUTO_TEST_SUITE_END()
