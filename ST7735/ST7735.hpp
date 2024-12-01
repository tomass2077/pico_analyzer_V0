#ifndef LCD_HPP
#define LCD_HPP

#include "hardware/dma.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include <stdlib.h>
#include <hardware/spi.h>

class ST7735
{
public:
    // Constructor with pin configurations, SPI port, and dimensions
    ST7735(uint8_t pin_dc, uint8_t pin_rst, uint8_t pin_cs,
           uint8_t pin_sck, uint8_t pin_mosi,
           spi_inst_t *spi_port, uint16_t width, uint16_t height);
    ~ST7735();

    void Init();
    void Display();
    uint16_t *GetImageBuffer();

private:
    void Writ_Bus(uint8_t dat);
    void Write_Data(uint8_t dat);
    void WR_DATA(uint16_t dat);
    void WR_REG(uint8_t dat);
    void Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
    void spi_config();

    // Inline GPIO control methods using member variables
    inline void CS_Clr() { gpio_put(pin_cs_, 0); }
    inline void CS_Set() { gpio_put(pin_cs_, 1); }
    inline void RST_Clr() { gpio_put(pin_rst_, 0); }
    inline void RST_Set() { gpio_put(pin_rst_, 1); }
    inline void DC_CTRL() { gpio_put(pin_dc_, 0); }
    inline void DC_DATA() { gpio_put(pin_dc_, 1); }

    uint16_t *image_;      // Image buffer
    uint8_t *line_buffer_; // Buffer for one line of image data
    uint dma_tx_;
    dma_channel_config dma_cfg_;

    // Pin configurations and SPI port
    uint8_t pin_dc_, pin_rst_, pin_cs_, pin_sck_, pin_mosi_;
    spi_inst_t *spi_port_;
    uint16_t width_, height_;
};

#endif // LCD_HPP
