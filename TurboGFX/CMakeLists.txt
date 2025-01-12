if (NOT TARGET TurboGFX)
    add_library(TurboGFX INTERFACE)

    target_sources(TurboGFX INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/TurboGFX.cpp
        ${CMAKE_CURRENT_LIST_DIR}/TurboGFX_canvas.cpp

    )

    target_link_libraries(TurboGFX INTERFACE pico_stdlib hardware_clocks hardware_dma pico_mem_ops hardware_spi)
    target_include_directories(TurboGFX INTERFACE ${CMAKE_CURRENT_LIST_DIR}/)
    
endif()
