cmake_minimum_required( VERSION 3.10 )

project(libgsc)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules")


# Boost libraries
set(Boost_USE_STATIC_LIBS ON)
find_package( Boost REQUIRED COMPONENTS filesystem log program_options )

add_library( libgsc )
target_sources(
libgsc
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Session.cpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/SessionState.cpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/SessionScript.cpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/CharTree.cpp>
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Session.hpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/SessionState.hpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/SessionScript.hpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.hpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/CharTree.hpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Enums.hpp>
)
target_include_directories( libgsc
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/>
)
target_link_libraries( libgsc PUBLIC Boost::boost Boost::filesystem Boost::log )
set_target_properties( libgsc PROPERTIES CXX_STANDARD 17 )


add_executable( gsc applications/gsc.cpp )
target_link_libraries( gsc
  libgsc
  Boost::boost
  Boost::program_options
  Boost::filesystem
  )

install( TARGETS gsc DESTINATION bin )
install( FILES bin/gsc-mon.py PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION bin )


add_subdirectory(testing)
