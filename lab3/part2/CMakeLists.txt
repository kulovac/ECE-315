add_executable(lab3_part2
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

target_link_libraries(lab3_part2
    freertos_kernel
    freertos_config
    xilinx_drivers
)

target_include_directories(lab3_part2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)


