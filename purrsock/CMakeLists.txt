cmake_minimum_required(VERSION 3.14)

project(purrsock)

option(TEST "Enable testing" OFF)

file(GLOB_RECURSE PURRSOCK_SOURCES "src/**.c" "include/purrsock/**.h")

add_library(purrsock STATIC ${PURRSOCK_SOURCES})

target_include_directories(purrsock PUBLIC "./include/")

if(WIN32)
    target_link_libraries(purrsock ws2_32)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    add_definitions(-DPLATFORM_LINUX)
else()
    message(FATAL_ERROR "Unsupported platform")
endif()
