#pragma once
#include <TurboGFX.hpp>
#include <manager.hpp>
#include "hardware/i2c.h"
#include "ProtocolMaster/i2c_defines.hpp"
#include "ProtocolMaster/ssd1306.hpp"
#include "ProtocolMaster/KnobIO.hpp"
#include "ProtocolMaster/Canvas.hpp"

namespace ProtocolMaster
{

    void I2C_HOST()
    {
        I2C_DRIVER_C driver;
        driver.init();

        Graphics *gfx = manager.gfx;
        logger.clear();

        uint8_t discovered[100];
        uint8_t discovered_index = 0;

        driver.setMaster();
        for (uint8_t address = 0x08; address < 0x77; ++address)
        {
            // Perform a write operation to see if a device responds
            uint8_t dummy_data = 0;
            gfx->clear();
            if (logger.GetLineCount() > 0)
                for (int i = 0; i < MIN(logger.GetLineCount(), 6); i++)
                {
                    gfx->drawString(10, gfx->height - 27 - 16 - i * 18, logger.GetLineFromEnd(i), LUT_C::IVORY_WHITE);
                }

            progress_bar(10, gfx->height - 25, 220, 20, uint32_t(address) * 100 / 111);
            int result = driver.read_data(address, &dummy_data, 1);
            // int result = -1;
            //  If the device responded, print the address
            if (result >= 0)
            {
                discovered[discovered_index] = address;
                discovered_index++;

                char hexStr[25];
                sprintf(hexStr, "Device at 0x%X", address);
                logger.AddLine(hexStr);
            }
            manager.UpdateScreen();
        }

        manager.handle_buttons();

        /* code */

        gfx->clear();

        char menuItems[12][20];
        uint16_t menu_devices[12];
        uint8_t menu_addresses[12];

        const char *menuItem_ptrs[12];
        for (int i = 0; i < 12; i++)
        {
            menuItem_ptrs[i] = menuItems[i];
        }
        MenuDrawer menu(menuItem_ptrs, 12);
        uint8_t curMenu = 0;
        while (!manager.BackAction())
        {
            manager.gfx->clear();
            manager.handle_buttons();
            uint8_t menuElemPtr = 0;
            if (curMenu > 0)
            {
                strncpy(menuItems[0], "Last Page", 20);
                menuElemPtr += 1;
            }

            uint16_t devsToSkip = curMenu * 10;

            for (uint8_t i = 0; i < discovered_index; i++)
            {
                uint8_t address = discovered[i];
                for (uint16_t d = 0; d < sizeof(I2C_DEFINES::i2c_discovery_devs) / sizeof(I2C_DEFINES::I2C_DISCOVERY_DEV); d++)
                {
                    I2C_DEFINES::I2C_DISCOVERY_DEV dev = I2C_DEFINES::i2c_discovery_devs[d];
                    if (dev.deviceInAddress(address))
                    {
                        if (devsToSkip)
                        {
                            devsToSkip--;
                            continue;
                        }
                        if (menuElemPtr < 12)
                        {
                            menu_addresses[menuElemPtr] = address;
                            menu_devices[menuElemPtr] = d;
                            sprintf(menuItems[menuElemPtr], "0x%X %s", address, dev.Name);
                        }

                        menuElemPtr++;
                    }
                }
            }
            if (menuElemPtr > 12)
                strncpy(menuItems[11], "Next Page", 20);

            menu.elementCount = MIN(12, menuElemPtr);

            menu.draw();
            if (manager.ConfirmAction())
            {
                if (menu.cur_item == 0 && curMenu > 0)
                {
                    curMenu--;
                }
                else if (menu.cur_item == 11 && menuElemPtr > 12)
                {
                    menu.jump(0);
                    curMenu++;
                }
                else
                {
                    if (menu_devices[menu.cur_item] == I2C_DEFINES::i2c_discovery_ENUM::SSD1306)
                    {
                        SSD1306Peripheral prei = SSD1306Peripheral(&driver);
                        manager.handle_buttons();
                        while (!manager.BackAction())
                        {
                            manager.handle_buttons();

                            manager.gfx->clear();

                            prei.clear();
                            float t = manager.curTime_S();
                            for (int i = 0; i < 128; i++)
                            {
                                prei.drawPixel(i, round((sinf(t + float(i) / 64) / 2 + .5) * 63.99), true);
                                prei.drawPixel(i, round((sinf(-t + float(i) / 64) / 2 + .5) * 63.99), true);
                            }
                            prei.print_framebuffer(0, 0);
                            prei.update();
                            manager.UpdateScreen();
                        }
                    }
                }
                manager.handle_buttons();
            }
            else
                manager.UpdateScreen();
        }
        driver.deinit();
    }
    void I2C_PERI()
    {
        I2C_DRIVER_C driver;
        driver.init();
        Graphics *gfx = manager.gfx;
        logger.clear();

        manager.handle_buttons();

        gfx->clear();

        const char *menuItems[] = {
            "SSD1306",
            "Knob IO"};

        MenuDrawer menu(menuItems, 2);
        while (!manager.BackAction())
        {
            manager.gfx->clear();
            manager.handle_buttons();

            menu.draw();
            if (manager.ConfirmAction())
            {
                if (menu.cur_item == 0)
                {

                    SSD1306Emulator prei(&driver, 0x3c);

                    manager.handle_buttons();
                    while (!manager.BackAction())
                    {
                        manager.handle_buttons();

                        manager.gfx->clear();
                        prei.print_framebuffer(0, 0);
                        manager.UpdateScreen();
                    }
                }
                else if (menu.cur_item == 1)
                {

                    KnobIO prei(&driver, 0x69);

                    manager.handle_buttons();
                    while (!manager.BackAction())
                    {
                        manager.handle_buttons();

                        manager.gfx->clear();
                        prei.draw(0, 0);
                        manager.UpdateScreen();
                    }
                }

                manager.handle_buttons();
            }
            else
                manager.UpdateScreen();
        }
        driver.deinit();
    }
    void I2C_main()
    {
        Graphics *gfx = manager.gfx;
        manager.handle_buttons();

        while (!manager.BackAction())
        {
            /* code */

            gfx->clear();

            const char *menuItems[3] = {
                "HOST",
                "PERIPHERAL",
                "DEBUG",

            };
            MenuDrawer menu(menuItems, 3);
            while (!manager.BackAction())
            {
                manager.gfx->clear();
                manager.handle_buttons();
                menu.draw();
                if (manager.ConfirmAction())
                {
                    if (menu.cur_item == 0)
                    {
                        I2C_HOST();
                    }
                    if (menu.cur_item == 1)
                    {
                        I2C_PERI();
                    }
                    manager.handle_buttons();
                }
                else
                    manager.UpdateScreen();
            }
        }
    }
}