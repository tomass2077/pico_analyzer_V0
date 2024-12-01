cmake_minimum_required(VERSION 3.12)
add_compile_definitions(PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

include(/home/tomass/pico/pico-sdk/external/pico_sdk_import.cmake)
set(PICO_EXTRAS_PATH "/home/tomass/pico/pico-extras")
include(/home/tomass/pico/pico-extras/external/pico_extras_import.cmake)

set(bin_name "full_build")
project(${bin_name}  C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_subdirectory(ST7789)
add_subdirectory(ST7735)
add_subdirectory(TurboGFX)

add_executable(${bin_name}
    main.cpp
)

pico_enable_stdio_usb(${bin_name} 1)
pico_enable_stdio_uart(${bin_name} 0)
target_link_libraries(${bin_name}
    hardware_sleep
    pico_stdlib
    hardware_uart
    hardware_pwm
    hardware_adc
    hardware_dma
    pico_mem_ops
    pico_multicore
    hardware_pio
    hardware_i2c
    ST7789
    TurboGFX
    pico_i2c_slave
    
)
target_include_directories(TurboGFX INTERFACE ${CMAKE_CURRENT_LIST_DIR}/)

pico_add_extra_outputs(${bin_name})