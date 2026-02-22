add_executable(lab2_part1
    lab2_part1.c
    sha256.c
)

target_link_libraries(lab2_part1
    freertos_kernel
    freertos_config
    xilinx_drivers
)

target_include_directories(lab2_part1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

