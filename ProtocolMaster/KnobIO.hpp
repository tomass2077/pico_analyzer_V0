#pragma once
#include <TurboGFX.hpp>
#include <manager.hpp>
#include "hardware/i2c.h"
#include "hardware/irq.h"
namespace ProtocolMaster
{
    class KnobIO
    {
        LOG_BUFFER_C command_hist;
        uint8_t transfer_buff[257];
        uint8_t transfer_ptr;

    public:
        KnobIO(I2C_DRIVER_C *_driver, uint8_t slave_addr)
            : driver(_driver)
        {
            transfer_ptr = 0;
            command_hist.clear();
            instance = this;
            driver->setSlave(slave_addr, i2c_irq_handler);
        }

        static void i2c_irq_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
        {
            switch (event)
            {
            case I2C_SLAVE_RECEIVE:

                instance->transfer_buff[instance->transfer_ptr++] = i2c_read_byte_raw(i2c);
                break;

            case I2C_SLAVE_REQUEST:
                i2c_write_byte_raw(i2c, 0xFA);
                break;

            case I2C_SLAVE_FINISH:
                if (instance->transfer_ptr > 2)
                {
                    if (instance->transfer_buff[0] == 1)
                    {
                        instance->transfer_buff[instance->transfer_ptr] = '\0';
                        instance->command_hist.AddLine((char *)(&(instance->transfer_buff[1])));
                        // printf("added line\n");
                    }
                    else
                    {
                        // char hexStr[25];
                        // sprintf(hexStr, "end 0x%X not 1\n", instance->transfer_buff[0]);
                        // printf(hexStr);
                    }
                }
                instance->transfer_ptr = 0;
                break;
            default:
                break;
            }
        }

        void draw(uint8_t x, uint8_t y)
        {

            for (int i = 0; i < MIN(command_hist.GetLineCount(), 8); i++)
            {
                manager.gfx->drawString(10, manager.gfx->height - 17 - i * 17, command_hist.GetLineFromEnd(i), LUT_C::IVORY_WHITE);
            }
        }

        static KnobIO *instance;

    private:
        I2C_DRIVER_C *driver;
    };
    KnobIO *KnobIO::instance = nullptr;

}