cmake_minimum_required(VERSION 3.0)
project(logDB)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_COMPILER arm-linux-g++)
set(CMAKE_C_COMPILER arm-linux-gcc)

set(CMAKE_C_FLAGS   "-std=gnu99")
set(CMAKE_CXX_FLAGS "-std=c++11 -Werror -Wno-pmf-conversions -Wall")


include_directories(src/appl/include)

aux_source_directory(src/appl applDir)

add_library(logDB SHARED ${applDir})

