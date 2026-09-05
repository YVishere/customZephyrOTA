cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_include_directories(app 
    PRIVATE 
        inc
        embedded-pio/zephyrcan
)

target_sources(app 
    PRIVATE 
        src/main.cpp
        inc/const.h
        embedded-pio/zephyrcan/zephyrcan.cpp
        embedded-pio/zephyrcan/zephyrcan.h
)
