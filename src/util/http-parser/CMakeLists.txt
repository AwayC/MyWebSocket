cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0135 NEW)
project(http-parser C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(http-parser
    http_parser.c
    http_parser.h
)