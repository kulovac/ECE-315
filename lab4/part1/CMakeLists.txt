add_executable(lab4_part1
    lab4_part1.c
    stepper.c
    uart_initialize.c
)

target_link_libraries(lab4_part1
    freertos_kernel
    freertos_config
    xilinx_drivers
)

target_include_directories(lab4_part1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

