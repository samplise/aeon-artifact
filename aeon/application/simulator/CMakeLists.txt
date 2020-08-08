IF(${PROJECT_NAME} STREQUAL "Mace")
  SET(APPS macesim)
  #SET(simmain_SRCS ServiceTests.cc ../modelchecker/TsunamiTest.cc  BPTest.cc ${EXTERNAL_TEST_SRCS})
  SET(macesim_SRCS ServiceTests.cc ${EXTERNAL_TEST_SRCS})
  SET(LIBNAME SimulatorFiles)
ELSE(${PROJECT_NAME} STREQUAL "Mace") 
  SET(APPS ${PROJECT_NAME}_macesim)
  SET(${PROJECT_NAME}_macesim_SRCS macesim.cc ${EXTERNAL_TEST_SRCS})
  SET(LIBNAME ${PROJECT_NAME}SimulatorFiles)
  LINK_DIRECTORIES(${Mace_BINARY_DIR}/application/simulator_common)
ENDIF(${PROJECT_NAME} STREQUAL "Mace")

SET(SERVICES ALL)
SET(EXTRALIBS SimulatorCommonFiles)

IF(EXISTS ${HDFS_INC_PATH})
  LIST(APPEND EXTRALIBS ${LIB_HDFS} ${LIB_JVM})
ENDIF(EXISTS ${HDFS_INC_PATH})

INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR}/application/simulator_common)
INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR}/application/simulator)

# Uncomment the line below for better performance
#SET(CMAKE_CXX_FLAGS "-O2")

INCLUDE(${${PROJECT_NAME}_BINARY_DIR}/${APPLICATION_SUBDIR}/application.cmake)

IF(${PROJECT_NAME} STREQUAL "Mace")
  #SET_TARGET_PROPERTIES(simmain PROPERTIES OUTPUT_NAME modelchecker)
ELSE(${PROJECT_NAME} STREQUAL "Mace")
  #SET_TARGET_PROPERTIES(${PROJECT_NAME}_simmain PROPERTIES OUTPUT_NAME modelchecker)
ENDIF(${PROJECT_NAME} STREQUAL "Mace")
