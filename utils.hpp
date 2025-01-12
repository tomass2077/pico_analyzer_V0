#pragma once

#include "manager.hpp"
#include <pico/i2c_slave.h>

class SmoothValue
{
    bool TMP_BOOST = false;

public:
    float value;
    float maxVelocity;

    SmoothValue(float initialValue, float maxVelocity)
        : value(initialValue), maxVelocity(maxVelocity) {}

    void TMP_SPEED()
    {
        if (!TMP_BOOST)
        {
            maxVelocity *= 2;
            TMP_BOOST = true;
        }
    }
    void update(int target, float deltaTime)
    {
        // Calculate the difference between target and current value
        float difference = target - value;

        // Determine the step (movement) based on the maximum velocity
        float step = maxVelocity * deltaTime;

        // Move toward the target without overshooting
        if (std::fabs(difference) <= step)
        {
            // Close enough, snap to the target
            if (TMP_BOOST)
            {
                maxVelocity /= 2;
                TMP_BOOST = false;
            }
            value = target;
        }
        else
        {
            // Move towards the target with limited velocity
            value += (difference > 0 ? step : -step);
        }
    }
};
class MenuDrawer
{
    const char **menuElements;
    SmoothValue smoothItem;

public:
    uint8_t elementCount;
    uint8_t cur_item = 0;

    MenuDrawer(const char *menuElements[], uint8_t elementCount)
        : menuElements(menuElements),
          elementCount(elementCount),
          smoothItem(0, 10)
    {
        cur_item = 0;
    }
    void jump(uint8_t v)
    {
        smoothItem.TMP_SPEED();
        cur_item = v;
    }

    void draw()
    {

        if (manager.buttons[1].short_press_event)
        {
            if (cur_item < elementCount - 1)
                cur_item++;
            else
            {
                smoothItem.TMP_SPEED();
                cur_item = 0;
            }
        }
        if (manager.buttons[0].short_press_event)
        {
            if (cur_item > 0)
                cur_item--;
            else
            {
                smoothItem.TMP_SPEED();
                cur_item = elementCount - 1;
            }
        }
        uint16_t okProgress = (MAX(50, MAX(manager.buttons[0].long_press_progress, manager.buttons[1].long_press_progress)) - 50) * 2;
        uint16_t height = manager.gfx->height;

        manager.gfx->drawLineAA(4, float(height - okProgress) / 2, 4, float(height + okProgress) / 2, 0);

        smoothItem.update(cur_item, manager.deltaTime);
        // manager.gfx->drawTriangleAA(5, height / 2 - 4, 15, height / 2, 8, height / 2, 7, true);
        // manager.gfx->drawTriangleAA(5, height / 2 + 4, 15, height / 2, 8, height / 2, 7, true);

        for (uint8_t i = 0; i < elementCount; i++)
        {
            float diff = abs(smoothItem.value - i);
            uint8_t liteness = MIN(5, diff);
            manager.gfx->drawString(MAX(0, 1 - diff) * 20 + 10, height / 2 - 8 + i * 20 - smoothItem.value * 20, menuElements[i], liteness);
        }
    }
};

void progress_bar(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t progress)
{

    // ROSE_RED
    // PEACH
    // MEADOW_GREEN
    int fullWProg = w + h;
    for (int W = 0; W < w; W++)
    {
        for (int H = 0; H < h; H++)
        {

            int WProg = W + H;
            if (float(WProg) / fullWProg * 100 < progress)
                if (WProg % 20 < 15)
                {
                    WProg = WProg - WProg % 20;
                    if (WProg < fullWProg / 3)
                        manager.gfx->drawPixel(x + W, y + H, LUT_C::ROSE_RED);
                    else if (WProg < fullWProg / 3 * 2)
                        manager.gfx->drawPixel(x + W, y + H, LUT_C::PEACH);
                    else
                        manager.gfx->drawPixel(x + W, y + H, LUT_C::MEADOW_GREEN);
                }
                else
                {
                    manager.gfx->drawPixel(x + W, y + H, LUT_C::MIDNIGHT_BLUE);
                }
        }
    }
    manager.gfx->drawRect(x, y, w, h, manager.gfx->lut.IVORY_WHITE, false);
}
#include <cstring>
#include <hardware/i2c.h>

