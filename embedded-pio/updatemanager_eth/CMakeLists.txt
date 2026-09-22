add_library(updatemanager_eth STATIC
    updatemanager_eth.cpp
)

target_link_libraries(updatemanager_eth PRIVATE zephyr_interface zephyrethernet)

add_dependencies(updatemanager_eth zephyr_generated_headers)

target_include_directories(updatemanager_eth
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
