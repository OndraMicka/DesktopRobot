#include "SDLRenderer.h"

namespace desktoprobot
{
    SDLRenderer::~SDLRenderer()
    {
        destroy();
    }

    void SDLRenderer::destroy()
    {
        if (texture_ != nullptr)
        {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
        }

        renderer_ = nullptr;
    }

    bool SDLRenderer::initialize(SDL_Renderer* renderer, int x, int y, int scale)
    {
        if (renderer == nullptr || scale <= 0)
        {
            return false;
        }

        renderer_ = renderer;
        texture_ = SDL_CreateTexture(
            renderer_,
            SDL_PIXELFORMAT_RGB24,
            SDL_TEXTUREACCESS_STREAMING,
            Framebuffer::Width,
            Framebuffer::Height
        );

        if (texture_ == nullptr)
        {
            return false;
        }

        if (SDL_SetTextureScaleMode(texture_, SDL_ScaleModeNearest) != 0)
        {
            SDL_DestroyTexture(texture_);
            texture_ = nullptr;
            renderer_ = nullptr;
            return false;
        }

        destination_ = { x, y, Framebuffer::Width * scale, Framebuffer::Height * scale };
        return true;
    }

    bool SDLRenderer::render(const Framebuffer& framebuffer)
    {
        if (renderer_ == nullptr || texture_ == nullptr)
        {
            return false;
        }

        for (int y = 0; y < Framebuffer::Height; ++y)
        {
            for (int x = 0; x < Framebuffer::Width; ++x)
            {
                const uint8_t value = framebuffer.getPixel(x, y) ? 255 : 0;
                const std::size_t index = static_cast<std::size_t>((y * Framebuffer::Width + x) * 3);
                rgbPixels_[index] = value;
                rgbPixels_[index + 1] = value;
                rgbPixels_[index + 2] = value;
            }
        }

        if (SDL_UpdateTexture(texture_, nullptr, rgbPixels_.data(), Framebuffer::Width * 3) != 0)
        {
            return false;
        }

        return SDL_RenderCopy(renderer_, texture_, nullptr, &destination_) == 0;
    }
}