class LOG_BUFFER_C
{
    static const uint16_t LINE_COUNT = 15;  // Number of lines in the buffer
    static const uint16_t LINE_LENGTH = 25; // Maximum length of each line

    char lines[LINE_COUNT][LINE_LENGTH]; // Buffer to store lines
    uint16_t start = 0;                  // Index of the oldest line
    uint16_t count = 0;                  // Number of lines currently stored

public:
    LOG_BUFFER_C()
    {
        // Initialize all lines to empty strings
        for (uint16_t i = 0; i < LINE_COUNT; ++i)
        {
            lines[i][0] = '\0';
        }
    }

    void AddLine(const char *msg)
    {
        // Calculate the index where the new message will be stored
        uint16_t index = (start + count) % LINE_COUNT;

        // Copy the message into the buffer, ensuring it fits
        strncpy(lines[index], msg, LINE_LENGTH - 1);
        lines[index][LINE_LENGTH - 1] = '\0'; // Ensure null termination

        if (count < LINE_COUNT)
        {
            // Buffer not full yet
            count++;
        }
        else
        {
            // Buffer full, overwrite the oldest line
            start = (start + 1) % LINE_COUNT;
        }
    }

    uint16_t GetLineCount() const
    {
        return count;
    }

    const char *GetLineFromEnd(uint16_t n) const
    {
        if (n >= count)
        {
            // Requested line is out of range
            return "OUT_OF_BUFFER\0"; // Return null pointer to indicate invalid request
        }

        // Calculate the index of the requested line
        uint16_t index = (start + count - n - 1) % LINE_COUNT;

        return lines[index]; // Safely return the line
    }
    bool GetLineFromEnd(uint16_t n, char *outBuffer, uint16_t maxLength) const
    {
        if (n >= count)
        {
            // Requested line is out of range
            return false;
        }

        // Calculate the index of the requested line
        uint16_t index = (start + count - n - 1) % LINE_COUNT;

        // Copy the line into outBuffer, ensuring it doesn't exceed maxLength
        strncpy(outBuffer, lines[index], maxLength - 1);
        outBuffer[maxLength - 1] = '\0'; // Ensure null termination

        return true;
    }
    void clear()
    {
        start = 0;
        count = 0;
    }
};

LOG_BUFFER_C logger;

class I2C_DRIVER_C
{
public:
    I2C_DRIVER_C()
    {
    }

    void init()
    {
        i2c = i2c1;
        gpio_set_function(22, GPIO_FUNC_I2C);
        gpio_set_function(23, GPIO_FUNC_I2C);
        gpio_pull_up(22);
        gpio_pull_up(23);
        i2c_init(i2c, 400 * 1000);
    }

    void setSlave(uint8_t address, i2c_slave_handler_t aa)
    {
        if (Is_Slave)
        {
            i2c_slave_deinit(i2c);
            i2c_deinit(i2c);
            i2c_init(i2c, 1000 * 1000);
        }
        i2c_slave_init(i2c, address, aa);
        Is_Slave = true;
    }
    void setMaster()
    {
        if (Is_Slave)
        {
            i2c_slave_deinit(i2c);
            i2c_deinit(i2c);
            i2c_init(i2c, 400 * 1000);
        }
        Is_Slave = false;
    }
    __force_inline void write_data(uint8_t address, const uint8_t *data, size_t len)
    {
        i2c_write_blocking(i2c, address, data, len, false);
    }
    __force_inline int read_data(uint8_t address, uint8_t *data, size_t len)
    {
        return (i2c_read_blocking(i2c1, address, data, len, false));
    }
    void deinit()
    {
        if (Is_Slave)
        {
            i2c_slave_deinit(i2c);
            sleep_ms(1); // Just make sure that any interrupt are cleared
        }

        i2c_deinit(i2c);
        gpio_deinit(22);
        gpio_deinit(23);
        gpio_disable_pulls(22);
        gpio_disable_pulls(23);
        i2c = nullptr;
    }

private:
    i2c_inst_t *i2c;
    bool Is_Slave = false;
};
