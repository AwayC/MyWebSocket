cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0135 NEW)
project(MyWebsocket CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)

# jwt-cpp
include(FetchContent)
fetchcontent_declare(jwt-cpp
        GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
        GIT_TAG 08bcf77a687fb06e34138e9e9fa12a4ecbe12332 # v0.7.0 release
)
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "disable building examples" FORCE)
fetchcontent_makeavailable(jwt-cpp)


add_subdirectory(third-party/libuv)
add_subdirectory(third-party/http-parser)
add_subdirectory(third-party/leptjson)

set(UTIL_DIR
    src/util/ssl.cpp
    src/util/jwt.cpp
)

add_library(websocket
    src/core/httpserver.cpp
    src/core/httpResp.cpp
    src/core/FileReader.cpp
    src/core/httpReq.cpp
    src/core/WebSocket.cpp
    src/core/WsServer.cpp
    src/core/websocket_parser.cpp
    ${UTIL_DIR}
    src/core/httpRouter.cpp
)

target_linK_libraries(websocket PUBLIC
    jwt-cpp::jwt-cpp
)

target_link_libraries(websocket PUBLIC
    uv_a
    http-parser
    leptjson
    ${OPENSSL_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)

target_include_directories(websocket
    PUBLIC
    include/
)

#test http server
add_executable(MyWebsocket main.cpp)
add_executable(client client.cpp)

target_link_libraries(MyWebsocket
    websocket
)

#test ws server
add_executable(ws_server test/WsServer.cpp)
target_link_libraries(ws_server
    websocket
)

#test ws parser
add_executable(ws_parser test/test_ws_parser.cpp)
target_link_libraries(ws_parser
    websocket
)

#test http router
add_executable(http_router test/test_router.cpp)
target_link_libraries(http_router
        websocket
)

#test jwt
add_executable(test_jwt test/test_jwt.cpp)
target_link_libraries(test_jwt
        websocket
        jwt-cpp::jwt-cpp
)

#test sendfile
add_executable(test_sendfile test/test_sendfile.cpp)
target_link_libraries(test_sendfile
        websocket
)
