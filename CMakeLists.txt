cmake_minimum_required( VERSION 2.8 )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules")


# The boost command line argument parser.
find_package( Boost COMPONENTS program_options REQUIRED)
include_directories( ${Boost_INCLUDE_DIRS} )
set( Boost_LIBS ${Boost_LIBS} ${Boost_LIBRARIES} )

find_package( Boost COMPONENTS regex REQUIRED)
include_directories( ${Boost_INCLUDE_DIRS} )
set( Boost_LIBS ${Boost_LIBS} ${Boost_LIBRARIES} )

set( LIBS ${LIBS} ${Boost_LIBS} )

find_package( xdo )
if(XDO_FOUND)
  set( LIBS ${LIBS} ${XDO_LIBRARIES} )
  include_directories( ${XDO_INCLUDE_DIRS} )
  add_definitions("-DUSE_XDO")
endif(XDO_FOUND)


add_definitions("-std=c++11")


add_executable( gsc ./src/gsc.cpp )
target_link_libraries( gsc ${LIBS} )
