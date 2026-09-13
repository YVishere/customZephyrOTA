cmake_minimum_required(VERSION 3.28.0)

list(
    APPEND 
    EXTRA_CONF_FILE
        ${CMAKE_CURRENT_SOURCE_DIR}/embedded-pio/zephyrcan/zephyrcan.conf
        ${CMAKE_CURRENT_SOURCE_DIR}/embedded-pio/zephyrethernet/zephyrethernet.conf
        ${CMAKE_CURRENT_SOURCE_DIR}/embedded-pio/updatemanager_eth/updatemanager_eth.conf
)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(canExample)

add_subdirectory(embedded-pio/zephyrcan)
add_subdirectory(embedded-pio/zephyrethernet)
add_subdirectory(embedded-pio/updatemanager_eth)

target_include_directories(app 
    PRIVATE 
        inc
)

target_sources(app 
    PRIVATE 
        src/main.cpp
        src/examplecan.cpp
        src/OTAP.cpp
        inc/OTAP.h
        inc/examplecan.h
        inc/const.h
)

target_link_libraries(app
    PRIVATE
        zephyrcan
        zephyrethernet
        updatemanager_eth
)
