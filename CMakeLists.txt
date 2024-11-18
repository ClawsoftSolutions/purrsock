cmake_minimum_required(VERSION 3.14)

project(purrsock)

file(GLOB_RECURSE PURRSOCK_SOURCES "src/**.c" "include/**.h")
add_library(purrsock STATIC ${PURRSOCK_SOURCES})
target_include_directories(purrsock PUBLIC include/)

if(WIN32)
  target_link_libraries(purrsock ws2_32)
endif()
