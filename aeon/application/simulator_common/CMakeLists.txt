FILE(GLOB sources *.cc *.c)

ADD_LIBRARY(SimulatorCommonFiles ${LIBTYPE} ${sources})
TARGET_LINK_LIBRARIES(SimulatorCommonFiles interfaces mace)
ADD_DEPENDENCIES(SimulatorCommonFiles interfaces mace)

INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR}/application/simulator_common)
INCLUDE_DIRECTORIES(${Mace_BINARY_DIR}/services/interfaces)
INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR}/services/interfaces)
INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR}/services)
INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR}/lib)
INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR})
INCLUDE_DIRECTORIES(${Mace_BINARY_DIR})

# IF(${PROJECT_NAME} STREQUAL "Mace")
# #  SET(APPS simmain)
# #  #SET(simmain_SRCS ServiceTests.cc ../modelchecker/TsunamiTest.cc  BPTest.cc ${EXTERNAL_TEST_SRCS})
# #  SET(simmain_SRCS ServiceTests.cc ${EXTERNAL_TEST_SRCS})
#   SET(LIBNAME SimulatorCommonFiles)
# ELSE(${PROJECT_NAME} STREQUAL "Mace") 
# #  SET(APPS ${PROJECT_NAME}_simmain)
# #  SET(${PROJECT_NAME}_simmain_SRCS simmain.cc ${EXTERNAL_TEST_SRCS})
#   SET(LIBNAME ${PROJECT_NAME}SimulatorCommonFiles)
# ENDIF(${PROJECT_NAME} STREQUAL "Mace")
# 
# SET(SERVICES ALL)
# #SET(EXTRALIBS)
# 
# 
# INCLUDE(${${PROJECT_NAME}_SOURCE_DIR}/${APPLICATION_SUBDIR}/application.cmake)
# 
# #IF(${PROJECT_NAME} STREQUAL "Mace")
# #  SET_TARGET_PROPERTIES(simmain PROPERTIES OUTPUT_NAME modelchecker)
# #ELSE(${PROJECT_NAME} STREQUAL "Mace")
# #  SET_TARGET_PROPERTIES(${PROJECT_NAME}_simmain PROPERTIES OUTPUT_NAME modelchecker)
# #ENDIF(${PROJECT_NAME} STREQUAL "Mace")
