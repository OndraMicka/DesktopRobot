#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace desktoprobot
{
    class Framebuffer
    {
    public:
        static constexpr int Width = 128;
        static constexpr int Height = 64;
        static constexpr std::size_t BufferSize = Width * Height / 8;

        Framebuffer();

        void clear();
        void setPixel(int x, int y, bool value = true);
        bool getPixel(int x, int y) const;
        const std::array<uint8_t, BufferSize>& data() const;

        void drawRect(int x, int y, int width, int height, bool value = true);
        void drawRoundedRect(int x, int y, int width, int height, int radius, bool value = true);
        void drawLine(int x0, int y0, int x1, int y1, bool value = true);
        void drawCircle(int centerX, int centerY, int radius, bool filled = false, bool value = true);

    private:
        std::array<uint8_t, BufferSize> pixels_{};
    };
}
