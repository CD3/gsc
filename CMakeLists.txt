cmake_minimum_required( VERSION 3.10 )

project(libgsc)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules")


# Boost libraries
set(Boost_USE_STATIC_LIBS ON)
find_package( Boost REQUIRED COMPONENTS filesystem log )

add_library( libgsc )
target_sources(
libgsc
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/gsc.cpp>
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/gsc.h>
)
target_include_directories( libgsc
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/>
)
target_link_libraries( libgsc PUBLIC Boost::boost Boost::filesystem Boost::log )
set_target_properties( libgsc PROPERTIES CXX_STANDARD 17 )


add_subdirectory(testing)
