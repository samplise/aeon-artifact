SET(APPS logreader)
SET(SERVICES ALL)

# This is how we link in the services to the log reader application. This allows the
# log reader to be able to deserialize any services from the binary log

#Here list the -dummy-init.cc files for services you'll be using either with state dump or binary dump
#LIST(APPEND logreader_SRCS )

IF (logreader_SRCS)
SET_SOURCE_FILES_PROPERTIES(${logreader_SRCS}
  GENERATED)
ENDIF (logreader_SRCS)

INCLUDE(${${PROJECT_NAME}_BINARY_DIR}/${APPLICATION_SUBDIR}/application.cmake)
