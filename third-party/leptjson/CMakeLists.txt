cmake_minimum_required(VERSION 3.10) 
project(leptjson_test)

set(CMAKE_CXX_STANDARD 17)

set(headers
	third-party/double-conversion/bignum.h
	third-party/double-conversion/bignum-dtoa.h
	third-party/double-conversion/cached-powers.h
	third-party/double-conversion/diy-fp.h
	third-party/double-conversion/double-conversion.h
	third-party/double-conversion/fast-dtoa.h
	third-party/double-conversion/double-to-string.h
	third-party/double-conversion/fixed-dtoa.h
	third-party/double-conversion/ieee.h
	third-party/double-conversion/string-to-double.h
	third-party/double-conversion/strtod.h
	third-party/double-conversion/utils.h
)
add_library(double-conversion STATIC
	third-party/double-conversion/bignum.cc
	third-party/double-conversion/bignum-dtoa.cc
	third-party/double-conversion/cached-powers.cc 
	third-party/double-conversion/double-to-string.cc
	third-party/double-conversion/fast-dtoa.cc
	third-party/double-conversion/fixed-dtoa.cc 
	third-party/double-conversion/string-to-double.cc
	third-party/double-conversion/strtod.cc
	${headers}
)

target_include_directories(double-conversion PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/third-party/double-conversion
)

add_library(leptjson
	leptjson.cpp
	leptjson.h 
)

target_include_directories(leptjson PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(leptjson PRIVATE double-conversion) 
add_executable(leptjson_test test.cpp) 
target_link_libraries(leptjson_test PRIVATE leptjson) 
target_include_directories(leptjson_test PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}
)
