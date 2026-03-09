add_executable(lab3_part1
    lab3_part1_student.c
    my_spi.c
    my_uart.c
)

target_link_libraries(lab3_part1
    freertos_kernel
    freertos_config
    xilinx_drivers
)

target_include_directories(lab3_part1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

