#pragma once
#include <TurboGFX.hpp>
#include <manager.hpp>
#include "hardware/i2c.h"
#include "hardware/irq.h"

namespace ProtocolMaster
{
    class SSD1306Emulator
    {
        uint8_t framebuffer[8 * 128];

    public:
        SSD1306Emulator(I2C_DRIVER_C *_driver, uint8_t slave_addr)
            : driver(_driver), address(slave_addr), display_on(false),
              invert_display(false), contrast(0x7F), expecting_control_byte(true),
              waiting_for_contrast(false), current_page(0), current_col(0)
        {
            clear_framebuffer();
            instance = this;
            driver->setSlave(slave_addr, i2c_irq_handler);
        }

        bool COMMAND_Flag = false;
        volatile bool WRITE_Flag = false;

        static void i2c_irq_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
        {
            switch (event)
            {
            case I2C_SLAVE_RECEIVE:

                if (instance->expecting_control_byte)
                {
                    instance->expecting_control_byte = false;
                    instance->COMMAND_Flag = i2c_read_byte_raw(i2c) == 0;
                }
                else if (instance->COMMAND_Flag)
                {
                    instance->handle_command(i2c_read_byte_raw(i2c));
                }
                else
                {
                    instance->WRITE_Flag = true;
                    instance->handle_data(i2c_read_byte_raw(i2c));
                }

                break;

            case I2C_SLAVE_REQUEST:
                i2c_write_byte_raw(i2c, 0xFA);
                break;

            case I2C_SLAVE_FINISH:
                instance->expecting_control_byte = true;
                instance->current_page = 0;
                instance->current_col = 0;
                instance->WRITE_Flag = false;

                break;

            default:
                break;
            }
        }

        void print_framebuffer(uint8_t x, uint8_t y)
        {
            uint64_t startT = time_us_64();
            while (WRITE_Flag && time_us_64() - startT < 10000)
            {
                tight_loop_contents();
            }
            // memcpy(framebufferB, framebufferA, sizeof(framebufferA));

            uint8_t color = LUT_C::IVORY_WHITE;
            if (!display_on)
                color = LUT_C::ROSE_RED;

            for (int page = 0; page < 8; ++page)
            {
                for (int bit = 0; bit < 8; ++bit)
                {
                    for (int col = 0; col < 128; ++col)
                    {
                        uint8_t pixel = (framebuffer[page + col * 8] >> bit) & 0x01;
                        if (invert_display)
                            pixel = !pixel;

                        if (pixel)
                            manager.gfx->drawPixel(col + x + 1, page * 8 + bit + y + 1, color);
                        else
                            manager.gfx->drawPixel(col + x + 1, page * 8 + bit + y + 1, LUT_C::ONYX_BLACK);
                    }
                }
            }
            manager.gfx->drawRect(x, y, 128 + 2, 64 + 2, color);
        }

        static SSD1306Emulator *instance;
        I2C_DRIVER_C *driver;

    private:
        uint8_t address;
        bool display_on;
        bool invert_display;
        uint8_t contrast;
        bool expecting_control_byte;
        bool waiting_for_contrast;
        int current_page;
        int current_col;

        int column_end;
        int page_end;

        void clear_framebuffer()
        {
            for (int page = 0; page < 8; ++page)
            {
                for (int col = 0; col < 128; ++col)
                {
                    framebuffer[page + col * 8] = 0x00;
                }
            }
            current_page = 0;
            current_col = 0;
        }
        uint8_t pending_command = 0;
        uint8_t address_buffer[4]; // To store column/page address parameters
        int address_index = 0;
        void handle_command(uint8_t command)
        {

            if (waiting_for_contrast)
            {
                contrast = command;
                waiting_for_contrast = false;
                return;
            }

            // Handle commands that require multiple bytes
            if (pending_command == 0x21 || pending_command == 0x22)
            {
                address_buffer[address_index++] = command;
                if ((pending_command == 0x21 && address_index == 2) || // Column address
                    (pending_command == 0x22 && address_index == 2))   // Page address
                {
                    if (pending_command == 0x21)
                    {
                        current_col = address_buffer[0];
                        column_end = address_buffer[1];
                    }
                    else if (pending_command == 0x22)
                    {
                        current_page = address_buffer[0];
                        page_end = address_buffer[1];
                    }

                    // Reset pending command
                    pending_command = 0;
                    address_index = 0;
                }
                return;
            }

            // Handle regular commands
            switch (command)
            {
            case 0xAE: // Display OFF
                display_on = false;
                break;
            case 0xAF: // Display ON
                display_on = true;
                break;
            case 0xA6: // Normal display
                invert_display = false;
                break;
            case 0xA7: // Invert display
                invert_display = true;
                break;
            case 0x81: // Set contrast
                waiting_for_contrast = true;
                break;
            case 0x21: // Set column address
            case 0x22: // Set page address
                pending_command = command;
                address_index = 0;
                break;
            default:
                // Unhandled commands
                break;
            }
        }

