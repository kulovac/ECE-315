cmake_minimum_required(VERSION 3.15)
project(labs C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(FREERTOS_KERNEL_PATH ${CMAKE_SOURCE_DIR}/external/FreeRTOS-Kernel)
set(EMBEDDEDSW_ROOT     ${CMAKE_SOURCE_DIR}/external/embeddedsw)

# ------------------------
# embeddedsw drivers
# ------------------------
add_library(xilinx_drivers INTERFACE)
target_include_directories(xilinx_drivers INTERFACE # NOTE: Add more as needed
    ${EMBEDDEDSW_ROOT}/XilinxProcessorIPLib/drivers/common/src
    ${EMBEDDEDSW_ROOT}/XilinxProcessorIPLib/drivers/gpio/src
    ${EMBEDDEDSW_ROOT}/XilinxProcessorIPLib/drivers/scugic/src
    ${EMBEDDEDSW_ROOT}/XilinxProcessorIPLib/drivers/uartps/src
    ${EMBEDDEDSW_ROOT}/lib/bsp/standalone/src/common
)

# ------------------------
# FreeRTOS config
# ------------------------
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE
    ${FREERTOS_KERNEL_PATH}/examples/template_configuration
)

set(FREERTOS_HEAP "4" CACHE STRING "" FORCE)
set(FREERTOS_PORT "TEMPLATE" CACHE STRING "" FORCE)

add_subdirectory(${FREERTOS_KERNEL_PATH} FreeRTOS-Kernel)

# Warnings (once, globally)
target_compile_options(freertos_kernel PRIVATE
    -Wall -Wextra -Wpedantic
)

# ------------------------
# Subprojects
# ------------------------
add_subdirectory(${CMAKE_SOURCE_DIR}/lab1/part1/)
add_subdirectory(${CMAKE_SOURCE_DIR}/lab1/part2/)
add_subdirectory(${CMAKE_SOURCE_DIR}/lab1/part3/)
add_subdirectory(${CMAKE_SOURCE_DIR}/lab2/part1/)

