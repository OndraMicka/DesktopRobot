#include "Framebuffer.h"

#include <cstdlib>

namespace desktoprobot
{
    Framebuffer::Framebuffer()
    {
        clear();
    }

    void Framebuffer::clear()
    {
        pixels_.fill(0);
    }

    void Framebuffer::setPixel(int x, int y, bool value)
    {
        if (x < 0 || x >= Width || y < 0 || y >= Height)
        {
            return;
        }

        const std::size_t index = static_cast<std::size_t>((y / 8) * Width + x);
        const uint8_t mask = static_cast<uint8_t>(1U << (y % 8));

        if (value)
        {
            pixels_[index] |= mask;
        }
        else
        {
            pixels_[index] &= static_cast<uint8_t>(~mask);
        }
    }

    bool Framebuffer::getPixel(int x, int y) const
    {
        if (x < 0 || x >= Width || y < 0 || y >= Height)
        {
            return false;
        }

        const std::size_t index = static_cast<std::size_t>((y / 8) * Width + x);
        const uint8_t mask = static_cast<uint8_t>(1U << (y % 8));
        return (pixels_[index] & mask) != 0;
    }

    const std::array<uint8_t, Framebuffer::BufferSize>& Framebuffer::data() const
    {
        return pixels_;
    }

    void Framebuffer::drawRect(int x, int y, int width, int height, bool value)
    {
        if (width <= 0 || height <= 0)
        {
            return;
        }

        for (int row = y; row < y + height; ++row)
        {
            for (int column = x; column < x + width; ++column)
            {
                setPixel(column, row, value);
            }
        }
    }

    void Framebuffer::drawLine(int x0, int y0, int x1, int y1, bool value)
    {
        const int dx = std::abs(x1 - x0);
        const int sx = x0 < x1 ? 1 : -1;
        const int dy = -std::abs(y1 - y0);
        const int sy = y0 < y1 ? 1 : -1;
        int error = dx + dy;

        while (true)
        {
            setPixel(x0, y0, value);
            if (x0 == x1 && y0 == y1)
            {
                break;
            }

            const int doubledError = 2 * error;
            if (doubledError >= dy)
            {
                error += dy;
                x0 += sx;
            }
            if (doubledError <= dx)
            {
                error += dx;
                y0 += sy;
            }
        }
    }

    void Framebuffer::drawCircle(int centerX, int centerY, int radius, bool filled, bool value)
    {
        if (radius < 0)
        {
            return;
        }

        int x = radius;
        int y = 0;
        int error = 1 - radius;

        while (x >= y)
        {
            if (filled)
            {
                drawLine(centerX - x, centerY + y, centerX + x, centerY + y, value);
                drawLine(centerX - x, centerY - y, centerX + x, centerY - y, value);
                drawLine(centerX - y, centerY + x, centerX + y, centerY + x, value);
                drawLine(centerX - y, centerY - x, centerX + y, centerY - x, value);
            }
            else
            {
                setPixel(centerX + x, centerY + y, value);
                setPixel(centerX + y, centerY + x, value);
                setPixel(centerX - y, centerY + x, value);
                setPixel(centerX - x, centerY + y, value);
                setPixel(centerX - x, centerY - y, value);
                setPixel(centerX - y, centerY - x, value);
                setPixel(centerX + y, centerY - x, value);
                setPixel(centerX + x, centerY - y, value);
            }

            ++y;
            if (error < 0)
            {
                error += 2 * y + 1;
            }
            else
            {
                --x;
                error += 2 * (y - x) + 1;
            }
        }
    }
}
