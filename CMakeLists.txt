cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0135 NEW)
project(MyWebsocket CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src/util/libuv)
add_subdirectory(src/util/http-parser)


add_library(websocket
    src/core/httpserver.cpp
)

target_link_libraries(websocket
    uv_a
    http-parser
)



add_executable(MyWebsocket main.cpp)
add_executable(client client.cpp)

target_link_libraries(MyWebsocket
    websocket
)
