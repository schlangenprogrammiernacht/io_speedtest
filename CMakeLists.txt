cmake_minimum_required (VERSION 3.2)
project (io_speedtest VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-Wall -pedantic")

add_subdirectory(master)
add_subdirectory(worker)