        void handle_data(uint8_t data)
        {
            if (current_page >= 0 && current_page < 8 && current_col >= 0 && current_col < 128)
            {
                framebuffer[current_page + (current_col++) * 8] = data;
                if (current_col > column_end)
                {
                    current_col = 0; // Wrap to the starting column
                    current_page++;
                    if (current_page > page_end)
                    {
                        current_page = 0; // Wrap to the starting page
                    }
                }
            }
        }
    };
    SSD1306Emulator *SSD1306Emulator::instance = nullptr;

    class SSD1306Peripheral
    {
    public:
        uint16_t SSD1306_HEIGHT = 64;
        uint8_t address = 0x3C;

        SSD1306Peripheral(I2C_DRIVER_C *_driver)
            : driver(_driver), buffer{}
        {
            driver->setMaster();
            init();
        }

        void init()
        {
            static const uint8_t init_sequence[] = {
                0xAE,
                0xD5, 0x80,
                0xA8, 0x3F,
                0xD3, 0x00,
                0x40,
                0x8D, 0x14,
                0x20, 0x00,
                0xA1,
                0xC8,
                0xDA, 0x12,
                0x81, 0xCF,
                0xD9, 0xF1,
                0xDB, 0x40,
                0xA4,
                0xA6,
                0x2E,
                0xAF};

            sendCommand(init_sequence, sizeof(init_sequence));
            clear();
            update();
        }

        void clear()
        {
            memset(buffer, 0, sizeof(buffer));
        }

        void drawPixel(int x, int y, bool on)
        {
            if (x < 0 || x >= 128 || y < 0 || y >= SSD1306_HEIGHT)
                return;

            if (on)
                buffer[x + (y / 8) * 128] |= (1 << (y % 8));
            else
                buffer[x + (y / 8) * 128] &= ~(1 << (y % 8));
        }

        void update()
        {
            static const uint8_t set_column_address[] = {0x21, 0x00, 128 - 1};
            static const uint8_t set_page_address[] = {0x22, 0x00, (SSD1306_HEIGHT / 8) - 1};

            sendCommand(set_column_address, sizeof(set_column_address));
            sendCommand(set_page_address, sizeof(set_page_address));

            uint8_t data[1 + sizeof(buffer)];
            data[0] = 0x40; // Co=0, D/C#=1
            memcpy(&data[1], buffer, sizeof(buffer));
            sendData(data, sizeof(data));
        }

        void print_framebuffer(uint8_t x, uint8_t y) const
        {
            uint8_t color = LUT_C::IVORY_WHITE;
            for (int page = 0; page < 8; ++page)
            {
                for (int bit = 0; bit < 8; ++bit)
                {
                    for (int col = 0; col < 128; ++col)
                    {
                        uint8_t pixel = (buffer[page * 128 + col] >> bit) & 0x01;

                        if (pixel)
                            manager.gfx->drawPixel(col + x + 1, page * 8 + bit + y + 1, color);
                        else
                            manager.gfx->drawPixel(col + x + 1, page * 8 + bit + y + 1, LUT_C::ONYX_BLACK);
                    }
                }
            }
            manager.gfx->drawRect(x, y, 128 + 2, 64 + 2, color);
        }

    private:
        I2C_DRIVER_C *driver;
        uint8_t buffer[128 * 8];

        void sendCommand(const uint8_t *commands, size_t len)
        {
            uint8_t data[len + 1];
            data[0] = 0x00; // Co=0, D/C#=0
            memcpy(&data[1], commands, len);
            driver->write_data(address, data, len + 1);
        }

        void sendData(const uint8_t *data, size_t len)
        {
            driver->write_data(address, data, len);
        }
    };
}
