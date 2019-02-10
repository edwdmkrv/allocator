cmake_minimum_required (VERSION 3.6)

# Project stuff
project (allocator_src
	VERSION 0.0.${TRAVIS_BUILD_NUMBER}
	DESCRIPTION "Homework 3 Allocator & Container use of implementation program"
	HOMEPAGE_URL "https://github.com/edwdmkrv/HW-02-C-14/tree/master/src"
	LANGUAGES CXX
)

# Program stuff
add_executable (allocator allocator.cpp)

set_target_properties (allocator PROPERTIES
	CXX_STANDARD 17
	CXX_EXTENSIONS OFF
	CXX_STANDARD_REQUIRED ON
	COMPILE_OPTIONS "-pedantic;-Wall;-Wextra"
	LINK_LIBRARIES "lib;${CMAKE_THREAD_LIBS_INIT}"
)

install (TARGETS allocator RUNTIME DESTINATION Debug/bin CONFIGURATIONS Debug)
install (TARGETS allocator RUNTIME DESTINATION bin CONFIGURATIONS Release)
