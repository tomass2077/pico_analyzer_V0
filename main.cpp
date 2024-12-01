#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/sleep.h"
#include "hardware/uart.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/clocks.h"

#include "pico/bootrom.h"
#include "manager.hpp"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <hardware/xosc.h>
#include "mainMenu.hpp"
static constexpr const char *NAME = "Pico Logic";

int main()
{
    stdio_init_all();
    xosc_init();
    set_sys_clock_khz(125000, true);

    manager.init();

    while (1)
    {
        // for (int i = 0; i < 64; i++)
        //{
        //     manager.gfx->drawRect(i * 3, 0, 3, 240, i, true);
        // }
        // manager.UpdateScreen();
        // printf("UPD\n");
        // sleep_ms(100);
        MainMenu();
    }

    return 0;
}
