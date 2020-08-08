FILE(GLOB sources *.cc *.c)

ADD_LIBRARY(appcommon ${LIBTYPE} ${sources})
TARGET_LINK_LIBRARIES(appcommon interfaces mace)
ADD_DEPENDENCIES(appcommon interfaces)

INCLUDE_DIRECTORIES(${Mace_BINARY_DIR}/services/interfaces)
INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR}/services/interfaces)
INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR}/services)
INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR}/lib)
INCLUDE_DIRECTORIES(${Mace_SOURCE_DIR})
INCLUDE_DIRECTORIES(${Mace_BINARY_DIR})
