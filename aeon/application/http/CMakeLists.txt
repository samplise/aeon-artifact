SET(APPS httpd httpget testwebserver)
IF(INCLUDE_CGI)
  SET(APPS ${APPS} cgid)
ENDIF(INCLUDE_CGI)
#SET(LIBNAME )
#SET(SERVICES )
SET(EXTRALIBS Http appcommon)

INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/../application.cmake)
