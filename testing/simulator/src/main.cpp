#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

#include "Framebuffer.h"
#include "SDLRenderer.h"
#include "UI.h"
#include "animation/AnimationPlayer.h"
#include "animation/TestAnimations.h"
#include "sound/SoundPlayer.h"
#include "sound/backends/pc/SDLAudioBackend.h"

namespace
{
    enum class ControlType
    {
        State,
        Animation,
        Sound
    };

    struct Control
    {
        desktoprobot::Button button;
        ControlType type;
        desktoprobot::RobotState state;
        desktoprobot::TestAnimationId animation;
        desktoprobot::SoundEffect sound;
    };

    struct SoundTestDefinition
    {
        desktoprobot::SoundEffect effect;
        const char* label;
    };

    struct Layout
    {
        int width;
        int height;
        int margin;
        int gap;
        int titleY;
        int panelsY;
        int panelHeight;
        int displayX;
        int displayY;
        int displayScale;
        int displayPanelX;
        int displayPanelWidth;
        int idleX;
        int idleWidth;
        int tiredX;
        int stateWidth;
        int soundsX;
        int soundsWidth;
        int tiredHeight;
        int sleepY;
        int sleepHeight;
        int buttonHeight;
        int buttonGap;
    };

    constexpr std::array<SoundTestDefinition, 11> SOUND_TESTS = {{
        { desktoprobot::SoundEffect::Happy, "HAPPY" },
        { desktoprobot::SoundEffect::SuperHappy, "SUPER HAPPY" },
        { desktoprobot::SoundEffect::Cuddly, "CUDDLY" },
        { desktoprobot::SoundEffect::Angry, "ANGRY" },
        { desktoprobot::SoundEffect::Surprised, "SURPRISED" },
        { desktoprobot::SoundEffect::Sleep, "SLEEP" },
        { desktoprobot::SoundEffect::Friendly, "FRIENDLY" },
        { desktoprobot::SoundEffect::WakeUp, "WAKE UP" },
        { desktoprobot::SoundEffect::Hum, "HUM" },
        { desktoprobot::SoundEffect::Content, "CONTENT" },
        { desktoprobot::SoundEffect::Bored, "BORED" }
    }};

    int countStateAnimations(desktoprobot::RobotState state)
    {
        int count = 0;
        for (const auto& definition : desktoprobot::getAnimationTestDefinitions())
        {
            if (definition.state == state)
            {
                ++count;
            }
        }
        return count;
    }

    int getStateGridRows(desktoprobot::RobotState state, int width)
    {
        const int columns = width >= 250 ? 2 : 1;
        const int count = countStateAnimations(state);
        return (count + columns - 1) / columns;
    }

    Layout createLayout(int width, int height)
    {
        Layout layout{};
        layout.width = width;
        layout.height = height;
        layout.margin = std::max(12, width / 100);
        layout.gap = std::max(8, width / 140);
        layout.titleY = std::max(24, height / 28);
        layout.panelsY = std::max(100, height / 8);
        layout.panelHeight = height - layout.panelsY - layout.margin;

        const int leftWidth = width * 37 / 100;
        layout.displayPanelX = layout.margin;
        layout.displayPanelWidth = leftWidth - layout.margin;
        const int displayAreaY = layout.panelsY + 54;
        const int displayAreaHeight = layout.panelHeight - 72;
        layout.displayScale = std::max(1, std::min({
            12,
            (layout.displayPanelWidth - layout.margin * 2) / desktoprobot::Framebuffer::Width,
            displayAreaHeight / desktoprobot::Framebuffer::Height
        }));
        layout.displayX = layout.displayPanelX +
            (layout.displayPanelWidth - desktoprobot::Framebuffer::Width * layout.displayScale) / 2;
        layout.displayY = displayAreaY +
            (displayAreaHeight - desktoprobot::Framebuffer::Height * layout.displayScale) / 2;

        const int controlsX = layout.displayPanelX + leftWidth + layout.gap;
        const int controlsWidth = width - controlsX - layout.margin;
        const int columnGaps = layout.gap * 2;
        const int columnsWidth = controlsWidth - columnGaps;
        layout.idleWidth = columnsWidth * 44 / 100;
        layout.stateWidth = columnsWidth * 24 / 100;
        layout.soundsWidth = columnsWidth - layout.idleWidth - layout.stateWidth;
        layout.idleX = controlsX;
        layout.tiredX = layout.idleX + layout.idleWidth + layout.gap;
        layout.soundsX = layout.tiredX + layout.stateWidth + layout.gap;

        layout.buttonGap = 5;
        const int stateRows =
            getStateGridRows(desktoprobot::RobotState::Tired, layout.stateWidth) +
            getStateGridRows(desktoprobot::RobotState::Sleep, layout.stateWidth);
        const int tiredRows = getStateGridRows(desktoprobot::RobotState::Tired, layout.stateWidth);
        const int maxButtonHeight =
            (layout.panelHeight - 200 - (stateRows + 1) * layout.buttonGap) / stateRows;
        layout.buttonHeight = std::clamp(maxButtonHeight, 20, 36);
        layout.tiredHeight = 100 + tiredRows * (layout.buttonHeight + layout.buttonGap);
        layout.sleepY = layout.panelsY + layout.tiredHeight + layout.gap;
        layout.sleepHeight = layout.panelHeight - layout.tiredHeight - layout.gap;
        return layout;
    }

