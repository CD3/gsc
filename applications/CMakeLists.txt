cmake_minimum_required( VERSION 3.9 )


project( gsc-applications VERSION 1.0.0.0 )

find_package( Boost REQUIRED COMPONENTS program_options )
add_subdirectory( .. libgsc )

file( GLOB_RECURSE application_SOURCES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
	"*.cpp" 
	)
		
message(STATUS "Detected Application Sources:")
foreach(appSrc ${application_SOURCES})
	message(STATUS "  ${appSrc}" )
endforeach()
	
foreach(appSrc ${application_SOURCES})
	get_filename_component(appName ${appSrc} NAME_WE)
	add_executable( ${appName} ${appSrc} )

	target_link_libraries( ${appName} 
	PUBLIC
    libgsc
		Boost::boost
		Boost::program_options
		)

  install( TARGETS ${appName} DESTINATION bin )
endforeach()
