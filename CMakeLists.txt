cmake_minimum_required( VERSION 3.1 )

add_executable( gsc ./gsc.cpp )
target_compile_features( gsc PRIVATE cxx_auto_type )
