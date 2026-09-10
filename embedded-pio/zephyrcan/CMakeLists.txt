add_library(zephyrcan STATIC
    zephyrcan.cpp
)

target_link_libraries(zephyrcan PRIVATE zephyr_interface)

add_dependencies(zephyrcan zephyr_generated_headers)

target_include_directories(zephyrcan
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
