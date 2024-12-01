#pragma once
#include <TurboGFX.hpp>
#include <manager.hpp>
#include "hardware/i2c.h"
#include "hardware/irq.h"
namespace ProtocolMaster
{
    class I2C_Canvas
    {
        LOG_BUFFER_C command_hist;
        uint8_t transfer_buff[512];
        uint16_t transfer_ptr;

        char *menu_Buffer_ptrs[32];
        char menu_Buffer_data[32 * 20];

        uint8_t menu_selected = 0;
        uint8_t menu_exit_state = 0;
        uint8_t menu_items = 0;
        MenuDrawer menu = MenuDrawer(menu_Buffer_ptrs, 0); // Corrected initialization

        bool menu_active = false;

        struct Draw_Fill
        {
            uint8_t color;
        };
        struct Draw_Text
        {
            uint8_t color;
            uint8_t x;
            uint8_t y;
            uint8_t len;
            // Followed by some uint8_t data
        };
        struct Draw_Rect // also for fill
        {
            uint8_t color;
            uint8_t x;
            uint8_t y;
            uint8_t w;
            uint8_t h;
        };
        struct Draw_Bit_Map
        {
            uint8_t x;
            uint8_t y;
            uint8_t w;
            uint16_t offset;
            uint16_t len;
            // Followed by some uint8_t data
        };
        struct Draw_Menu
        {
            uint8_t defaul_selected;
            uint16_t len;
            // Followed by some data that "len" characters, separated into lines by '\0'
        };
        enum Draw_Types
        {
            DRAW_NONE,
            DRAW_PUSH_FRAME,
            DRAW_FILL,
            DRAW_TEXT,
            DRAW_RECT,
            DRAW_RECT_FILL,
            DRAW_BIT_MAP,
            DRAW_MENU,

        };

    public:
        I2C_Canvas(I2C_DRIVER_C *_driver) : driver(_driver)
        {
            transfer_ptr = 0;
            command_hist.clear();
            instance = this;
            driver->setSlave(0x69, i2c_irq_handler);
        }

        uint8_t generate_button_map()
        {
            uint8_t out;
            if (menu_active)
            {
                out = menu_selected | menu_exit_state | 0b10000000;
                if (menu_exit_state)
                {
                    menu_exit_state = 0;
                    menu_active = false;

                    flag_button_a = false;
                    flag_button_b = false;
                    flag_button_a_long = false;
                    flag_button_b_long = false;
                    flag_confirm_action = false;
                    flag_exit_action = false;
                }
            }
            else
            {

                out = flag_button_a * 0b1 +
                      flag_button_b * 0b10 +
                      (flag_button_a_long | flag_button_a_long_hold) * 0b100 +
                      (flag_button_b_long | flag_button_b_long_hold) * 0b1000 +
                      flag_confirm_action * 0b10000 +
                      flag_exit_action * 0b100000 +
                      // Empty * 0b1000000+
                      0 * 0b10000000; // menu mode

                flag_button_a = false;
                flag_button_b = false;
                flag_button_a_long = false;
                flag_button_b_long = false;
                flag_confirm_action = false;
                flag_exit_action = false;
            }

            return (out);
        }

        static void i2c_irq_handler(i2c_inst_t *i2c, i2c_slave_event_t event)
        {
            switch (event)
            {
            case I2C_SLAVE_RECEIVE:
                if (instance->transfer_ptr < sizeof(instance->transfer_buff))
                    instance->transfer_buff[instance->transfer_ptr++] = i2c_read_byte_raw(i2c);
                break;

            case I2C_SLAVE_REQUEST:
                i2c_write_byte_raw(i2c, instance->generate_button_map());
                break;

            case I2C_SLAVE_FINISH:

                instance->exec_command();

                break;
            default:
                break;
            }
        }

        bool flag_redraw = false;

        bool flag_button_a = false;
        bool flag_button_b = false;
        bool flag_button_a_long = false;
        bool flag_button_b_long = false;
        bool flag_button_a_long_hold = false;
        bool flag_button_b_long_hold = false;
        bool flag_confirm_action = false;
        bool flag_exit_action = false;

        void Run()
        {

            while (1)
            {
                manager.handle_buttons();

                if (manager.buttons[0].short_press_event)
                    flag_button_a = true;
                if (manager.buttons[1].short_press_event)
                    flag_button_b = true;

                if (manager.buttons[0].long_press_event)
                    flag_button_a_long = true;
                if (manager.buttons[1].long_press_event)
                    flag_button_b_long = true;

                flag_button_a_long_hold = manager.buttons[0].long_press_indicator;
                flag_button_b_long_hold = manager.buttons[1].long_press_indicator;

                if (manager.ConfirmAction())
                    flag_confirm_action = true;

                if (manager.BackAction())
                    flag_exit_action = true;

                if (flag_redraw)
                {
                    manager.UpdateScreen();
                }

                if (menu_active)
                {
                    manager.gfx->clear();
                    menu.draw();
                    if (manager.ConfirmAction())
                        menu_exit_state = 0b01000000;
                    else if (manager.BackAction())
                        menu_exit_state = 0b00100000;
                    manager.UpdateScreen();
                }
            }
        }

        static I2C_Canvas *instance;

