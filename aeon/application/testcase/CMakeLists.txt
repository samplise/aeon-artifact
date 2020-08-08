SET(APPS testcase )
SET(SERVICES TcpTransport 
  TestCases 
  TestCase1 
  TestCase2
  TestCase3
  TestCase4
  TestCase5
  TestCase6 
  TestCase7
  Throughput
  Latency
)

INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/../application.cmake)

ADD_TEST("testcase-testcase1" ${EXECUTABLE_OUTPUT_PATH}/testcase -test_case 1) 
ADD_TEST("testcase-testcase2" ${EXECUTABLE_OUTPUT_PATH}/testcase ${Mace_SOURCE_DIR}/tools/contextlattice/params.test2) 
ADD_TEST("testcase-testcase3" ${Mace_SOURCE_DIR}/tools/contextlattice/test_twophysicalnode.sh ${Mace_SOURCE_DIR}/tools/contextlattice/)
ADD_TEST("testcase-testcase4" ${EXECUTABLE_OUTPUT_PATH}/testcase ${Mace_SOURCE_DIR}/tools/contextlattice/params.test4) 
ADD_TEST("testcase-testcase5" ${EXECUTABLE_OUTPUT_PATH}/testcase ${Mace_SOURCE_DIR}/tools/contextlattice/params.test5) 
ADD_TEST("testcase-testcase6" ${EXECUTABLE_OUTPUT_PATH}/testcase ${Mace_SOURCE_DIR}/tools/contextlattice/params.test6) 
#ADD_TEST("testcase-testcase7" ${EXECUTABLE_OUTPUT_PATH}/testcase -test_case 7) 

ADD_TEST("testcase-migrate1"  ${Mace_SOURCE_DIR}/tools/contextlattice/run_migration_test.sh ${Mace_SOURCE_DIR}/tools/contextlattice/)
