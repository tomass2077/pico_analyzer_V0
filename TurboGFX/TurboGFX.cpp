#include "TurboGFX.hpp"
#include <cstring> // For memset
#include <complex>
#include <vector>
#include "fonst.hpp"

// Adjusted Graphics class methods for 16-bit color
Graphics::Graphics(uint8_t *buffer, uint16_t width, uint16_t height)
    : buffer(buffer), width(width), height(height)
{
    buffer_size = width * height;
}

void Graphics::clear()
{
    memset(buffer, lut.ONYX_BLACK, buffer_size);
}
void Graphics::clear(uint8_t color)
{
    memset(buffer, color, buffer_size);
}

void Graphics::swapFloat(float &a, float &b)
{
    float temp = a;
    a = b;
    b = temp;
}

uint16_t Graphics::minUInt16(uint16_t a, uint16_t b)
{
    return (a < b) ? a : b;
}

uint16_t Graphics::maxUInt16(uint16_t a, uint16_t b)
{
    return (a > b) ? a : b;
}

// Fast line drawing (Bresenham's algorithm)
void Graphics::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
    int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;

    while (true)
    {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

// Anti-aliased line drawing (Wu's algorithm)
void Graphics::drawLineAA(float x0, float y0, float x1, float y1, uint8_t color)
{
    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);

    if (steep)
    {
        swapFloat(x0, y0);
        swapFloat(x1, y1);
    }

    if (x0 > x1)
    {
        swapFloat(x0, x1);
        swapFloat(y0, y1);
    }

    float dx = x1 - x0;
    float dy = y1 - y0;
    float gradient = (dx == 0.0f) ? 1.0f : dy / dx;

    // Handle first endpoint
    float xEnd = roundf(x0);
    float yEnd = y0 + gradient * (xEnd - x0);
    float xGap = 1.0f - std::fmod(x0 + 0.5f, 1.0f);
    int32_t xPixel1 = static_cast<int32_t>(xEnd);
    int32_t yPixel1 = static_cast<int32_t>(std::floor(yEnd));

    if (steep)
    {
        drawPixelAA(yPixel1, xPixel1, color, static_cast<uint8_t>((1.0f - (yEnd - yPixel1)) * 255));
        drawPixelAA(yPixel1 + 1, xPixel1, color, static_cast<uint8_t>((yEnd - yPixel1) * 255));
    }
    else
    {
        drawPixelAA(xPixel1, yPixel1, color, static_cast<uint8_t>((1.0f - (yEnd - yPixel1)) * 255));
        drawPixelAA(xPixel1, yPixel1 + 1, color, static_cast<uint8_t>((yEnd - yPixel1) * 255));
    }

    float intery = yEnd + gradient;

    // Handle second endpoint
    xEnd = roundf(x1);
    yEnd = y1 + gradient * (xEnd - x1);
    xGap = std::fmod(x1 + 0.5f, 1.0f);
    int32_t xPixel2 = static_cast<int32_t>(xEnd);
    int32_t yPixel2 = static_cast<int32_t>(std::floor(yEnd));

    if (steep)
    {
        drawPixelAA(yPixel2, xPixel2, color, static_cast<uint8_t>((1.0f - (yEnd - yPixel2)) * 255));
        drawPixelAA(yPixel2 + 1, xPixel2, color, static_cast<uint8_t>((yEnd - yPixel2) * 255));
    }
    else
    {
        drawPixelAA(xPixel2, yPixel2, color, static_cast<uint8_t>((1.0f - (yEnd - yPixel2)) * 255));
        drawPixelAA(xPixel2, yPixel2 + 1, color, static_cast<uint8_t>((yEnd - yPixel2) * 255));
    }

    // Main loop
    if (steep)
    {
        for (int32_t x = xPixel1 + 1; x < xPixel2; ++x)
        {
            int32_t y = static_cast<int32_t>(std::floor(intery));
            float frac = intery - y;
            drawPixelAA(y, x, color, static_cast<uint8_t>((1.0f - frac) * 255));
            drawPixelAA(y + 1, x, color, static_cast<uint8_t>(frac * 255));
            intery += gradient;
        }
    }
    else
    {
        for (int32_t x = xPixel1 + 1; x < xPixel2; ++x)
        {
            int32_t y = static_cast<int32_t>(std::floor(intery));
            float frac = intery - y;
            drawPixelAA(x, y, color, static_cast<uint8_t>((1.0f - frac) * 255));
            drawPixelAA(x, y + 1, color, static_cast<uint8_t>(frac * 255));
            intery += gradient;
        }
    }
}

