SET(APPS scheduler launcher)
#SET(LIBNAME adht)
SET(SERVICES CondorHeartBeat)
#ADD_EXECUTABLE(launcher launcher.cc)
#TARGET_LINK_LIBRARIES(launcher CondorHeartBeat )
#SET(EXTRALIBS Http)
#SET(XMLRPC ${CMAKE_CURRENT_SOURCE_DIR}/StringDHT.h)

INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/../application.cmake)
