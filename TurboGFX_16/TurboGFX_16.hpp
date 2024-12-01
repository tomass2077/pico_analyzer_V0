#pragma once
#include <cstdint>

struct Color_565
{
    uint16_t value;

    Color_565() : value(0) {}
    Color_565(uint8_t r, uint8_t g, uint8_t b)
    {
        value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    Color_565(uint16_t val) : value(val) {}

    uint8_t r() const { return (value >> 11) & 0x1F; }
    uint8_t g() const { return (value >> 5) & 0x3F; }
    uint8_t b() const { return value & 0x1F; }

    bool operator==(const Color_565 &other) const { return value == other.value; }
    bool operator!=(const Color_565 &other) const { return value != other.value; }

    static Color_565 fromHSV(float h, float s, float v)
    {
        float r, g, b;
        int i = static_cast<int>(h * 6);
        float f = h * 6 - i;
        float p = v * (1 - s);
        float q = v * (1 - f * s);
        float t = v * (1 - (1 - f) * s);

        switch (i % 6)
        {
        case 0:
            r = v;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = v;
            b = p;
            break;
        case 2:
            r = p;
            g = v;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = v;
            break;
        case 4:
            r = t;
            g = p;
            b = v;
            break;
        case 5:
            r = v;
            g = p;
            b = q;
            break;
        }

        // Convert to 5-bit/6-bit/5-bit RGB
        uint8_t r565 = static_cast<uint8_t>(r * 255);
        uint8_t g565 = static_cast<uint8_t>(g * 255);
        uint8_t b565 = static_cast<uint8_t>(b * 255);

        return Color_565(r565, g565, b565);
    }
};

class Graphics
{
public:
    Graphics(uint16_t *buffer, uint16_t width, uint16_t height);

    // Clear the buffer with a specific color
    void clear(Color_565 color);
    void fade(Color_565 color, uint8_t alpha);

    _PSTL_PRAGMA_FORCEINLINE Color_565 getPixel(uint16_t x, uint16_t y)
    {
        return buffer[y + x * height];
    };

    _PSTL_PRAGMA_FORCEINLINE void drawPixel(uint16_t x, uint16_t y, Color_565 color)
    {
        if (x >= width || y >= height)
            return;
        buffer[y + x * height] = color.value;
    }

    // Fast pixel drawing
    _PSTL_PRAGMA_FORCEINLINE void drawPixel_PreClip(uint16_t x, uint16_t y, Color_565 color)
    {
        buffer[y + x * height] = color.value;
    }

    // Anti-aliased pixel drawing
    _PSTL_PRAGMA_FORCEINLINE void drawPixelAA(uint16_t x, uint16_t y, Color_565 color, uint8_t alpha)
    {
        if (x >= width || y >= height)
            return;
        uint16_t bgValue = buffer[y + x * height];
        Color_565 bgColor(bgValue);
        if (alpha == 1)
            buffer[y + x * height] = color.value;
        else if (alpha == 0)
            return;
        else
            buffer[y + x * height] = blendColors(color, bgColor, alpha).value;
    }
    // Fast shapes
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, Color_565 color);
    void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, Color_565 color, bool fill = false);
    void drawCircle(uint16_t x0, uint16_t y0, uint16_t radius, Color_565 color, bool fill = false);
    void drawTriangle(uint16_t x0, uint16_t y0,
                      uint16_t x1, uint16_t y1,
                      uint16_t x2, uint16_t y2,
                      Color_565 color, bool fill = false);
    void drawArc(uint16_t x0, uint16_t y0, uint16_t radius,
                 uint16_t startAngle, uint16_t endAngle,
                 Color_565 color, bool fill = false);

    // Anti-aliased shapes with fixed-point parameters (shifted by 8 bits)
    void drawLineAA(float x0, float y0, float x1, float y1, Color_565 color);
    void drawRectAA(float x, float y, float w, float h, Color_565 color, bool fill = false);
    void drawRoundedRectAA(float x, float y, float w, float h, float radius, Color_565 color, bool fill);
    void drawCircleAA(float x0, float y0, float radius, Color_565 color, bool fill = false);
    void drawTriangleAA(float x0, float y0,
                        float x1, float y1,
                        float x2, float y2,
                        Color_565 color, bool fill = false);
    void drawArcAA(float x0, float y0, float radius,
                   float startAngle, float endAngle,
                   Color_565 color, bool fill = false);

    void drawChar(int x, int y, char c, Color_565 color);
    void drawString(int x, int y, const char *str, Color_565 color);

    uint16_t width;
    uint16_t *buffer;

    uint16_t height;

private:
    uint32_t buffer_size;

    // Blending cache
    Color_565 cachedFgColor;
    Color_565 cachedBgColor;
    uint8_t cachedAlpha = 0;
    Color_565 cachedResultColor;

    // Helper functions
    Color_565 blendColors(Color_565 fgColor, Color_565 bgColor, uint8_t alpha);
    void swapColor(Color_565 &a, Color_565 &b);
    void swapFloat(float &a, float &b);

    int32_t absInt32(int32_t x);
    void swapUInt16(uint16_t &a, uint16_t &b);
    uint16_t minUInt16(uint16_t a, uint16_t b);
    uint16_t maxUInt16(uint16_t a, uint16_t b);
};
