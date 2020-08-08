SET(APPS notifyd)
SET(LIBNAME notify)
SET(SERVICES GenericTreeMulticast RouteTransportWrapper RandTree TcpTransport)
SET(EXTRALIBS Http)
SET(XMLRPC ${Mace_BINARY_DIR}/services/interfaces/MulticastServiceClass.h)

INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/../application.cmake)
