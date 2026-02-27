add_executable(lab2_part3
    lab2_part3.c
    uart_driver.c
)

target_link_libraries(lab2_part3
    freertos_kernel
    freertos_config
    xilinx_drivers
)

target_include_directories(lab2_part3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