    private:
        __force_inline bool SizeCheck(uint16_t *read_ptr, size_t struct_size)
        {
            // Check if adding the structure size would exceed transfer_ptr
            if ((*read_ptr + struct_size) > transfer_ptr)
            {
                printf("Size check failed");
                return false;
            }
            return true;
        }
        void exec_command()
        {
            if (transfer_ptr == 0)
                return;
            uint16_t read_ptr = 0;

            while (read_ptr < transfer_ptr)
            {
                // Read the current command type
                Draw_Types to_draw = (Draw_Types)(transfer_buff[read_ptr]);
                read_ptr++;

                switch (to_draw)
                {
                case DRAW_NONE:
                    // No action needed for DRAW_NONE
                    break;

                case DRAW_PUSH_FRAME:
                    // Handle frame push if needed
                    flag_redraw = true;
                    break;

                case DRAW_FILL:
                {
                    if (SizeCheck(&read_ptr, sizeof(Draw_Fill)))
                    {
                        Draw_Fill *params = (Draw_Fill *)&transfer_buff[read_ptr];
                        read_ptr += sizeof(Draw_Fill);
                        manager.gfx->clear(params->color);
                    }
                    break;
                }

                case DRAW_TEXT:
                {
                    if (SizeCheck(&read_ptr, sizeof(Draw_Text)))
                    {
                        Draw_Text *params = (Draw_Text *)&transfer_buff[read_ptr];
                        read_ptr += sizeof(Draw_Text);

                        if (SizeCheck(&read_ptr, params->len))
                        {
                            char *text = (char *)&transfer_buff[read_ptr];
                            read_ptr += params->len;

                            text[params->len - 1] = '\0'; // Ensure null-termination
                            manager.gfx->drawString(params->x, params->y, text, params->color);
                        }
                    }
                    break;
                }

                case DRAW_RECT:
                case DRAW_RECT_FILL:
                {
                    if (SizeCheck(&read_ptr, sizeof(Draw_Rect)))
                    {
                        Draw_Rect *params = (Draw_Rect *)&transfer_buff[read_ptr];
                        read_ptr += sizeof(Draw_Rect);
                        manager.gfx->drawRect(params->x, params->y, params->w, params->h, params->color, to_draw == DRAW_RECT_FILL);
                    }
                    break;
                }

                case DRAW_BIT_MAP:
                {
                    if (SizeCheck(&read_ptr, sizeof(Draw_Bit_Map)))
                    {
                        Draw_Bit_Map *params = (Draw_Bit_Map *)&transfer_buff[read_ptr];
                        read_ptr += sizeof(Draw_Bit_Map);

                        if (SizeCheck(&read_ptr, params->len))
                        {
                            uint8_t *colors = (uint8_t *)&transfer_buff[read_ptr];
                            read_ptr += params->len;

                            uint16_t x = params->x;
                            uint16_t y = params->y;
                            uint16_t w = params->w;
                            uint16_t lastPix = params->offset + params->len;

                            for (uint16_t i = params->offset; i < lastPix; i++)
                            {
                                manager.gfx->drawPixel(x + (i % w), y + (i / w), colors[i - params->offset]);
                            }
                        }
                    }
                    break;
                }
                case DRAW_MENU:
                {
                    if (SizeCheck(&read_ptr, sizeof(Draw_Menu)))
                    {
                        Draw_Menu *params = (Draw_Menu *)&transfer_buff[read_ptr];
                        read_ptr += sizeof(Draw_Menu);

                        if (params->len == 0)
                        {
                            menu_active = false;
                        }
                        else if (params->len > sizeof(menu_Buffer_data))
                        {
                            menu_active = false;
                            printf("Tried to activate menu with too much data");
                        }
                        else if (SizeCheck(&read_ptr, params->len))
                        {
                            char *chars = (char *)&transfer_buff[read_ptr];
                            read_ptr += params->len;

                            menu_active = true;
                            menu_exit_state = 0;
                            menu_selected = params->defaul_selected;

                            // Copy the data
                            memcpy(menu_Buffer_data, chars, params->len);

                            // Initialize the first item
                            menu_items = 0;
                            menu_Buffer_ptrs[menu_items++] = &(menu_Buffer_data[0]);

                            // Loop through the menu_Buffer_data
                            for (uint16_t i = 0; i < params->len - 1; i++)
                            {
                                if (menu_Buffer_data[i] == '\0' && (i + 1) < params->len)
                                {
                                    if (menu_items < (sizeof(menu_Buffer_ptrs) / sizeof(menu_Buffer_ptrs[0])))
                                    {
                                        menu_Buffer_ptrs[menu_items++] = &(menu_Buffer_data[i + 1]);
                                    }
                                    else
                                    {
                                        // Exceeded maximum number of menu items
                                        break;
                                    }
                                }
                            }

                            // Ensure the data is null-terminated
                            menu_Buffer_data[params->len - 1] = '\0';

                            // Initialize the menu
                            menu = MenuDrawer(menu_Buffer_ptrs, menu_items);
                        }
                    }
                    break;
                }

                default:
                    printf("Unknown header");
                    break;
                }
            }

            transfer_ptr = 0;
        }
        I2C_DRIVER_C *driver;
    };
    I2C_Canvas *I2C_Canvas::instance = nullptr;

}
