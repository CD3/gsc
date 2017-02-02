cmake_minimum_required( VERSION 2.8 )


# The boost command line argument parser.
find_package( Boost COMPONENTS program_options REQUIRED)
include_directories( ${Boost_INCLUDE_DIRS} )
set( Boost_LIBS ${Boost_LIBS} ${Boost_LIBRARIES} )

find_package( Boost COMPONENTS regex REQUIRED)
include_directories( ${Boost_INCLUDE_DIRS} )
set( Boost_LIBS ${Boost_LIBS} ${Boost_LIBRARIES} )

add_definitions("-std=c++11")


add_executable( gsc ./src/gsc.cpp )
target_link_libraries( gsc ${Boost_LIBS} )
