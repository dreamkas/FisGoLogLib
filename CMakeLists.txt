cmake_minimum_required(VERSION 3.0)
project(logDB)

set(CMAKE_CXX_STANDARD 11)


#dreamkas RF
set(CMAKE_CXX_COMPILER /usr/local/usr/bin/arm-linux-g++)
set(CMAKE_C_COMPILER /usr/local/usr/bin/arm-linux-gcc)
set(CMAKE_LIBRARY_PATH "/usr/local/usr/arm-buildroot-linux-uclibcgnueabi/sysroot/lib")
include_directories(/usr/local/arm_linux_4.8/arm-nuvoton-linux-uclibceabi/include/)

#dreamkas F
#set(CMAKE_CXX_COMPILER /usr/local/arm_linux_4.8/bin/arm-linux-g++)
#set(CMAKE_C_COMPILER /usr/local/arm_linux_4.8/bin/arm-linux-gcc)


set(CMAKE_C_FLAGS   "-std=gnu99")
set(CMAKE_CXX_FLAGS "-std=c++11 -Werror -Wno-pmf-conversions -Wall")


include_directories(src/appl/include)

aux_source_directory(src/appl applDir)

add_library(logDB SHARED ${applDir})