    void addControl(
        std::vector<Control>& controls,
        const char* label,
        SDL_Rect rect,
        ControlType type,
        desktoprobot::RobotState state = desktoprobot::RobotState::Idle,
        desktoprobot::TestAnimationId animation = desktoprobot::TestAnimationId::Blink,
        desktoprobot::SoundEffect sound = desktoprobot::SoundEffect::Happy
    )
    {
        controls.push_back({
            { label, rect, false, false },
            type,
            state,
            animation,
            sound
        });
    }

    void drawPanel(SDL_Renderer* renderer, int x, int y, int width, int height, const char* title)
    {
        SDL_Rect panel{ x, y, width, height };
        SDL_SetRenderDrawColor(renderer, 25, 25, 30, 255);
        SDL_RenderFillRect(renderer, &panel);
        SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
        SDL_RenderDrawRect(renderer, &panel);
        desktoprobot::drawPanelHeader(renderer, x + 10, y + 10, width - 20, 28, title);
    }

    void addStateControls(
        std::vector<Control>& controls,
        desktoprobot::RobotState state,
        int x,
        int y,
        int width,
        int buttonHeight,
        int buttonGap
    )
    {
        addControl(
            controls,
            "SELECT STATE",
            { x + 14, y + 46, width - 28, buttonHeight },
            ControlType::State,
            state
        );

        const auto& definitions = desktoprobot::getAnimationTestDefinitions();
        const int columns = width >= 250 ? 2 : 1;
        const int gridX = x + 12;
        const int gridY = y + 88;
        const int gridWidth = width - 24;
        const int buttonWidth = (gridWidth - buttonGap * (columns - 1)) / columns;
        std::size_t index = 0;

        for (const auto& definition : definitions)
        {
            if (definition.state != state)
            {
                continue;
            }

            const int column = static_cast<int>(index % columns);
            const int row = static_cast<int>(index / columns);
            addControl(
                controls,
                definition.label,
                {
                    gridX + column * (buttonWidth + buttonGap),
                    gridY + row * (buttonHeight + buttonGap),
                    buttonWidth,
                    buttonHeight
                },
                ControlType::Animation,
                state,
                definition.id
            );
            ++index;
        }
    }

    void addSoundControls(
        std::vector<Control>& controls,
        const Layout& layout
    )
    {
        constexpr int columns = 2;
        const int gridX = layout.soundsX + 12;
        const int gridY = layout.panelsY + 78;
        const int gridWidth = layout.soundsWidth - 24;
        const int buttonWidth = (gridWidth - layout.buttonGap) / columns;

        for (std::size_t index = 0; index < SOUND_TESTS.size(); ++index)
        {
            const int column = static_cast<int>(index % columns);
            const int row = static_cast<int>(index / columns);
            addControl(
                controls,
                SOUND_TESTS[index].label,
                {
                    gridX + column * (buttonWidth + layout.buttonGap),
                    gridY + row * (layout.buttonHeight + layout.buttonGap),
                    buttonWidth,
                    layout.buttonHeight
                },
                ControlType::Sound,
                desktoprobot::RobotState::Idle,
                desktoprobot::TestAnimationId::Blink,
                SOUND_TESTS[index].effect
            );
        }
    }

    void updateControlHover(std::vector<Control>& controls, int mouseX, int mouseY)
    {
        for (auto& control : controls)
        {
            control.button.hovered = desktoprobot::pointInRect(mouseX, mouseY, control.button.rect);
        }
    }

