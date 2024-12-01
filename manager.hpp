#pragma once
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "ST7789/ST7789.hpp"
#include "TurboGFX.hpp"
#include "DataGatherer.hpp"
class Manager_c
{
private:
    // Constants
    static constexpr const uint8_t PIN_BL_PIN = 15;
    static constexpr const uint8_t PIN_BUTTON_UP = 17;
    static constexpr const uint8_t PIN_BUTTON_DOWN = 16;
    static constexpr const uint8_t PIN_BUFF_EN = 8;

    const uint32_t DEBOUNCE_TIME = 5000;       // 5 ms debounce time in microseconds
    const uint32_t LONG_PRESS_TIME = 1000000;  // 1 second for long press in microseconds
    const uint32_t DOUBLE_PRESS_TIME = 500000; // 0.5 second for double press in microseconds

    class ButtonState
    {
    private:
        uint8_t pin;                   // Pin number for the button
        uint64_t last_change_time = 0; // Time when the state last changed (for debouncing)
        uint64_t press_start_time = 0; // Time when the button was pressed
        Manager_c *mi_manager;         // Pointer to the manager instance
        bool long_press_state = false; // True if button is held down
        bool allEventBlocker = false;

    public:
        void init(uint8_t _pin, Manager_c *_mi_manager)
        {
            pin = _pin;
            mi_manager = _mi_manager;
            gpio_init(pin);
            gpio_set_dir(pin, GPIO_IN);
        }

        void update()
        {
            short_press_event = false;
            long_press_event = false;

            bool start_state = hold_indicator;
            bool cur_state = !gpio_get(pin);
            uint64_t cur_time = time_us_64();

            if (cur_state != hold_indicator && cur_time - last_change_time > 5000)
                hold_indicator = cur_state;

            if (cur_state != hold_indicator)
                last_change_time = cur_time;

            if (!start_state && hold_indicator) // pressed
            {
                if (mi_manager->dual_press_count == 1)
                {
                    mi_manager->dual_press_event = true;
                }
                mi_manager->dual_press_count++;
                press_start_time = cur_time;
            }
            if (start_state && !hold_indicator) // pressed
                mi_manager->dual_press_count--;

            if (mi_manager->dual_press_count == 2)
            {
                allEventBlocker = true;
            }
            if (mi_manager->dual_press_count == 0 && allEventBlocker)
            {

                allEventBlocker = false;
                return;
            }
            if (allEventBlocker)
                return;

            bool long_press = cur_time - press_start_time > 400000;

            if (start_state && !hold_indicator && !long_press)
                short_press_event = true;

            if (hold_indicator && (long_press && !long_press_state))
                long_press_event = true;

            long_press_state = long_press;
            long_press_indicator = long_press && hold_indicator;

            if (hold_indicator && (cur_time - press_start_time) <= 400000)
                long_press_progress = uint64_t((cur_time - press_start_time) / 4000);
            else
                long_press_progress = 0;
        }
        bool long_press_indicator = false; // True if button is held down

        bool hold_indicator = false;
        bool short_press_event = false;
        bool long_press_event = false;
        bool long_press_overwrite = false;
        uint8_t long_press_progress = 0;
    };

    uint slice_num_bl; // PWM slice number for backlight
    uint64_t lastFrameTime;

public:
    bool dual_press_event = false;
    uint8_t dual_press_count = 0;
    float deltaTime = 0;

    ButtonState buttons[2];

    bool digital_inputs[8];
    float analog_input[4];

    ST7789 *display;
    Graphics *gfx;
    DataGatherer &data_maker = DataGatherer::getInstance();

    void init()
    {

        // for (uint8_t i = 0; i < 256; i++)
        //{
        //     uint8_t v[3];
        //     v[0] = rand();
        //     v[1] = rand();
        //     v[2] = rand();
        //
        //    gfx->lut.AddColor(v);
        //}
        // Initialize backlight PWM (8-bit)
        gpio_init(PIN_BL_PIN); // Initialize GPIO
        gpio_set_dir(PIN_BL_PIN, GPIO_OUT);
        gpio_put(PIN_BL_PIN, 1);

        // Initialize buffer enable pin
        gpio_init(PIN_BUFF_EN);
        gpio_set_dir(PIN_BUFF_EN, GPIO_OUT);
        gpio_put(PIN_BUFF_EN, 1);

        buttons[0]
            .init(17, this);
        buttons[1].init(16, this);

        // Initialize digital inputs
        for (auto i = 0; i < 8; i++)
        {
            gpio_init(i);
            gpio_set_dir(i, GPIO_IN);
            gpio_pull_down(i);
        }

        // Initialize ADC
        adc_init();
        for (int i = 0; i < 4; i++)
        {
            adc_gpio_init(26 + i);
        }
        display = new ST7789(14, 13, 9, 10, 11, spi1, 135, 240);
        gfx = new Graphics(display->GetImageBuffer(), 240, 135);

        data_maker.init();
        gfx->clear();
        gfx->lut.Clear();

        lastFrameTime = time_us_64();
    }

    void set_backlight_brightness(uint8_t brightness)
    {
        pwm_set_chan_level(slice_num_bl, PWM_CHAN_A, brightness);
    }

    void set_adc_clock(uint32_t freq)
    {
        float clkdiv = 48000000.0f / freq;
        adc_set_clkdiv(clkdiv);
    }

    void pool_digital()
    {
        for (auto i = 0; i < 8; i++)
            digital_inputs[i] = !gpio_get(i);
    }

    void pool_analog()
    {
        for (int i = 0; i < 4; i++)
        {
            adc_select_input(i);
            uint16_t raw = adc_read();
            analog_input[i] = raw * 3.3f / 4095.0f;
        }
    }
    void handle_buttons()
    {
        dual_press_event = false;
        for (auto &button : buttons)
        {
            button.update();
        }
    }

    void UpdateScreen()
    {
        display->Display((gfx->lut.lut_table));

        uint64_t newFrameTime = time_us_64();
        deltaTime = (double(newFrameTime) - double(lastFrameTime)) / 1000000;
        lastFrameTime = newFrameTime;
    }
    float curTime_S()
    {
        return (double(double(time_us_64()) / 1000000));
    }
    bool BackAction()
    {
        return dual_press_event;
    }
    bool ConfirmAction()
    {
        return buttons[0].long_press_event || buttons[1].long_press_event;
    }
};

Manager_c manager;