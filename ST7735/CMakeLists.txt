if (NOT TARGET ST7735)
    add_library(ST7735 INTERFACE)

    target_sources(ST7735 INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/ST7735.cpp
    )


    target_link_libraries(ST7735 INTERFACE pico_stdlib hardware_clocks hardware_dma pico_mem_ops hardware_spi)
    target_include_directories(ST7735 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../)
    
endif()
