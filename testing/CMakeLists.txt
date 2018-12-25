cmake_minimum_required( VERSION 2.8 )

message( "Configuring unit test build." )

OPTION( BUILD_UNIT_TESTS "Build unit tests for the library" ON )


#  ____      _       _       _   _       _ _     _____         _       
# / ___|__ _| |_ ___| |__   | | | |_ __ (_) |_  |_   _|__  ___| |_ ___ 
#| |   / _` | __/ __| '_ \  | | | | '_ \| | __|   | |/ _ \/ __| __/ __|
#| |__| (_| | || (__| | | | | |_| | | | | | |_    | |  __/\__ \ |_\__ \
# \____\__,_|\__\___|_| |_|  \___/|_| |_|_|\__|   |_|\___||___/\__|___/
                                                                      



include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(BUILD_UNIT_TESTS)

set( binDir ${CMAKE_BINARY_DIR}/testBin )
set( buildDir ${CMAKE_BINARY_DIR} )

# Unit tests written with the Catch framework are
# automatically detected. Just put the source files
# in the `CatchTests` directory.

set( unitTest_DIR ./CatchTests)
# Search for sources.
file( GLOB_RECURSE unitTest_SOURCES
      RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
      "${unitTest_DIR}/*.cpp" )
message(STATUS "Detected Catch-based Unit Test Sources:")
foreach(testSrc ${unitTest_SOURCES})
  message(STATUS "  ${testSrc}" )
endforeach()


# Add unit test executable
set( testName ${PROJECT_NAME}_CatchTests )
add_executable(${testName} ${unitTest_SOURCES})
target_link_libraries( ${testName} libgsc )

#Link dependencies
target_link_libraries(${testName} ${LIBS} ) # If your project builds a library, link to it here

# Make sure that testing binaries are created in the binDir directory
set_target_properties(${testName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY  ${binDir} )

# Now add the unit test binary to the CMake test target
#Notice the WORKING_DIRECTORY and COMMAND
add_test(NAME ${testName} 
         WORKING_DIRECTORY ${buildDir}
         COMMAND ${binDir}/${testName} )

add_custom_target( PexpectTests ALL DEPENDS ${binDir}/PexpectTests ) 
add_custom_command(
  OUTPUT ${binDir}/PexpectTests
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/testing/PexpectTests ${binDir}/PexpectTests
  DEPENDS rebuild)
add_custom_command( OUTPUT rebuild COMMAND cmake -E echo )

endif()

