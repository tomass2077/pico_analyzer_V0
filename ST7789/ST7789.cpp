#include "ST7789.hpp"
#include <hardware/clocks.h>
#include <string.h>
#define buffSize 64
ST7789::ST7789(uint8_t pin_dc, uint8_t pin_rst, uint8_t pin_cs,
               uint8_t pin_sck, uint8_t pin_mosi,
               spi_inst_t *spi_port, uint16_t width, uint16_t height)
    : pin_dc_(pin_dc), pin_rst_(pin_rst), pin_cs_(pin_cs),
      pin_sck_(pin_sck), pin_mosi_(pin_mosi),
      spi_port_(spi_port), width_(width), height_(height)
{
    // Initialize GPIO pins
    gpio_init(pin_dc_);
    gpio_set_dir(pin_dc_, GPIO_OUT);

    gpio_init(pin_rst_);
    gpio_set_dir(pin_rst_, GPIO_OUT);

    gpio_init(pin_cs_);
    gpio_set_dir(pin_cs_, GPIO_OUT);

    // Initialize image buffer and line buffer
    image_ = new uint8_t[width_ * height_];
    line_buffer_a = new uint16_t[buffSize]; // Each pixel is 2 bytes
    line_buffer_b = new uint16_t[buffSize]; // Each pixel is 2 bytes

    // Initialize SPI and LCD
    spi_config();
    Init();
}

ST7789::~ST7789()
{
    delete[] image_;
    delete[] line_buffer_a;
    delete[] line_buffer_b;
}

uint8_t *ST7789::GetImageBuffer()
{
    return image_;
}

void ST7789::Writ_Bus(uint8_t dat)
{
    CS_Clr();
    spi_write_blocking(spi_port_, &dat, 1);
    CS_Set();
}

void ST7789::Write_Data(uint8_t dat)
{
    DC_DATA();
    Writ_Bus(dat);
}

void ST7789::WR_DATA(uint16_t dat)
{
    DC_DATA();
    uint8_t buf[2] = {static_cast<uint8_t>(dat >> 8), static_cast<uint8_t>(dat & 0xFF)};
    CS_Clr();
    spi_write_blocking(spi_port_, buf, 2);
    CS_Set();
}

void ST7789::WR_REG(uint8_t dat)
{
    DC_CTRL();
    Writ_Bus(dat);
}

void ST7789::Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    WR_REG(0x2A); // Column address set
    WR_DATA(x1 + 53);
    WR_DATA(x2 + 53);
    WR_REG(0x2B); // Row address set
    WR_DATA(y1 + 40);
    WR_DATA(y2 + 40);
    WR_REG(0x2C); // Memory write
}

void ST7789::spi_config()
{
    // Initialize SPI pins
    gpio_init(pin_sck_);
    gpio_set_function(pin_sck_, GPIO_FUNC_SPI);

    gpio_init(pin_mosi_);
    gpio_set_function(pin_mosi_, GPIO_FUNC_SPI);

    // Configure SPI port with a frequency suitable for ST7789
    uint32_t freq = clock_get_hz(clk_sys);
    spi_init(spi_port_, freq);

    spi_set_format(spi_port_,
                   8,            // data_bits
                   SPI_CPOL_0,   // cpol
                   SPI_CPHA_0,   // cpha
                   SPI_MSB_FIRST // order
    );

    // Configure DMA
    dma_tx_ = dma_claim_unused_channel(true);
    dma_cfg_ = dma_channel_get_default_config(dma_tx_);
    channel_config_set_transfer_data_size(&dma_cfg_, DMA_SIZE_8);
    channel_config_set_dreq(&dma_cfg_, spi_get_dreq(spi_port_, true));

    CS_Set();
}

void ST7789::Init()
{
    RST_Clr();
    sleep_ms(200);
    RST_Set();
    sleep_ms(20);

    WR_REG(0x01); // reboot
    sleep_ms(20);
    WR_REG(0x11); // Sleep Out

    WR_REG(0x3A);           // Interface Pixel Format
    Write_Data(0b00000101); // 18 bits/pixel

    WR_REG(0x36);           // Memory Data Access Control
    Write_Data(0b01000000); // Adjust as needed for rotation

    WR_REG(0xC6); // Frame Rate Control in Normal Mode
    Write_Data(0);

    WR_REG(0x21); // Display Inversion ON
    WR_REG(0x13); // Normal display on

    WR_REG(0x29); // Display ON
}

void ST7789::Display(uint16_t *lut)
{
    Address_Set(0, 0, width_ - 1, height_ - 1);
    while (spi_is_busy(spi_port_))
        tight_loop_contents();
    DC_DATA();
    CS_Clr();

    int size = width_ * height_;
    uint8_t *pixel = image_;

    uint16_t *bufferSend;
    while (size > 0)
    {

        int sendPixels = MIN(size, buffSize);

        for (int x = 0; x < sendPixels; x++)
        {
            uint8_t val = *pixel++;
            // line_buffer_a[x] = rand();
            line_buffer_a[x] = lut[val];
        }
        // Wait for previous DMA transfer to complete
        dma_channel_wait_for_finish_blocking(dma_tx_);

        bufferSend = line_buffer_a;

        // Configure and start DMA transfer
        dma_channel_configure(
            dma_tx_,
            &dma_cfg_,
            &spi_get_hw(spi_port_)->dr, // Write address (SPI data register)
            bufferSend,                 // Read address (line buffer)
            sendPixels * 2,             // Number of bytes to transfer
            true                        // Start immediately
        );
        size -= sendPixels;
        uint16_t *tmp = line_buffer_a;
        line_buffer_a = line_buffer_b;
        line_buffer_b = tmp;
    }

    // Wait for the last DMA transfer to complete
    dma_channel_wait_for_finish_blocking(dma_tx_);
    while (spi_is_busy(spi_port_))
        tight_loop_contents();
    CS_Set();
}
