#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE libmace
#include <boost/test/unit_test.hpp>
#include "ThreadStructure.h"
#include "Event.h"
#include "ReadLine.h"
#include "ContextMapping.h"
#include "HierarchicalContextLock.h"
#include "mace.h"



BOOST_AUTO_TEST_SUITE( lib_HierarchicalContextLock )

BOOST_AUTO_TEST_CASE( Case1 )
{
/*  mace::Event he( mace::Event::STARTEVENT );

  mace::string str;
  mace::HierarchicalContextLock( he, str );

  ThreadStructure::setEvent( he );

  mace::HierarchicalContextLock::commit();
  */
}
BOOST_AUTO_TEST_SUITE_END()
