SET(APPS chubbyApplication)
SET(SERVICES SimpleChubby Transport Paxos WCPaxos)
 
INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/../application.cmake)
