cmake_minimum_required (VERSION 3.12)

# Project stuff
project (ipfilter_tst
	VERSION 0.0.${TRAVIS_BUILD_NUMBER}
	DESCRIPTION "Homework 3 Allocator & Container implementation tests"
	HOMEPAGE_URL "https://github.com/edwdmkrv/HW-02-C-14/tree/master/tst"
	LANGUAGES CXX
)

# Test stuff
add_executable (test_allocator test_allocator.cpp)
add_executable (test_container test_container.cpp)

set_target_properties (test_allocator test_container PROPERTIES
	CXX_STANDARD 17
	CXX_EXTENSIONS OFF
	CXX_STANDARD_REQUIRED ON
	COMPILE_OPTIONS "-pedantic;-Wall;-Wextra"
	LINK_LIBRARIES "${GTEST_LIBRARY};${GTEST_MAIN_LIBRARY};lib;${CMAKE_THREAD_LIBS_INIT}"
)

add_test (NAME Test-allocator
	COMMAND test_allocator
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

add_test (NAME Test-container
	COMMAND test_container
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

include (CTest)
