#pragma once
#include "manager.hpp"
#include "utils.hpp"
#include "ProtocolMaster/Protocol_Master_main.hpp"
void TestScreen()
{
    manager.gfx->clear();
    manager.handle_buttons();
    float time = 0;
    while (!manager.BackAction())
    {
        manager.gfx->clear();
        manager.handle_buttons();

        char cc[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L'};
        char cn[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

        for (int i = 0; i < 12; i++)
        {
            for (int j = 0; j < 5; j++)
            {
                char buf[3];
                buf[0] = cn[j];
                buf[1] = cc[i];
                buf[2] = 0;
                manager.gfx->drawString(i * 20 + 2, j * 27 + 5, buf, int((time * 15 + (j * 12 + i)) / 10) % 29);
                manager.gfx->drawRect(i * 20, j * 27, 20, 27, int((time * 15 + (j * 12 + i)) / 10) % 29);
                manager.gfx->drawRect(i * 20 + 1, j * 27 + 1, 20 - 2, 27 - 2, int((time * 15 + (j * 12 + i)) / 10) % 29);
            }
        }
        manager.UpdateScreen();
        time += manager.deltaTime;
    }
}
#define histies 100
class averagere
{
    uint16_t hist[histies];
    uint16_t pt = 0;
    uint16_t out = 0;

public:
    void pushV(uint16_t val)
    {
        out = (out + val) / 2;
        hist[pt] = val;
        pt = (pt + 1) % histies;
    }
    uint16_t getAvr()
    {
        // return (out);
        uint32_t sumie = 0;
        for (uint8_t i = 0; i < histies; i++)
        {
            sumie += hist[i];
        }
        return (sumie / histies);
    }
};
// void VirtualLed()
// {
// manager.gfx->clear(0);
// manager.handle_buttons();
// averagere averageDouty[8];
// averagere averagSimps;
//
// while (!manager.BackAction())
// {
//
// manager.data_maker.stopGather();
// uint16_t geatherProgress = manager.data_maker.GetGeatherProgress();
// uint8_t *geatherBuffer = manager.data_maker.GetDataBuffer();
// manager.data_maker.SetInterval(100);
//
// manager.data_maker.StartDigitalGeather(1000);
//
// uint32_t CurTime = time_us_32();
// manager.gfx->clear(0);
// manager.handle_buttons();
//
// uint16_t HighForPin[8] = {0}; // Initializes all elements to zero
// for (uint16_t i = 0; i < geatherProgress; i++)
// {
// uint8_t v = geatherBuffer[i];
// HighForPin[0] += v & 1;
// HighForPin[1] += (v >> 1) & 1;
// HighForPin[2] += (v >> 2) & 1;
// HighForPin[3] += (v >> 3) & 1;
// HighForPin[4] += (v >> 4) & 1;
// HighForPin[5] += (v >> 5) & 1;
// HighForPin[6] += (v >> 6) & 1;
// HighForPin[7] += (v >> 7) & 1;
// }
// averagSimps.pushV(geatherProgress);
// for (int i = 0; i < 8; i++)
// {
// averageDouty[i].pushV(HighForPin[i]);
// float douty = MIN(1, MAX(0, float(averageDouty[i].getAvr()) / MAX(1, float(averagSimps.getAvr()))));
// uint8_t v = 255.0 * (float(HighForPin[i]) / float(geatherProgress));
// manager.gfx->drawRect(21 + 25 * i, 102, 23, 23, {0, v, 0}, true);
//
// manager.gfx->drawRect(21 + 25 * i, 102, 23, 23, {255, (uint8_t)(HighForPin[i] > 0) * 255, (uint8_t)(HighForPin[i] > 0) * 255}, false);
//
// char buffer[64];
// int ret = snprintf(buffer, sizeof(buffer), "%i", int(douty * 100));
//
// manager.gfx->drawString(21 + 25 * i, 100 - 16, buffer, 0xffff);
// }
// manager.UpdateScreen();
// }
// }

void MainMenu()
{
    Graphics *gfx = manager.gfx;
    gfx->clear();

    manager.handle_buttons();

    const char *menuItems[5] = {"USB BOOT", "Test Screen", "Virtual LED", "Single Channel", "Protocol Master"};
    const uint8_t menuItemCount = 5;

    MenuDrawer menu(menuItems, 5);
    while (1)
    {
        manager.gfx->clear();
        manager.handle_buttons();
        menu.draw();
        if (manager.ConfirmAction())
        {
            if (menu.cur_item == 0)
            {
                reset_usb_boot(1 << 15, 0);
            }
            if (menu.cur_item == 1)
            {
                TestScreen();
            }
            if (menu.cur_item == 2)
            {
                // VirtualLed();
            }
            if (menu.cur_item == 4)
            {
                ProtocolMaster::Main();
            }
            manager.handle_buttons();
        }
        manager.UpdateScreen();
    }
}