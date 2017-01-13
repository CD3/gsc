cmake_minimum_required( VERSION 3.1 )


# The boost command line argument parser.
find_package( Boost COMPONENTS program_options REQUIRED)
include_directories( ${Boost_INCLUDE_DIRS} )
set( Boost_LIBS ${Boost_LIBS} ${Boost_LIBRARIES} )


add_executable( gsc ./src/gsc.cpp )
target_compile_features( gsc PRIVATE cxx_auto_type )
target_link_libraries( gsc ${Boost_LIBS} )
