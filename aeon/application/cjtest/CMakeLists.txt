SET(APPS cjtest)
#SET(LIBNAME AppMacedon)

SET(SERVICES TcpTransport Simple)
#SET(cjtest_SRCS driver.cc)

ADD_EXECUTABLE(driver driver.cc)
TARGET_LINK_LIBRARIES(driver mace )
#SET(SERVICES ALL)
SET(EXTRALIBS boost_unit_test_framework)

INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/../application.cmake)
#ADD_TEST("contextjob-standalone-vnode" ${EXECUTABLE_OUTPUT_PATH}/cjtest --log-level=test_suite --run_test=standalone/VirtualNode) 
#ADD_TEST("contextjob-standalone-physaddr" ${EXECUTABLE_OUTPUT_PATH}/cjtest --log-level=test_suite --run_test=standalone/PhysicalAddress) 
#ADD_TEST("contextjob-scheduled" ${EXECUTABLE_OUTPUT_PATH}/driver) 
