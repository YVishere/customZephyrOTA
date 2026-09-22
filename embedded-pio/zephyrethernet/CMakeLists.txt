add_library(zephyrethernet STATIC
    zephyrethernet.cpp
)

target_link_libraries(zephyrethernet PRIVATE zephyr_interface)

add_dependencies(zephyrethernet zephyr_generated_headers)

target_include_directories(zephyrethernet
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
