#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <array>
#include <iostream>

#include "AnimationPlayer.h"
#include "Framebuffer.h"
#include "SDLRenderer.h"
#include "UI.h"
#include "animations/blink.h"
#include "animations/dizzy.h"
#include "animations/idle.h"
#include "animations/sleep.h"
#include "animations/smile.h"
#include "animations/surprised.h"

namespace
{
    const int SCALE = 8;
    const int DISPLAY_X = 20;
    const int DISPLAY_Y = 30;
    const int PANEL_X = DISPLAY_X + desktoprobot::Framebuffer::Width * SCALE + 30;
    const int PANEL_Y = 30;
    const int PANEL_W = 220;
    const int BUTTON_W = 180;
    const int BUTTON_H = 36;

    template <std::size_t N>
    void updateButtonHover(std::array<desktoprobot::Button, N>& buttons, int mouseX, int mouseY)
    {
        for (auto& button : buttons)
        {
            button.hovered = desktoprobot::pointInRect(mouseX, mouseY, button.rect);
        }
    }

    template <std::size_t N>
    void updateButtonState(std::array<desktoprobot::Button, N>& buttons, const desktoprobot::AnimationPlayer& player)
    {
        for (auto& button : buttons)
        {
            button.active = (player.getCurrentAnimation() == button.animation);
        }
    }
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 1;
    }

    const int windowWidth = PANEL_X + PANEL_W + 30;
    const int windowHeight = DISPLAY_Y + desktoprobot::Framebuffer::Height * SCALE + 30;

    SDL_Window* window = SDL_CreateWindow(
        "Desktop Robot Simulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        windowWidth,
        windowHeight,
        SDL_WINDOW_SHOWN
    );

    if (!window)
    {
        std::cerr << "Window creation failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if (!renderer)
    {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    desktoprobot::SDLRenderer display;
    if (!display.initialize(renderer, DISPLAY_X, DISPLAY_Y, SCALE))
    {
        std::cerr << "SDL display initialization failed: " << SDL_GetError() << '\n';
        display.destroy();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    desktoprobot::AnimationPlayer player;
    player.setLoopAnimation(desktoprobot::idleAnimation);
    desktoprobot::Framebuffer framebuffer;

    std::array<desktoprobot::Button, 3> loopButtons = {
        desktoprobot::Button{ "IDLE", { PANEL_X + 20, 90, BUTTON_W, BUTTON_H }, &desktoprobot::idleAnimation, true, true, false },
        desktoprobot::Button{ "SMILE", { PANEL_X + 20, 140, BUTTON_W, BUTTON_H }, &desktoprobot::smileAnimation, true, false, false },
        desktoprobot::Button{ "SLEEP", { PANEL_X + 20, 190, BUTTON_W, BUTTON_H }, &desktoprobot::sleepAnimation, true, false, false }
    };

    std::array<desktoprobot::Button, 3> oneShotButtons = {
        desktoprobot::Button{ "BLINK", { PANEL_X + 20, 260, BUTTON_W, BUTTON_H }, &desktoprobot::blinkAnimation, false, false, false },
        desktoprobot::Button{ "SURPRISED", { PANEL_X + 20, 310, BUTTON_W, BUTTON_H }, &desktoprobot::surprisedAnimation, false, false, false },
        desktoprobot::Button{ "DIZZY", { PANEL_X + 20, 360, BUTTON_W, BUTTON_H }, &desktoprobot::dizzyAnimation, false, false, false }
    };

    uint32_t lastFrameTime = SDL_GetTicks();
    bool running = true;

    while (running)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }

            if (event.type == SDL_MOUSEMOTION)
            {
                int mouseX = 0;
                int mouseY = 0;
                SDL_GetMouseState(&mouseX, &mouseY);
                updateButtonHover(loopButtons, mouseX, mouseY);
                updateButtonHover(oneShotButtons, mouseX, mouseY);
            }

            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                int mouseX = 0;
                int mouseY = 0;
                SDL_GetMouseState(&mouseX, &mouseY);

                for (auto& button : loopButtons)
                {
                    if (desktoprobot::pointInRect(mouseX, mouseY, button.rect))
                    {
                        player.setLoopAnimation(*button.animation);
                        updateButtonState(loopButtons, player);
                        updateButtonState(oneShotButtons, player);
                        break;
                    }
                }

                for (auto& button : oneShotButtons)
                {
                    if (desktoprobot::pointInRect(mouseX, mouseY, button.rect))
                    {
                        player.playOneShot(*button.animation);
                        updateButtonState(loopButtons, player);
                        updateButtonState(oneShotButtons, player);
                        break;
                    }
                }
            }
        }

        const uint32_t now = SDL_GetTicks();
        const uint32_t delta = now - lastFrameTime;
        lastFrameTime = now;

        player.update(delta);
        player.renderCurrentFrame(framebuffer);
        updateButtonState(loopButtons, player);
        updateButtonState(oneShotButtons, player);

        SDL_SetRenderDrawColor(renderer, 10, 10, 12, 255);
        SDL_RenderClear(renderer);

        SDL_Rect displayBackground{
            DISPLAY_X,
            DISPLAY_Y,
            desktoprobot::Framebuffer::Width * SCALE,
            desktoprobot::Framebuffer::Height * SCALE
        };
        SDL_SetRenderDrawColor(renderer, 25, 25, 30, 255);
        SDL_RenderFillRect(renderer, &displayBackground);
        if (!display.render(framebuffer))
        {
            std::cerr << "SDL framebuffer rendering failed: " << SDL_GetError() << '\n';
            running = false;
            continue;
        }

        SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
        SDL_RenderDrawRect(renderer, &displayBackground);

        SDL_Rect panelBackground{ PANEL_X, PANEL_Y, PANEL_W, 430 };
        SDL_SetRenderDrawColor(renderer, 25, 25, 30, 255);
        SDL_RenderFillRect(renderer, &panelBackground);
        SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
        SDL_RenderDrawRect(renderer, &panelBackground);

        desktoprobot::drawPanelHeader(renderer, PANEL_X + 10, PANEL_Y + 10, PANEL_W - 20, 24, "LOOP / STATES");
        for (const auto& button : loopButtons)
        {
            desktoprobot::drawButton(renderer, button);
        }

        desktoprobot::drawPanelHeader(renderer, PANEL_X + 10, PANEL_Y + 230, PANEL_W - 20, 24, "ONE-SHOT");
        for (const auto& button : oneShotButtons)
        {
            desktoprobot::drawButton(renderer, button);
        }

        SDL_RenderPresent(renderer);
    }

    display.destroy();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
