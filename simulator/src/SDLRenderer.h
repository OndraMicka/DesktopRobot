#pragma once

#include <SDL2/SDL.h>

#include "Framebuffer.h"

#include <array>
#include <cstdint>

namespace desktoprobot
{
    class SDLRenderer
    {
    public:
        SDLRenderer() = default;
        ~SDLRenderer();

        SDLRenderer(const SDLRenderer&) = delete;
        SDLRenderer& operator=(const SDLRenderer&) = delete;

        bool initialize(SDL_Renderer* renderer, int x, int y, int scale);
        bool render(const Framebuffer& framebuffer);
        void destroy();

    private:
        SDL_Renderer* renderer_ = nullptr;
        SDL_Texture* texture_ = nullptr;
        SDL_Rect destination_{};
        std::array<uint8_t, Framebuffer::Width * Framebuffer::Height * 3> rgbPixels_{};
    };
}
