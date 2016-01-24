cmake_minimum_required(VERSION 3.4)
project(libudev-fbsd C)

set(LINUX_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(utils)

add_subdirectory(src)
