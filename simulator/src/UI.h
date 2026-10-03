#pragma once

#include <SDL2/SDL.h>

#include "Animation.h"

namespace desktoprobot
{
    struct Button
    {
        const char* label;
        SDL_Rect rect;
        const Animation* animation;
        bool isLoop;
        bool active;
        bool hovered;
    };

    bool pointInRect(int x, int y, const SDL_Rect& rect);
    void drawText(SDL_Renderer* renderer, const char* text, int x, int y, int scale, SDL_Color color);
    void drawButton(SDL_Renderer* renderer, const Button& button);
    void drawPanelHeader(SDL_Renderer* renderer, int x, int y, int w, int h, const char* title);
}
