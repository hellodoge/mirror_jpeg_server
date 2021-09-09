cmake_minimum_required(VERSION 3.19)
project(mirror_jpeg_server)

set(CMAKE_CXX_STANDARD 17)

find_package(JPEG REQUIRED)
find_package(Boost 1.76.0 REQUIRED)

include_directories(include)
include_directories(${Boost_INCLUDE_DIR})

set(SOURCES_HTTP_SERVER src/http_server.cpp include/http_server.hpp)

set(SOURCES_HANDLER_COMMON include/handler_interface.hpp)

set(SOURCES_HANDLER_MIRROR_JPEG
        ${SOURCES_HANDLER_COMMON}
        include/mirror_jpeg_handler.hpp
        src/mirror_jpeg_handler.cpp)

add_executable(mirror_jpeg_server src/main.cpp ${SOURCES_HTTP_SERVER} ${SOURCES_HANDLER_MIRROR_JPEG})

target_link_libraries(mirror_jpeg_server pthread jpeg)