INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR}/lib)
INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR}/services/Transport)
INCLUDE_DIRECTORIES(${Mace_BINARY_DIR}/services/interfaces)
INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR}/services/interfaces)
#SET( SERVICES TcpTransport )
#INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/../../application/application.cmake)
ADD_EXECUTABLE(libtest1 libtest1.cc)
TARGET_LINK_LIBRARIES(libtest1 ${Boost_LIBRARIES} mace)

ADD_TEST("libmace" ${EXECUTABLE_OUTPUT_PATH}/libtest1)

ADD_EXECUTABLE(fullcontext_test fullcontext_test.cc)
TARGET_LINK_LIBRARIES(fullcontext_test boost_unit_test_framework mace)

ADD_TEST("libmace-fullcontext-test" ${EXECUTABLE_OUTPUT_PATH}/fullcontext_test)

ADD_EXECUTABLE(contextlock_test contextlock_test.cc)
TARGET_LINK_LIBRARIES(contextlock_test boost_unit_test_framework mace)

ADD_TEST("libmace-contextlock-test1" ${EXECUTABLE_OUTPUT_PATH}/contextlock_test -test_option 1)
ADD_TEST("libmace-contextlock-test2" ${EXECUTABLE_OUTPUT_PATH}/contextlock_test -test_option 2)
ADD_TEST("libmace-contextlock-test3" ${EXECUTABLE_OUTPUT_PATH}/contextlock_test -test_option 3)

ADD_EXECUTABLE(ThreadStructure_test ThreadStructure_test.cc)
TARGET_LINK_LIBRARIES(ThreadStructure_test boost_unit_test_framework mace)

ADD_TEST("libmace-ThreadStructure-test" ${EXECUTABLE_OUTPUT_PATH}/ThreadStructure_test )

ADD_EXECUTABLE(ContextBaseClass_test ContextBaseClass_test.cc)
TARGET_LINK_LIBRARIES(ContextBaseClass_test boost_unit_test_framework mace)

ADD_TEST("libmace-ContextBaseClass-test" ${EXECUTABLE_OUTPUT_PATH}/ContextBaseClass_test )

ADD_EXECUTABLE(ScopedContextRPC_test ScopedContextRPC_test.cc)
TARGET_LINK_LIBRARIES(ScopedContextRPC_test boost_unit_test_framework mace)

ADD_TEST("libmace-ScopedContextRPC-test" ${EXECUTABLE_OUTPUT_PATH}/ScopedContextRPC_test )

ADD_EXECUTABLE(ReadLine_test ReadLine_test.cc)
TARGET_LINK_LIBRARIES(ReadLine_test boost_unit_test_framework mace)

ADD_TEST("libmace-ReadLine-test" ${EXECUTABLE_OUTPUT_PATH}/ReadLine_test )

ADD_EXECUTABLE(HierarchicalContextLock_test HierarchicalContextLock_test.cc)
TARGET_LINK_LIBRARIES(HierarchicalContextLock_test boost_unit_test_framework mace)

ADD_TEST("libmace-HierarchicalContextLock-test" ${EXECUTABLE_OUTPUT_PATH}/HierarchicalContextLock_test )

ADD_EXECUTABLE(HeadEventDispatch_test HeadEventDispatch_test.cc)
TARGET_LINK_LIBRARIES(HeadEventDispatch_test boost_unit_test_framework mace)

ADD_TEST("libmace-HeadEventDispatch-test" ${EXECUTABLE_OUTPUT_PATH}/HeadEventDispatch_test )

ADD_EXECUTABLE(AccessLine_test AccessLine_test.cc)
TARGET_LINK_LIBRARIES(AccessLine_test boost_unit_test_framework mace)

ADD_TEST("libmace-AccessLine-test" ${EXECUTABLE_OUTPUT_PATH}/AccessLine_test )

ADD_EXECUTABLE(ContextService_test ContextService_test.cc)
TARGET_LINK_LIBRARIES(ContextService_test boost_unit_test_framework mace)

ADD_TEST("libmace-ContextService-test" ${EXECUTABLE_OUTPUT_PATH}/ContextService_test )

ADD_EXECUTABLE(AsyncDispatch_test AsyncDispatch_test.cc)
TARGET_LINK_LIBRARIES(AsyncDispatch_test boost_unit_test_framework mace)

ADD_TEST("libmace-AsyncDispatch-test" ${EXECUTABLE_OUTPUT_PATH}/AsyncDispatch_test )

ADD_EXECUTABLE(Event_test Event_test.cc)
TARGET_LINK_LIBRARIES(Event_test boost_unit_test_framework mace)

ADD_TEST("libmace-Event-test" ${EXECUTABLE_OUTPUT_PATH}/Event_test )
