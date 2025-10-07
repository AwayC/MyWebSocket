cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0135 NEW)
project(MyWebsocket CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third-party/libuv)
add_subdirectory(third-party/http-parser)
add_subdirectory(third-party/leptjson)

add_library(websocket
    src/core/httpserver.cpp
    src/core/httpResp.cpp
    src/core/FileReader.cpp
    src/core/WebSocket.cpp
    src/core/WsServer.cpp
)

target_link_libraries(websocket PUBLIC
    uv_a
    http-parser
    leptjson
)

target_include_directories(websocket
    PUBLIC
    include/
)

add_executable(MyWebsocket main.cpp)
add_executable(client client.cpp)

target_link_libraries(MyWebsocket
    websocket
)
