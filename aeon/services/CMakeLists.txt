# All Services
#SET(SERVICE_DIRS interfaces Transport RequestTransport Http File RandTree
#ReplayTree RanSub Pastry Bamboo Chord GenericOverlayRoute GenericTreeMulticast
#ScribeMS SignedMulticast SimApplication SplitStreamMS FileSync DHT
#CondorHeartBeat LUFactorization LUFAck ThroughputTest GameOfLife Simple Tag TagPlayer)

SET(SERVICE_DIRS interfaces Transport RequestTransport Http File RandTree
ReplayTree RanSub Pastry Bamboo Chord GenericOverlayRoute GenericTreeMulticast
ScribeMS SignedMulticast SimApplication SplitStreamMS FileSync DHT
CondorHeartBeat  LUFactorization Simple  MaceTag
GameOfLife NewGOL TestCases GenericGameOfLife NPB_MGApplication
Paxos SimpleChubby Benchmark SiloDatabase ElasticMemcached Memcached
SimpleTwitter SerializeTest BTreeApp BinaryTreeApp TagApp ListApp PageRankApp
PiazzaApp ElasticPiazzaApp ElasticTagApp TagItemShareApp TreeTPCCApp
Microbenchmark HaloPresenceService TPCWMinApp WTFApp DistributedDAGTestApp 
Chirper HelloWorld GameApp)

#Minimal services to test
#SET(SERVICE_DIRS interfaces Transport RequestTransport Pastry Bamboo
#GenericOverlayRoute GenericTreeMulticast SignedMulticast)


WRITE_FILE(${EXTERNAL_VARS_FILE} "
# Service Includes and Links" APPEND)

FOREACH(SERVICE_DIR ${SERVICE_DIRS})
  IF(SERVICE_DIR STREQUAL "Transport")
    SET(ALL_TGT_DEP TransportCryptoServiceClass_dep BufferedTransportServiceClass_dep ConnectionAcceptanceServiceClass_dep)
  ELSE(SERVICE_DIR STREQUAL "Transport")
    SET(ALL_TGT_DEP)
  ENDIF(SERVICE_DIR STREQUAL "Transport")

  SET(EXTRA_SVLIB_DEP interfaces)

  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/${SERVICE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${SERVICE_DIR})

  SET(FULL_SERVICE_DIRS ${FULL_SERVICE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/${SERVICE_DIR})
  SET(FULL_SERVICE_BIN_DIRS ${FULL_SERVICE_BIN_DIRS} ${CMAKE_CURRENT_BINARY_DIR}/${SERVICE_DIR})

  SET(SERVICE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${SERVICE_DIR}")
  CONFIGURE_FILE("servicedir.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/${SERVICE_DIR}/CMakeLists.txt" @ONLY)
  ADD_SUBDIRECTORY(${CMAKE_CURRENT_BINARY_DIR}/${SERVICE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/${SERVICE_DIR})
  INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/${SERVICE_DIR}/mh.cmake)
  WRITE_FILE(${EXTERNAL_VARS_FILE} "INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/${SERVICE_DIR}/mh.cmake)" APPEND)
ENDFOREACH(SERVICE_DIR)

#WRITE_FILE(${EXTERNAL_VARS_FILE} "SET(FULL_SERVICE_DIRS ${FULL_SERVICE_DIRS})" APPEND)
#WRITE_FILE(${EXTERNAL_VARS_FILE} "SET(FULL_SERVICE_BIN_DIRS ${FULL_SERVICE_BIN_DIRS})" APPEND)

WRITE_FILE(${EXTERNAL_VARS_FILE} "INCLUDE_DIRECTORIES(${FULL_SERVICE_DIRS} ${FULL_SERVICE_BIN_DIRS})" APPEND)
IF(NOT LIBRARY_OUTPUT_PATH)
  WRITE_FILE(${EXTERNAL_VARS_FILE} "LINK_DIRECTORIES(${FULL_SERVICE_BIN_DIRS})" APPEND)
ENDIF(NOT LIBRARY_OUTPUT_PATH)

CONFIGURE_FILE(${Mace_SOURCE_DIR}/application/application.cmake.in ${Mace_BINARY_DIR}/application/application.cmake @ONLY)
