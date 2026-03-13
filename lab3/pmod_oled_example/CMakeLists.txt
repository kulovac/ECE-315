add_executable(pmod_example
    ChrFont0.c
    FillPat.c
    OLEDControllerCustom.c
    OledChar.c
    OledDriver.c
    OledGrph.c
    PmodOLED.c
    main.c
    pmodkypd.c
)

target_link_libraries(pmod_example
    freertos_kernel
    freertos_config
    xilinx_drivers
)

target_include_directories(pmod_example PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


