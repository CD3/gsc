cmake_minimum_required( VERSION 3.9 )

add_executable( gsc gsc.cpp )
target_link_libraries( gsc
  libgsc
  Boost::boost
  Boost::program_options
  Boost::filesystem
  )

install( TARGETS gsc DESTINATION bin )
