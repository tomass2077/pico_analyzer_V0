#include "ST7735.hpp"
#include <hardware/clocks.h>

ST7735::ST7735(uint8_t pin_dc, uint8_t pin_rst, uint8_t pin_cs,
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
    image_ = new uint16_t[width_ * height_];
    line_buffer_ = new uint8_t[width_ * 2]; // 2 bytes per pixel (16-bit color)

    // Initialize SPI and LCD
    spi_config();
    Init();
}

ST7735::~ST7735()
{
    delete[] image_;
    delete[] line_buffer_;
}

uint16_t *ST7735::GetImageBuffer()
{
    return image_;
}

void ST7735::Writ_Bus(uint8_t dat)
{
    CS_Clr();
    spi_write_blocking(spi_port_, &dat, 1);
    CS_Set();
}

void ST7735::Write_Data(uint8_t dat)
{
    DC_DATA();
    Writ_Bus(dat);
}

void ST7735::WR_DATA(uint16_t dat)
{
    DC_DATA();
    uint8_t buf[2] = {static_cast<uint8_t>(dat >> 8), static_cast<uint8_t>(dat & 0xFF)};
    CS_Clr();
    spi_write_blocking(spi_port_, buf, 2);
    CS_Set();
}

void ST7735::WR_REG(uint8_t dat)
{
    DC_CTRL();
    Writ_Bus(dat);
}

void ST7735::Address_Set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    WR_REG(0x2A); // Column address set
    WR_DATA(x1);
    WR_DATA(x2);
    WR_REG(0x2B); // Row address set
    WR_DATA(y1 + 24);
    WR_DATA(y2 + 24);
    WR_REG(0x2C); // Memory write
}

void ST7735::spi_config()
{
    // Initialize SPI pins
    gpio_init(pin_sck_);
    gpio_set_function(pin_sck_, GPIO_FUNC_SPI);

    gpio_init(pin_mosi_);
    gpio_set_function(pin_mosi_, GPIO_FUNC_SPI);

    // Configure SPI port
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

void ST7735::Init()
{
    RST_Clr();
    sleep_ms(200);
    RST_Set();
    sleep_ms(20);

    WR_REG(0x11); // Exit sleep mode
    sleep_ms(100);

    WR_REG(0x20); // Display inversion mode

    // Initialization commands (same as the original code)
    WR_REG(0xB1);
    Write_Data(0x05);
    Write_Data(0x3A);
    Write_Data(0x3A);

    WR_REG(0xB2);
    Write_Data(0x05);
    Write_Data(0x3A);
    Write_Data(0x3A);

    WR_REG(0xB3);
    Write_Data(0x05);
    Write_Data(0x3A);
    Write_Data(0x3A);
    Write_Data(0x05);
    Write_Data(0x3A);
    Write_Data(0x3A);

    WR_REG(0xB4);
    Write_Data(0x03);

    WR_REG(0xC0);
    Write_Data(0x62);
    Write_Data(0x02);
    Write_Data(0x04);

    WR_REG(0xC1);
    Write_Data(0xC0);

    WR_REG(0xC2);
    Write_Data(0x0D);
    Write_Data(0x00);

    WR_REG(0xC3);
    Write_Data(0x8D);
    Write_Data(0x6A);

    WR_REG(0xC4);
    Write_Data(0x8D);
    Write_Data(0xEE);

    WR_REG(0xC5); // VCOM
    Write_Data(0x0E);

    WR_REG(0xE0);
    Write_Data(0x10);
    Write_Data(0x0E);
    Write_Data(0x02);
    Write_Data(0x03);
    Write_Data(0x0E);
    Write_Data(0x07);
    Write_Data(0x02);
    Write_Data(0x07);
    Write_Data(0x0A);
    Write_Data(0x12);
    Write_Data(0x27);
    Write_Data(0x37);
    Write_Data(0x00);
    Write_Data(0x0D);
    Write_Data(0x0E);
    Write_Data(0x10);

    WR_REG(0xE1);
    Write_Data(0x10);
    Write_Data(0x0E);
    Write_Data(0x03);
    Write_Data(0x03);
    Write_Data(0x0F);
    Write_Data(0x06);
    Write_Data(0x02);
    Write_Data(0x08);
    Write_Data(0x0A);
    Write_Data(0x13);
    Write_Data(0x26);
    Write_Data(0x36);
    Write_Data(0x00);
    Write_Data(0x0D);
    Write_Data(0x0E);
    Write_Data(0x10);

    WR_REG(0x3A);     // Define the format of RGB picture data
    Write_Data(0x05); // 16-bit/pixel

    WR_REG(0x36);
    Write_Data(0x68); // Rotation

    WR_REG(0x29); // Display On
}

void ST7735::Display()
{
    Address_Set(0, 0, width_, height_);
    DC_DATA();
    CS_Clr();

    for (uint16_t y = 0; y < height_; y++)
    {
        uint16_t *line = image_ + y * width_;
        for (uint16_t x = 0; x < width_; x++)
        {
            uint16_t pixel = line[x];
            // Swap bytes for SPI transfer
            line_buffer_[x * 2] = static_cast<uint8_t>(pixel >> 8);
            line_buffer_[x * 2 + 1] = static_cast<uint8_t>(pixel & 0xFF);
        }

        // Wait for previous DMA transfer to complete
        dma_channel_wait_for_finish_blocking(dma_tx_);

        // Configure and start DMA transfer
        dma_channel_configure(
            dma_tx_,
            &dma_cfg_,
            &spi_get_hw(spi_port_)->dr, // Write address (SPI data register)
            line_buffer_,               // Read address (line buffer)
            width_ * 2,                 // Number of bytes to transfer
            true                        // Start immediately
        );
    }

    // Wait for the last DMA transfer to complete
    dma_channel_wait_for_finish_blocking(dma_tx_);
    CS_Set();
}
