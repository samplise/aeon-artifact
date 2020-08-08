SET(APPS unit_app)
#SET(LIBNAME AppMacedon)
#SET(SERVICES LookupTest ransub_test chord_test Chord Bamboo RouteTransportWrapper RecursiveOverlayRoute CacheRecursiveOverlayRoute TcpTransport UdpTransport RanSub RanSubAggregator RanSub2Aggregator RandTree)

#SET(SERVICES ThroughputTest GameOfLife Simple)
#SET(SERVICES TcpTransport  Simple Tag GameOfLife NewGOL NewGOLMonitor MIS
#  MISsingle MISmulti
#GameOfLifeDowngrade)
SET(SERVICES TimerTest TcpTransport)

#SET(SERVICES ALL)
#SET(EXTRALIBS Transport)

INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/../application.cmake)
