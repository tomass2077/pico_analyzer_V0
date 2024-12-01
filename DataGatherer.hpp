#pragma once
#include "pico/stdlib.h"
#include <pico/multicore.h>
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include <hardware/sync.h>
uint32_t pularTime = 0;
bool pulval = 0;

uint8_t makeFakeGpio(uint32_t time)
{
    if (time - pularTime > 250000)
    {
        pulval = rand() % 2;
        pularTime = time;
    }
    return (0b1 | (time % 1000000 < 500000) * 4 | (time % 1000000 < 100000) * 8 |
            (rand() % 1000 < 1) * 16 |
            (rand() % 1000 < 5000) * 32 |
            (time - pularTime > 250000 / 2) * 64 |
            pulval * 128);
}
class DataGatherer
{
private:
    volatile uint32_t interval_us = 1;
    volatile bool analogMode = false;
    volatile bool gatherForceStop = false;
    volatile bool gatherRunning = false;
    volatile bool enableGathering = false;

    float analog_input[4];
    uint8_t *pinStateBufferA;
    uint8_t *pinStateBufferB;

    uint16_t pinStateBufferPtr = 0;
    uint16_t samplesToGather = 0;

public:
    static DataGatherer &getInstance()
    {
        static DataGatherer instance;
        return instance;
    }

    void init()
    {

        // Initialize digital inputs
        for (auto i = 0; i < 8; i++)
        {
            gpio_init(i);
            gpio_set_dir(i, GPIO_IN);
        }

        adc_init();
        for (int i = 0; i < 4; i++)
        {
            adc_gpio_init(26 + i);
        }

        multicore_launch_core1(core1task_wrapper);

        pinStateBufferA = (uint8_t *)malloc(10000);
        pinStateBufferB = (uint8_t *)malloc(10000);
    }
    void StartDigitalGeather(uint16_t samples)
    {
        stopGather();
        gatherForceStop = false;
        analogMode = false;
        samplesToGather = samples;
        enableGathering = true;

        while (!gatherRunning)
        {
            tight_loop_contents(); // Wait for event
        }
    }
    uint8_t *GetDataBuffer()
    {
        return pinStateBufferB;
    }

    void WaitForGeatherFinished()
    {
        while (gatherRunning)
        {
            tight_loop_contents();
        }
    }
    void stopGather()
    {
        gatherForceStop = true;
        while (gatherRunning)
        {
            tight_loop_contents();
        }
    }
    uint16_t GetGeatherProgress()
    {
        return (pinStateBufferPtr);
    }
    void SetInterval(uint32_t us)
    {
        interval_us = us;
    }

private:
    static void core1task_wrapper()
    {
        getInstance().core1task();
    }

    void core1task()
    {
        while (true)
        {
            /* code */

            while (!enableGathering)
            {
                gatherRunning = false;
                tight_loop_contents();
            }

            enableGathering = false;
            gatherRunning = true;

            if (!analogMode)
            {
                // Digital mode data gathering
                pinStateBufferPtr = 0;
                while (pinStateBufferPtr < samplesToGather && !gatherForceStop)
                {
                    uint8_t out = 0;
                    uint32_t start_time = time_us_32();

                    // OR-ing GPIO states within the interval to capture short pulses
                    while (time_us_32() - start_time < interval_us)
                    {
                        uint32_t current_time = time_us_32();
                        out |= gpio_get_all(); // makeFakeGpio(start_time);

                        // Ensure no duplicate sampling at the same timestamp
                        while (current_time == time_us_32())
                        {
                            tight_loop_contents(); // Low-power wait
                        }
                    }

                    // Store the result in the buffer
                    pinStateBufferA[pinStateBufferPtr++] = out;
                }

                // Swap buffers
                uint8_t *tmp = pinStateBufferA;
                pinStateBufferA = pinStateBufferB;
                pinStateBufferB = tmp;
            }
            else
            {
            }
        }
    }
};