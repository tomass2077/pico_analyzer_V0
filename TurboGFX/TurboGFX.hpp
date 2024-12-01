#pragma once
#include <cstdint>
#include <cstring>

class LUT_C
{
public:
    static constexpr size_t COLOR_BYTES = 2;

private:
    uint16_t colorTable[29] = {
        0xF79C,
        0xBDB7,
        0x8411,
        0x630C,
        0x4229,
        0x39CB,
        0x0000,
        0x3148,
        0x420D,
        0x4C19,
        0x6E1A,
        0xA6F8,
        0xEF13,
        0xD50D,
        0xB28A,
        0x6A8D,
        0x4A0B,
        0x8247,
        0xA3CB,
        0xE676,
        0xC68D,
        0x8D8C,
        0x53CF,
        0x4AC9,
        0x7B88,
        0xB5AF,
        0xEE58,
        0xCC59,
        0x5AAD};
    uint8_t CurPtr = 0;

public:
    enum COLORS
    {
        IVORY_WHITE,
        LAVENDER_GRAY,
        PLUM_GRAY,
        DARK_GRAY,
        SLATE_GRAY,
        MIDNIGHT_BLUE,
        ONYX_BLACK,
        GRAPE_PURPLE,
        INDIGO,
        OCEAN_BLUE,
        LAGOON_BLUE,
        SEAFOAM_GREEN,
        SAND_BEIGE,
        PEACH,
        ROSE_RED,
        HEATHER_PURPLE,
        ASH_VIOLET,
        TERRACOTTA,
        DESERT_TAN,
        ALMOND_BEIGE,
        LIME_GREEN,
        MEADOW_GREEN,
        FOREST_TEAL,
        MOSS_GREEN,
        GOLDEN_OLIVE,
        SAGE,
        BLUSH_PINK,
        ORCHID_PURPLE,
        SMOKY_LAVENDER
    };
    uint16_t lut_table[256];

    _PSTL_PRAGMA_FORCEINLINE uint16_t *GetValue(uint8_t i)
    {
        return &(lut_table[i]);
    }
    void Clear()
    {
        memset(lut_table, 128, 256 * COLOR_BYTES);
        for (int i = 0; i < 29; i++)
        {
            SetColor(i, colorTable[i]);
        }
        CurPtr = 0;
    }
    void AddColor(uint16_t color)
    {
        lut_table[CurPtr] = color;
        CurPtr++;
    }
    void SetColor(uint8_t i, uint16_t color)
    {
        lut_table[i] = color >> 8;
        lut_table[i] |= color << 8;
    }
};

class Graphics
{
public:
    LUT_C lut;
    Graphics(uint8_t *buffer, uint16_t width, uint16_t height);

    void clear();
    void clear(uint8_t color);

    _PSTL_PRAGMA_FORCEINLINE uint8_t getPixel(uint16_t x, uint16_t y)
    {
        return buffer[y + x * height];
    };

    _PSTL_PRAGMA_FORCEINLINE void drawPixel(uint16_t x, uint16_t y, uint8_t color)
    {
        if (x >= width || y >= height)
            return;
        buffer[y + x * height] = color;
    }

    // Fast pixel drawing
    _PSTL_PRAGMA_FORCEINLINE void drawPixel_PreClip(uint16_t x, uint16_t y, uint8_t color)
    {
        buffer[y + x * height] = color;
    }

    // Anti-aliased pixel drawing
    _PSTL_PRAGMA_FORCEINLINE void drawPixelAA(uint16_t x, uint16_t y, uint8_t color, uint8_t alpha)
    {
        buffer[y + x * height] = color;
    }
    // Fast shapes
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
    void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color, bool fill = false);
    void drawCircle(uint16_t x0, uint16_t y0, uint16_t radius, uint8_t color, bool fill = false);
    void drawTriangle(uint16_t x0, uint16_t y0,
                      uint16_t x1, uint16_t y1,
                      uint16_t x2, uint16_t y2,
                      uint8_t color, bool fill = false);
    void drawArc(uint16_t x0, uint16_t y0, uint16_t radius,
                 uint16_t startAngle, uint16_t endAngle,
                 uint8_t color, bool fill = false);

    // Anti-aliased shapes with fixed-point parameters (shifted by 8 bits)
    void drawLineAA(float x0, float y0, float x1, float y1, uint8_t color);
    void drawRectAA(float x, float y, float w, float h, uint8_t color, bool fill = false);
    void drawRoundedRectAA(float x, float y, float w, float h, float radius, uint8_t color, bool fill);
    void drawCircleAA(float x0, float y0, float radius, uint8_t color, bool fill = false);
    void drawTriangleAA(float x0, float y0,
                        float x1, float y1,
                        float x2, float y2,
                        uint8_t color, bool fill = false);
    void drawArcAA(float x0, float y0, float radius,
                   float startAngle, float endAngle,
                   uint8_t color, bool fill = false);

    void drawChar(int x, int y, char c, uint8_t color);
    void drawString(int x, int y, const char *str, uint8_t color);

    uint16_t width;
    uint16_t height;
    uint8_t *buffer;

private:
    uint32_t buffer_size;

    // Helper functions
    void swapColor(uint8_t &a, uint8_t &b);
    void swapFloat(float &a, float &b);

    uint16_t minUInt16(uint16_t a, uint16_t b);
    uint16_t maxUInt16(uint16_t a, uint16_t b);
};
