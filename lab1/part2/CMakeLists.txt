add_executable(lab1_part2
    lab1_part2.c
    pmodkypd.c
)

target_link_libraries(lab1_part2
    freertos_kernel
    freertos_config
    xilinx_drivers
)

target_include_directories(lab1_part2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

