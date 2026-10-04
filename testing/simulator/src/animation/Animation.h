#pragma once

#include "../Framebuffer.h"

#include <cstddef>
#include <cstdint>

namespace desktoprobot
{
    enum class DrawCommandType : uint8_t
    {
        Pixel,
        Rectangle,
        RoundedRectangle,
        Line,
        Circle
    };

    struct DrawCommand
    {
        DrawCommandType type;
        int x1;
        int y1;
        int x2;
        int y2;
        bool filled;
        int radius = -1;
    };

    constexpr DrawCommand pixel(int x, int y)
    {
        return { DrawCommandType::Pixel, x, y, 0, 0, true };
    }

    constexpr DrawCommand rectangle(int x, int y, int width, int height)
    {
        return { DrawCommandType::Rectangle, x, y, width, height, true };
    }

    constexpr DrawCommand line(int x1, int y1, int x2, int y2, bool value = true)
    {
        return { DrawCommandType::Line, x1, y1, x2, y2, value };
    }

    constexpr DrawCommand circle(int centerX, int centerY, int radius, bool filled = false)
    {
        return { DrawCommandType::Circle, centerX, centerY, radius, 0, filled };
    }

    constexpr DrawCommand roundedRectangle(int x, int y, int width, int height, int radius, bool filled = true)
    {
        return { DrawCommandType::RoundedRectangle, x, y, width, height, filled, radius };
    }

    struct Frame
    {
        uint32_t durationMs;
        const DrawCommand* commands;
        std::size_t commandCount;
    };

    struct Animation
    {
        const char* name;
        const Frame* frames;
        std::size_t frameCount;
        bool loop;
    };

    void drawFrame(const Frame& frame, Framebuffer& framebuffer);
}