// Rectangle drawing
void Graphics::drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t color, bool fill)
{
    if (fill)
    {
        for (uint16_t i = y; i < y + h; ++i)
        {
            drawLine(x, i, x + w - 1, i, color);
        }
    }
    else
    {
        drawLine(x, y, x + w - 1, y, color);
        drawLine(x, y + h - 1, x + w - 1, y + h - 1, color);
        drawLine(x, y, x, y + h - 1, color);
        drawLine(x + w - 1, y, x + w - 1, y + h - 1, color);
    }
}

// Circle drawing
void Graphics::drawCircle(uint16_t x0, uint16_t y0, uint16_t radius, uint8_t color, bool fill)
{
    int16_t x = radius;
    int16_t y = 0;
    int16_t err = 0;

    while (x >= y)
    {
        if (fill)
        {
            drawLine(x0 - x, y0 + y, x0 + x, y0 + y, color);
            drawLine(x0 - y, y0 + x, x0 + y, y0 + x, color);
            drawLine(x0 - x, y0 - y, x0 + x, y0 - y, color);
            drawLine(x0 - y, y0 - x, x0 + y, y0 - x, color);
        }
        else
        {
            drawPixel(x0 + x, y0 + y, color);
            drawPixel(x0 - x, y0 + y, color);
            drawPixel(x0 + x, y0 - y, color);
            drawPixel(x0 - x, y0 - y, color);
            drawPixel(x0 + y, y0 + x, color);
            drawPixel(x0 - y, y0 + x, color);
            drawPixel(x0 + y, y0 - x, color);
            drawPixel(x0 - y, y0 - x, color);
        }
        y++;
        if (err <= 0)
        {
            err += 2 * y + 1;
        }
        else
        {
            x--;
            err += 2 * (y - x) + 1;
        }
    }
}
void Graphics::drawCircleAA(float x0, float y0, float radius, uint8_t color, bool fill)
{
    float angleStep = 1.0f / radius;

    if (fill)
    {
        for (float angle = 0.0f; angle <= 2.0f * 3.14159265f; angle += angleStep)
        {
            float xEnd = x0 + radius * cosf(angle);
            float yEnd = y0 + radius * sinf(angle);
            drawLineAA(x0, y0, xEnd, yEnd, color);
        }
    }
    else
    {
        for (float angle = 0.0f; angle <= 2.0f * 3.14159265f; angle += angleStep)
        {
            float xPoint = x0 + radius * cosf(angle);
            float yPoint = y0 + radius * sinf(angle);
            drawPixelAA(xPoint, yPoint, color, 255);
        }
    }
}
// Fast triangle drawing (filled using barycentric coordinates)
void Graphics::drawTriangle(uint16_t x0, uint16_t y0,
                            uint16_t x1, uint16_t y1,
                            uint16_t x2, uint16_t y2,
                            uint8_t color, bool fill)
{
    if (fill)
    {
        // Compute bounding box
        uint16_t minX = minUInt16(minUInt16(x0, x1), x2);
        uint16_t maxX = maxUInt16(maxUInt16(x0, x1), x2);
        uint16_t minY = minUInt16(minUInt16(y0, y1), y2);
        uint16_t maxY = maxUInt16(maxUInt16(y0, y1), y2);

        // Precompute area of the triangle
        int32_t area = (x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0);

        // Scan over bounding box
        for (uint16_t y = minY; y <= maxY; y++)
        {
            for (uint16_t x = minX; x <= maxX; x++)
            {
                // Compute barycentric coordinates
                int32_t w0 = (x1 - x0) * (y - y0) - (x - x0) * (y1 - y0);
                int32_t w1 = (x2 - x1) * (y - y1) - (x - x1) * (y2 - y1);
                int32_t w2 = (x0 - x2) * (y - y2) - (x - x2) * (y0 - y2);

                if ((w0 >= 0 && w1 >= 0 && w2 >= 0) ||
                    (w0 <= 0 && w1 <= 0 && w2 <= 0))
                {
                    drawPixel(x, y, color);
                }
            }
        }
    }
    else
    {
        drawLine(x0, y0, x1, y1, color);
        drawLine(x1, y1, x2, y2, color);
        drawLine(x2, y2, x0, y0, color);
    }
}