    void updateControlStates(
        std::vector<Control>& controls,
        desktoprobot::RobotState selectedState,
        const desktoprobot::AnimationPlayer& player
    )
    {
        for (auto& control : controls)
        {
            if (control.type == ControlType::State)
            {
                control.button.active = control.state == selectedState;
            }
            else if (control.type == ControlType::Animation)
            {
                control.button.active =
                    player.getCurrentAnimation() == &desktoprobot::getTestAnimation(control.animation);
            }
            else
            {
                control.button.active = false;
            }
        }
    }

    const char* getCurrentAnimationLabel(const desktoprobot::AnimationPlayer& player)
    {
        const auto& definitions = desktoprobot::getAnimationTestDefinitions();
        for (const auto& definition : definitions)
        {
            if (player.getCurrentAnimation() == &desktoprobot::getTestAnimation(definition.id))
            {
                return definition.label;
            }
        }
        return player.getCurrentAnimation() != nullptr
            ? player.getCurrentAnimation()->name
            : "NONE";
    }
}

int main()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "Desktop Robot Test Simulator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        0,
        0,
        SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP
    );

    if (!window)
    {
        std::cerr << "Window creation failed: " << SDL_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << '\n';
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    int screenWidth = 0;
    int screenHeight = 0;
    if (SDL_GetRendererOutputSize(renderer, &screenWidth, &screenHeight) != 0)
    {
        std::cerr << "SDL renderer size query failed: " << SDL_GetError() << '\n';
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    const Layout layout = createLayout(screenWidth, screenHeight);

    desktoprobot::SDLRenderer display;
    if (!display.initialize(renderer, layout.displayX, layout.displayY, layout.displayScale))
    {
        std::cerr << "SDL display initialization failed: " << SDL_GetError() << '\n';
        display.destroy();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    desktoprobot::SDLAudioBackend audioBackend;
    if (!audioBackend.initialize())
    {
        std::cerr << "SDL audio initialization failed: " << SDL_GetError() << '\n';
        display.destroy();
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    desktoprobot::SoundPlayer soundPlayer(audioBackend);
    desktoprobot::AnimationPlayer player;
    desktoprobot::RobotState selectedState = desktoprobot::RobotState::Idle;
    player.setLoopAnimation(desktoprobot::getStateAnimation(selectedState));
    desktoprobot::Framebuffer framebuffer;

    std::vector<Control> controls;
    controls.reserve(
        desktoprobot::getAnimationTestDefinitions().size() +
        SOUND_TESTS.size() + 3
    );
    addStateControls(
        controls,
        desktoprobot::RobotState::Idle,
        layout.idleX,
        layout.panelsY,
        layout.idleWidth,
        layout.buttonHeight,
        layout.buttonGap
    );
    addStateControls(
        controls,
        desktoprobot::RobotState::Tired,
        layout.tiredX,
        layout.panelsY,
        layout.stateWidth,
        layout.buttonHeight,
        layout.buttonGap
    );
    addStateControls(
        controls,
        desktoprobot::RobotState::Sleep,
        layout.tiredX,
        layout.sleepY,
        layout.stateWidth,
        layout.buttonHeight,
        layout.buttonGap
    );
    addSoundControls(controls, layout);

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
            else if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_h:
                    soundPlayer.playHappy();
                    break;
                case SDLK_s:
                    soundPlayer.playSuperHappy();
                    break;
                case SDLK_c:
                    soundPlayer.playCuddly();
                    break;
                case SDLK_a:
                    soundPlayer.playAngry();
                    break;
                default:
                    break;
                }
            }
            else if (event.type == SDL_MOUSEMOTION)
            {
                updateControlHover(controls, event.motion.x, event.motion.y);
            }
            else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
            {
                for (auto& control : controls)
                {
                    if (!desktoprobot::pointInRect(event.button.x, event.button.y, control.button.rect))
                    {
                        continue;
                    }

                    if (control.type == ControlType::State)
                    {
                        selectedState = control.state;
                        player.setLoopAnimation(desktoprobot::getStateAnimation(selectedState));
                    }
                    else if (control.type == ControlType::Animation)
                    {
                        selectedState = control.state;
                        player.setLoopAnimation(desktoprobot::getStateAnimation(selectedState));
                        player.playOneShot(desktoprobot::getTestAnimation(control.animation));

                        for (const auto& definition : desktoprobot::getAnimationTestDefinitions())
                        {
                            if (definition.id == control.animation && definition.playsSound)
                            {
                                soundPlayer.play(definition.sound);
                                break;
                            }
                        }
                    }
                    else
                    {
                        soundPlayer.play(control.sound);
                    }
                    break;
                }
            }
        }

        const uint32_t now = SDL_GetTicks();
        const uint32_t delta = now - lastFrameTime;
        lastFrameTime = now;

        player.update(delta);
        player.renderCurrentFrame(framebuffer);
        updateControlStates(controls, selectedState, player);

        SDL_SetRenderDrawColor(renderer, 10, 10, 12, 255);
        SDL_RenderClear(renderer);

        drawPanel(renderer, layout.idleX, layout.panelsY, layout.idleWidth, layout.panelHeight, "IDLE");
        drawPanel(renderer, layout.tiredX, layout.panelsY, layout.stateWidth, layout.tiredHeight, "TIRED");
        drawPanel(renderer, layout.tiredX, layout.sleepY, layout.stateWidth, layout.sleepHeight, "SLEEP");
        drawPanel(renderer, layout.soundsX, layout.panelsY, layout.soundsWidth, layout.panelHeight, "SOUNDS");
        desktoprobot::drawText(
            renderer,
            "KEYS H HAPPY S SUPER C CUDDLY A ANGRY",
            layout.soundsX + 15,
            layout.panelsY + 48,
            1,
            SDL_Color{ 185, 185, 195, 255 }
        );

        desktoprobot::drawText(
            renderer,
            "DESKTOP ROBOT TEST SIMULATOR",
            layout.margin,
            layout.titleY,
            2,
            SDL_Color{ 245, 245, 250, 255 }
        );
        desktoprobot::drawText(
            renderer,
            "MANUAL ANIMATION AND SOUND TESTS",
            layout.margin,
            layout.titleY + 34,
            1,
            SDL_Color{ 170, 170, 180, 255 }
        );
        desktoprobot::drawText(
            renderer,
            "STATE",
            layout.idleX,
            layout.panelsY - 30,
            2,
            SDL_Color{ 170, 190, 220, 255 }
        );
        desktoprobot::drawText(
            renderer,
            desktoprobot::getStateName(selectedState),
            layout.idleX + 52,
            layout.panelsY - 30,
            2,
            SDL_Color{ 255, 255, 255, 255 }
        );
        desktoprobot::drawText(
            renderer,
            "ANIMATION",
            layout.idleX + 150,
            layout.panelsY - 30,
            2,
            SDL_Color{ 170, 190, 220, 255 }
        );
        desktoprobot::drawText(
            renderer,
            getCurrentAnimationLabel(player),
            layout.idleX + 240,
            layout.panelsY - 30,
            2,
            SDL_Color{ 255, 255, 255, 255 }
        );

        SDL_Rect displayPanel{
            layout.displayPanelX,
            layout.panelsY,
            layout.displayPanelWidth,
            layout.panelHeight
        };
        SDL_SetRenderDrawColor(renderer, 25, 25, 30, 255);
        SDL_RenderFillRect(renderer, &displayPanel);
        SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
        SDL_RenderDrawRect(renderer, &displayPanel);
        desktoprobot::drawText(
            renderer,
            "128 X 64 FACE DISPLAY",
            layout.displayPanelX + 15,
            layout.panelsY + 12,
            1,
            SDL_Color{ 185, 185, 195, 255 }
        );

        SDL_Rect displayBackground{
            layout.displayX,
            layout.displayY,
            desktoprobot::Framebuffer::Width * layout.displayScale,
            desktoprobot::Framebuffer::Height * layout.displayScale
        };
        SDL_SetRenderDrawColor(renderer, 8, 8, 10, 255);
        SDL_RenderFillRect(renderer, &displayBackground);
        if (!display.render(framebuffer))
        {
            std::cerr << "SDL framebuffer rendering failed: " << SDL_GetError() << '\n';
            running = false;
            continue;
        }
        SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
        SDL_RenderDrawRect(renderer, &displayBackground);

        for (const auto& control : controls)
        {
            desktoprobot::drawButton(renderer, control.button);
        }

        SDL_RenderPresent(renderer);
    }

    display.destroy();
    audioBackend.shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
