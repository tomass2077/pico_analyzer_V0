if (NOT TARGET TurboGFX_16)
    add_library(TurboGFX_16 INTERFACE)

    target_sources(TurboGFX_16 INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/TurboGFX_16.cpp
    )

    target_link_libraries(TurboGFX_16 INTERFACE pico_stdlib hardware_clocks hardware_dma pico_mem_ops hardware_spi)
    target_include_directories(TurboGFX_16 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/)
    
endif()
