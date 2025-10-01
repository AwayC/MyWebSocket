cmake_minimum_required(VERSION 3.14)
project(leptjson_test)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_library(leptjson leptjson.cpp leptjson.h)

target_include_directories(leptjson PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
