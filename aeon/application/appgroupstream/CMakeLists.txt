SET(APPS groupstream groupstream2 groupstream3)
#SET(LIBNAME AppMacedon)
SET(SERVICES Bamboo ReplayTree RandTree GenericTreeMulticast SignedMulticast Pastry Scribe CacheRecursiveOverlayRoute TcpTransport UdpTransport)
#SET(EXTRALIBS)

INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/../application.cmake)
