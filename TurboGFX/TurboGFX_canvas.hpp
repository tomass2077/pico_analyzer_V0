#pragma once
#include <TurboGFX.hpp>
#include <manager.hpp>
#include <utils.hpp>
class Turbo_Canvas
{
    uint8_t *transfer_buff;
    uint16_t transfer_buff_size;

    const char *menu_Buffer_ptrs[12];
    char menu_Buffer_data[12 * 20];
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
        // Followed by some uint8_t charecters
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
        uint8_t offset;
        uint8_t len;
        // Followed by some uint8_t data, thats 8bit(from the lut) color
    };
    struct Draw_Menu
    {
        uint8_t defaul_selected;
        uint8_t len;
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
    Turbo_Canvas(uint16_t buffer_size)
    {
        transfer_buff = (uint8_t *)malloc(buffer_size);
    }

    bool flag_redraw = false;

    void init_screen()
    {
        manager.gfx->clear();
        manager.gfx->drawString(120 - 8 * 6, manager.gfx->height / 2 - 16, "Canvas live", LUT_C::IVORY_WHITE);
        manager.gfx->drawString(120 - 8 * 12 + 4 /*aka .5 of 8*/, manager.gfx->height / 2, "Waitign for draw command", LUT_C::IVORY_WHITE);
        manager.UpdateScreen();
        sleep_ms(500);
    }
    void Update_screen(uint16_t buffer_length)
    {
        transfer_buff_size = buffer_length;
        exec_command();
        transfer_buff_size = 0;
    }

private:
    __force_inline bool SizeCheck(uint16_t *read_ptr, size_t struct_size)
    {
        // Check if adding the structure size would exceed transfer_buff_size
        if (struct_size > 511 || (*read_ptr + struct_size) >= transfer_buff_size)
        {
            // printf("Size check failed\n");
            return false;
        }
        return true;
    }
    void exec_command()
    {
        if (transfer_buff_size == 0)
            return;
        uint16_t read_ptr = 0;

        while (read_ptr < transfer_buff_size)
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
                        uint8_t lastPix = params->len + params->offset;

                        for (uint16_t i = params->offset; i < lastPix; i++)
                        {
                            uint16_t y_cur = y + (i / w);
                            if (y_cur > 135)
                                break;
                            manager.gfx->drawPixel(x + (i % w), y_cur, colors[i - params->offset]);
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
                        // printf("Tried to activate menu with too much data\n");
                    }
                    else if (SizeCheck(&read_ptr, params->len))
                    {
                        char *chars = (char *)&transfer_buff[read_ptr];
                        read_ptr += params->len;

                        menu_active = true;
                        // Copy the data
                        memcpy(menu_Buffer_data, chars, params->len);

                        // Initialize the first item
                        uint8_t menu_items = 0;
                        menu_Buffer_ptrs[menu_items++] = &(menu_Buffer_data[0]);

                        // Loop through the menu_Buffer_data
                        uint8_t size_chck = 0;
                        for (uint16_t i = 0; i < params->len - 1; i++)
                        {
                            size_chck++;
                            if (size_chck > 20) // force 20 char limit
                            {
                                menu_Buffer_data[i] = 0;
                                size_chck = 0;
                            }
                            if (menu_Buffer_data[i] == '\0' && (i + 1) < params->len)
                            {
                                if (menu_items < (sizeof(menu_Buffer_ptrs) / sizeof(menu_Buffer_ptrs[0])))
                                    menu_Buffer_ptrs[menu_items++] = &(menu_Buffer_data[i + 1]);
                                else
                                    // Exceeded maximum number of menu items
                                    break;
                            }
                        }

                        // Ensure the data is null-terminated
                        menu_Buffer_data[params->len - 1] = '\0';

                        // Initialize the menu
                        menu = MenuDrawer(menu_Buffer_ptrs, menu_items);
                        menu.jump(params->defaul_selected);
                    }
                }
                break;
            }

            default:
                // printf("Unknown header: %d\n", to_draw);
                transfer_buff_size = 0; // ends drawing
                break;
            }
        }
    }
};
