cmake_minimum_required( VERSION 3.13 )

project(libgsc)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/Modules")
include(${CMAKE_BINARY_DIR}/conan_paths.cmake OPTIONAL)


# Boost libraries
message(STATUS "Checking for Conan-installed Boost")
find_package( boost QUIET ) # conan version
if( TARGET boost::boost )
  add_library(Boost::boost INTERFACE IMPORTED )
  target_link_libraries( Boost::boost INTERFACE boost::boost )

  add_library(Boost::log   INTERFACE IMPORTED )
  add_library(Boost::filesystem  INTERFACE IMPORTED )
  add_library(Boost::program_options INTERFACE IMPORTED )
  target_link_libraries( Boost::log INTERFACE Boost::boost )
  target_link_libraries( Boost::filesystem INTERFACE Boost::boost )
  target_link_libraries( Boost::program_options INTERFACE Boost::boost )
  message(STATUS "Found Conan-installed Boost")
else()
  message(STATUS "Did not find Conan-installed Boost")
  message(STATUS "Requiring system installed Boost")
  set(Boost_USE_STATIC_LIBS ON)
  find_package( Boost REQUIRED COMPONENTS filesystem log program_options ) # system version
endif()


add_library( libgsc )
target_sources(
libgsc
  PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Session.cpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/SessionState.cpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/SessionScript.cpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.cpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Keybindings.cpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/CharTree.cpp>
  INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Session.hpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/SessionState.hpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/SessionScript.hpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Utils.hpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/CharTree.hpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Keybindings.hpp>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Enums.hpp>
)
target_include_directories( libgsc
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/>
)
target_link_libraries( libgsc PUBLIC Boost::boost Boost::filesystem Boost::log )
target_compile_features( libgsc PUBLIC cxx_std_17 )


add_executable( gsc applications/gsc.cpp )
target_link_libraries( gsc
  libgsc
  Boost::boost
  Boost::program_options
  Boost::filesystem
  )

add_executable( show_key applications/show_key.cpp )

install( TARGETS gsc DESTINATION bin )
install( FILES bin/gsc-mon.py PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE DESTINATION bin )


add_subdirectory(testing)