// Anti-aliased triangle drawing using fixed-point arithmetic
void Graphics::drawTriangleAA(float x0, float y0,
                              float x1, float y1,
                              float x2, float y2,
                              uint8_t color, bool fill)
{
    if (fill)
    {
        // Implementing an anti-aliased filled triangle is complex and requires
        // coverage calculation. Here, we'll perform edge AA and fill the interior.

        // First, sort vertices by y-coordinate ascending (y0 <= y1 <= y2)
        if (y0 > y1)
        {
            swapFloat(y0, y1);
            swapFloat(x0, x1);
        }
        if (y1 > y2)
        {
            swapFloat(y1, y2);
            swapFloat(x1, x2);
        }
        if (y0 > y1)
        {
            swapFloat(y0, y1);
            swapFloat(x0, x1);
        }

        // Compute gradients
        float dx01 = (x1 - x0) / (y1 - y0);
        float dx12 = (x2 - x1) / (y2 - y1);
        float dx02 = (x2 - x0) / (y2 - y0);

        // Initialize x-coordinates for scanline interpolation
        float xa, xb;

        // For each scanline
        for (float y = y0; y <= y2; y += 1.0f)
        {
            // Determine start and end points of the current scanline
            if (y < y1)
            {
                xa = x0 + (y - y0) * dx01;
                xb = x0 + (y - y0) * dx02;
            }
            else if (y <= y2)
            {
                xa = x1 + (y - y1) * dx12;
                xb = x0 + (y - y0) * dx02;
            }
            else
            {
                continue;
            }

            // Ensure xa is the leftmost and xb is the rightmost
            if (xa > xb)
                swapFloat(xa, xb);

            // Draw the scanline with anti-aliasing
            for (float x = std::ceil(xa); x <= std::floor(xb); x += 1.0f)
            {
                drawPixel(x, y, color);
            }
        }

        // Draw anti-aliased edges
        drawLineAA(x0, y0, x1, y1, color);
        drawLineAA(x1, y1, x2, y2, color);
        drawLineAA(x2, y2, x0, y0, color);
    }
    else
    {
        drawLineAA(x0, y0, x1, y1, color);
        drawLineAA(x1, y1, x2, y2, color);
        drawLineAA(x2, y2, x0, y0, color);
    }
}

// Monospace text rendering
void Graphics::drawChar(int x, int y, char c, uint8_t color)
{
    uint16_t num = (c - 32) * 16;

    for (uint8_t pos = 0; pos < 16; pos++)
    {
        int yPos = y + pos;
        // Check y position only once per row
        if (yPos < 0 || yPos >= height)
            continue;

        uint8_t temp = asc2_1608[num + pos];

        // Precompute x bounds and avoid checking for each pixel in the row
        if (x >= 0 && x + 8 <= width)
        {
            for (uint8_t t = 0; t < 8; t++)
            {
                if (temp & (1 << t))
                {
                    drawPixel_PreClip(x + t, yPos, color);
                }
            }
        }
        else
        {
            for (uint8_t t = 0; t < 8; t++)
            {
                if (temp & (1 << t))
                {
                    int xPos = x + t;
                    if (xPos >= 0 && xPos < width)
                        drawPixel_PreClip(xPos, yPos, color);
                }
            }
        }
    }
}

void Graphics::drawString(int x, int y, const char *str, uint8_t color)
{
    while (*str)
    {
        drawChar(x, y, *str, color);
        x += 8; // Character width is 8 pixels
        str++;
    }
}