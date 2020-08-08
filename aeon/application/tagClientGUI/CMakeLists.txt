SET(APPS tagClientGUI)
SET(SERVICES TagClientRG TcpTransport)

SET(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)

INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/../application.cmake)
target_link_libraries ( ${APPS} ${CURSES_NCURSES_LIBRARY} )
