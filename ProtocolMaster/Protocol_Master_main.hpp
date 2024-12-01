#pragma once
#include <TurboGFX.hpp>
#include <manager.hpp>
#include "ProtocolMaster/Protocol_Master_I2C.hpp"
namespace ProtocolMaster
{
    void Main()
    {
        Graphics *gfx = manager.gfx;
        manager.handle_buttons();

        while (!manager.BackAction())
        {
            /* code */

            gfx->clear();

            const char *menuItems[11] = {
                "I2C",
                "SPI",
                "UART",
                "I2S",
                "1-Wire",
                "DMX512",
                "JTAG",
                "MIDI",
                "PS2",
                "SWD",
                "DHT2x",
            };
            MenuDrawer menu(menuItems, 11);
            while (!manager.BackAction())
            {
                manager.gfx->clear();
                manager.handle_buttons();
                menu.draw();
                if (manager.ConfirmAction())
                {
                    if (menu.cur_item == 0)
                    {
                        ProtocolMaster::I2C_main();
                    }
                    manager.handle_buttons();
                }
                else
                    manager.UpdateScreen();
            }
        }
    }
}