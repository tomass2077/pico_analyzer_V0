if (NOT TARGET ST7789)
    add_library(ST7789 INTERFACE)

    target_sources(ST7789 INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/ST7789.cpp
    )
    target_link_libraries(ST7789 INTERFACE pico_stdlib hardware_clocks hardware_dma pico_mem_ops hardware_spi)
    target_include_directories(ST7789 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../)
    
endif()
