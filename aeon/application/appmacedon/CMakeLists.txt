SET(APPS appmacedon)
#SET(LIBNAME AppMacedon)
#SET(SERVICES ReplayTree RandTree GenericTreeMulticast Pastry ScribeMS SplitStreamMS CacheRecursiveOverlayRoute)
SET(SERVICES ALL)
#SET(EXTRALIBS)

INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/../application.cmake)
