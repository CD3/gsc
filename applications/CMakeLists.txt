cmake_minimum_required( VERSION 3.9 )


project( gsc-applications )

set(Boost_USE_STATIC_LIBS ON)
find_package( Boost REQUIRED COMPONENTS program_options filesystem )
add_subdirectory( .. libgsc )

add_executable( gsc gsc.cpp )
target_link_libraries( gsc
  libgsc
  Boost::boost
  Boost::program_options
  Boost::filesystem
  )

install( TARGETS gsc DESTINATION bin )
