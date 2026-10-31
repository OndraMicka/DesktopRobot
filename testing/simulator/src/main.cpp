#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <algorithm>
#include <array>
#include <iostream>
#include <vector>

#include "Framebuffer.h"
#include "SDLRenderer.h"
#include "animation/AnimationPlayer.h"
#include "animation/TestAnimations.h"
#include "sound/SoundPlayer.h"
#include "sound/backends/pc/SDLAudioBackend.h"
#include "ui/UI.h"
#include "../../../esp32/src/behavior/BehaviourProgram.h"
#include <ctime>
#include "../../../esp32/src/display/animations/TouchWakeAnimation.h"
#include "../../../esp32/src/display/animations/TouchLookUpAnimation.h"
#include <cstdio>

namespace
{
    class SimulatorOutput : public AnimationOutput {
    public:
        explicit SimulatorOutput(desktoprobot::AnimationPlayer& player, desktoprobot::SoundPlayer& sound) : player_(player), sound_(sound) {}
        void playAnimation(AnimationId id, uint32_t, bool hold = false) override {
            const auto limit = hold ? desktoprobot::getExpressionFrameCount(id) : 0U;
            player_.play(desktoprobot::getTestAnimation(id), limit);
        }
        void playTouchWake(::RobotState source,uint32_t) override { player_.play(desktoprobot::TouchWakeAnimation::get(static_cast<desktoprobot::RobotState>(source))); }
        void playTouchLookUp(::RobotState source, uint32_t) override { player_.play(desktoprobot::TouchLookUpAnimation::get(static_cast<desktoprobot::RobotState>(source))); }
        void playBase(::RobotState state, uint32_t) override {
            player_.play(desktoprobot::getStateAnimation(static_cast<desktoprobot::RobotState>(state)));
        }
        void playShakeRecovery(uint32_t) override {
            player_.play(desktoprobot::getShakeRecoveryAnimation(player_.getCurrentFrame()));
        }
        void playTouchReturn(uint32_t) override { player_.play(desktoprobot::getTouchReturnAnimation(player_.getCurrentFrame())); }
        // New HOLD is an audition only; the removed HOLD variants are no
        // longer triggered by behavior. Keep the existing TAP sound hookup.
        void interactionSound(bool friendly) override { if (!friendly) sound_.play(desktoprobot::SoundEffect::Tap1); }
        bool isAnimationFinished() const override { return player_.isFinished(); }
        void showErrorFace() override { /* Firmware owns the hardware error screen. */ }
    private:
        desktoprobot::AnimationPlayer& player_;
        desktoprobot::SoundPlayer& sound_;
    };

    enum class ControlType
    {
        State,
        Animation,
        Sound,
        Input,
        Toggle
    };

    struct Control
    {
        desktoprobot::Button button;
        ControlType type;
        desktoprobot::RobotState state;
        desktoprobot::TestAnimationId animation;
        desktoprobot::SoundEffect sound;
        InputEvent input = InputEvent::NONE;
    };

    struct SoundTestDefinition
    {
        desktoprobot::SoundEffect effect;
        const char* label;
    };

    struct Panel
    {
        int x;
        int width;
        const char* title;
    };

    struct Layout
    {
        int width;
        int height;
        int margin;
        int panelY;
        int panelHeight;
        int buttonHeight;
        int buttonGap;
        int displayX;
        int displayY;
        int displayScale;
        std::array<Panel, 4> panels;
    };

    constexpr std::array<SoundTestDefinition, 7> SOUND_TESTS = {{
        { desktoprobot::SoundEffect::Shake1, "SHAKE 1 - HOLD" },
        { desktoprobot::SoundEffect::Shake2, "SHAKE 2 - HOLD" },
        { desktoprobot::SoundEffect::Tap1, "TAP 1" },
        { desktoprobot::SoundEffect::WakeUp, "SOUND - WAKE UP" },
        { desktoprobot::SoundEffect::FallAsleep, "SOUND - FALL ASLEEP" },
        { desktoprobot::SoundEffect::Hold, "SOUND - HOLD" },
        { desktoprobot::SoundEffect::PowerOn, "SOUND - POWER ON" }
    }};

    int getStateAnimationCount(desktoprobot::RobotState state)
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

    Layout createLayout(int width, int height)
    {
        Layout layout{};
        layout.width = width;
        layout.height = height;
        layout.margin = std::max(12, width / 100);
        layout.panelY = std::clamp(height / 6, 76, 160);
        layout.panelHeight = height - layout.panelY - layout.margin;
        layout.buttonGap = std::max(3, height / 216);

        const int panelGap = std::max(8, width / 120);
        const int availableWidth = width - 2 * layout.margin - 3 * panelGap;
        const int panelWidth = availableWidth / 4;
        for (std::size_t index = 0; index < layout.panels.size(); ++index)
        {
            layout.panels[index] = {
                layout.margin + static_cast<int>(index) * (panelWidth + panelGap),
                panelWidth,
                index == 0 ? "IDLE" : index == 1 ? "TIRED" : index == 2 ? "SLEEP" : "INPUT / SOUNDS"
            };
        }

        const int maximumRows = std::max({
            getStateAnimationCount(desktoprobot::RobotState::Idle) + 1,
            getStateAnimationCount(desktoprobot::RobotState::Tired) + 1,
            getStateAnimationCount(desktoprobot::RobotState::Sleep) + 1,
            static_cast<int>(SOUND_TESTS.size()) + 4
        });
        const int listHeight = layout.panelHeight - 60;
        const int desiredButtonHeight =
            (listHeight - (maximumRows - 1) * layout.buttonGap) / maximumRows;
        layout.buttonHeight = std::clamp(desiredButtonHeight, 12, 34);

        layout.displayScale = std::clamp((layout.panelY - 16) / desktoprobot::Framebuffer::Height, 1, 2);
        const int displayWidth = desktoprobot::Framebuffer::Width * layout.displayScale;
        const int displayHeight = desktoprobot::Framebuffer::Height * layout.displayScale;
        layout.displayX = width - layout.margin - displayWidth;
        layout.displayY = (layout.panelY - displayHeight) / 2;
        return layout;
    }

    void addControl(
        std::vector<Control>& controls,
        const char* label,
        SDL_Rect rect,
        ControlType type,
        desktoprobot::RobotState state = desktoprobot::RobotState::Idle,
        desktoprobot::TestAnimationId animation = desktoprobot::TestAnimationId::Blink,
        desktoprobot::SoundEffect sound = desktoprobot::SoundEffect::Shake1,
        InputEvent input = InputEvent::NONE
    )
    {
        controls.push_back({
            { label, rect, false, false },
            type,
            state,
            animation,
            sound,
            input
        });
    }

    void drawPanel(SDL_Renderer* renderer, const Panel& panel, int y, int height)
    {
        SDL_Rect rect{ panel.x, y, panel.width, height };
        SDL_SetRenderDrawColor(renderer, 25, 25, 30, 255);
        SDL_RenderFillRect(renderer, &rect);
        SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
        SDL_RenderDrawRect(renderer, &rect);
        desktoprobot::drawPanelHeader(renderer, panel.x + 10, y + 10, panel.width - 20, 28, panel.title);
    }

    void addStateControls(
        std::vector<Control>& controls,
        const Panel& panel,
        int y,
        int buttonHeight,
        int buttonGap,
        desktoprobot::RobotState state
    )
    {
        const int buttonX = panel.x + 12;
        const int buttonWidth = panel.width - 24;
        int buttonY = y + 48;
        const char* stateLabel = state == desktoprobot::RobotState::Idle
            ? "SET IDLE"
            : state == desktoprobot::RobotState::Tired ? "SET TIRED" : "SET SLEEP";

        addControl(
            controls,
            stateLabel,
            { buttonX, buttonY, buttonWidth, buttonHeight },
            ControlType::State,
            state
        );
        buttonY += buttonHeight + buttonGap;

        for (const auto& definition : desktoprobot::getAnimationTestDefinitions())
        {
            if (definition.state != state)
            {
                continue;
            }

            addControl(
                controls,
                definition.label,
                { buttonX, buttonY, buttonWidth, buttonHeight },
                ControlType::Animation,
                state,
                definition.id
            );
            buttonY += buttonHeight + buttonGap;
        }
    }

    void addSoundControls(
        std::vector<Control>& controls,
        const Panel& panel,
        int y,
        int buttonHeight,
        int buttonGap
    )
    {
        const int buttonX = panel.x + 12;
        const int buttonWidth = panel.width - 24;
        int buttonY = y + 48;
        for (auto input : {InputEvent::TAP, InputEvent::HOLD, InputEvent::RELEASE, InputEvent::NONE}) {
            const char* label = input == InputEvent::TAP ? "TAP" : input == InputEvent::HOLD ? "HOLD" : input == InputEvent::RELEASE ? "RELEASE" : "AUTO / PREVIEW";
            addControl(controls, label, {buttonX, buttonY, buttonWidth, buttonHeight},
                input == InputEvent::NONE ? ControlType::Toggle : ControlType::Input,
                desktoprobot::RobotState::Idle, desktoprobot::TestAnimationId::Blink, desktoprobot::SoundEffect::Tap1, input);
            buttonY += buttonHeight + buttonGap;
        }
        for (const auto& sound : SOUND_TESTS)
        {
            addControl(
                controls,
                sound.label,
                { buttonX, buttonY, buttonWidth, buttonHeight },
                ControlType::Sound,
                desktoprobot::RobotState::Idle,
                desktoprobot::TestAnimationId::Blink,
                sound.effect
            );
            buttonY += buttonHeight + buttonGap;
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
        const desktoprobot::AnimationPlayer& player,
        const Control* loopingSound
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
                control.button.active = &control == loopingSound;
            }
        }
    }

    const char* getCurrentAnimationLabel(const desktoprobot::AnimationPlayer& player)
    {
        for (const auto& definition : desktoprobot::getAnimationTestDefinitions())
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
    SimulatorOutput output(player, soundPlayer);
    BehaviourConfig config;
    config.randomSeed = static_cast<uint32_t>(std::time(nullptr));
    BehaviourProgram behavior(output, config);
    behavior.begin(SDL_GetTicks());
    bool automatic = true;
    int64_t timeOffset = 0;
    // Controls: A auto, T tap, H hold/release, arrows time.


    std::vector<Control> controls;
    controls.reserve(desktoprobot::getAnimationTestDefinitions().size() + SOUND_TESTS.size() + 3);
    addStateControls(
        controls,
        layout.panels[0],
        layout.panelY,
        layout.buttonHeight,
        layout.buttonGap,
        desktoprobot::RobotState::Idle
    );
    addStateControls(
        controls,
        layout.panels[1],
        layout.panelY,
        layout.buttonHeight,
        layout.buttonGap,
        desktoprobot::RobotState::Tired
    );
    addStateControls(
        controls,
        layout.panels[2],
        layout.panelY,
        layout.buttonHeight,
        layout.buttonGap,
        desktoprobot::RobotState::Sleep
    );
    addSoundControls(
        controls,
        layout.panels[3],
        layout.panelY,
        layout.buttonHeight,
        layout.buttonGap
    );

    bool touchMouseHeld = false;
    bool shakeKeyHeld = false;
    Control* loopingSound = nullptr;
    const auto stopSoundLoop = [&]()
    {
        if (loopingSound == nullptr) return;
        soundPlayer.stop();
        loopingSound = nullptr;
    };
    bool shakeMouseHeld = false;
    bool shaking = false;
    const auto updateShaking = [&]()
    {
        const bool active = shakeKeyHeld || shakeMouseHeld;
        if (active == shaking) return;
        shaking = active;
        if (shaking)
        {
            if (automatic) behavior.handleEvent(InputEvent::SHAKE_START);
            else player.setLoopAnimation(desktoprobot::getTestAnimation(desktoprobot::TestAnimationId::Shaking));
        }
        else
        {
            if (automatic) { behavior.handleEvent(InputEvent::SHAKE_END); return; }
            const auto& recovery = desktoprobot::getShakeRecoveryAnimation(player.getCurrentFrame());
            player.setLoopAnimation(desktoprobot::getStateAnimation(selectedState));
            player.playOneShot(recovery);
        }
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
            else if (event.type == SDL_KEYDOWN && event.key.repeat == 0)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_a:
                    automatic = !automatic;
                    if (automatic) behavior.begin(SDL_GetTicks());
                    break;
                case SDLK_t: if (automatic) behavior.handleEvent(InputEvent::TAP); break;
                case SDLK_h: if (automatic) behavior.handleEvent(InputEvent::HOLD); break;
                case SDLK_RIGHT: timeOffset += 300; break;
                case SDLK_LEFT: timeOffset -= 300; break;
                case SDLK_F7: { const auto t = std::time(nullptr); std::tm local = *std::localtime(&t); timeOffset = 7*3600-1-(local.tm_hour*3600+local.tm_min*60+local.tm_sec); break; }
                case SDLK_F8: { const auto t = std::time(nullptr); std::tm local = *std::localtime(&t); timeOffset = 21*3600-1-(local.tm_hour*3600+local.tm_min*60+local.tm_sec); break; }
                case SDLK_F9: { const auto t = std::time(nullptr); std::tm local = *std::localtime(&t); timeOffset = 21*3600+1800-1-(local.tm_hour*3600+local.tm_min*60+local.tm_sec); break; }
                case SDLK_SPACE:
                    shakeKeyHeld = true;
                    updateShaking();
                    break;
                default:
                    break;
                }
            }
            else if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_h) {
                if (automatic) behavior.handleEvent(InputEvent::RELEASE);
            }
            else if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_SPACE)
            {
                shakeKeyHeld = false;
                updateShaking();
            }
            else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
            {
                if (touchMouseHeld && automatic) behavior.handleEvent(InputEvent::RELEASE);
                touchMouseHeld = false;
                stopSoundLoop();
                shakeMouseHeld = false;
                updateShaking();
            }
            else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
            {
                stopSoundLoop();
                if (automatic) behavior.handleEvent(InputEvent::RELEASE);
                shakeKeyHeld = false;
                shakeMouseHeld = false;
                updateShaking();
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

                    if (control.type == ControlType::Toggle) {
                        automatic = !automatic;
                        if (automatic) behavior.begin(SDL_GetTicks());
                        break;
                    }
                    if (control.type == ControlType::Input) {
                        if (automatic) behavior.handleEvent(control.input);
                        touchMouseHeld = automatic && control.input == InputEvent::HOLD;
                        break;
                    }
                    if (control.type == ControlType::Animation
                        && control.animation == desktoprobot::TestAnimationId::Shaking)
                    {
                        shakeMouseHeld = true;
                        updateShaking();
                        break;
                    }
                    if (shaking && control.type != ControlType::Sound) break;

                    if (control.type != ControlType::Sound && automatic) {
                        if (control.type == ControlType::State) behavior.setState(static_cast<::RobotState>(control.state));
                        else behavior.preview(control.animation);
                        break;
                    }
                    if (control.type == ControlType::State)
                    {
                        const auto* transition = desktoprobot::getStateTransition(selectedState, control.state);
                        if (selectedState == control.state) break;
                        selectedState = control.state;
                        player.setLoopAnimation(desktoprobot::getStateAnimation(selectedState));
                        if (transition != nullptr)
                            player.playOneShot(desktoprobot::getTestAnimation(transition->id));
                    }
                    else if (control.type == ControlType::Animation)
                    {
                        selectedState = control.state;
                        for (const auto& definition : desktoprobot::getAnimationTestDefinitions())
                        {
                            if (definition.id == control.animation && definition.transition)
                                selectedState = definition.targetState;
                        }
                        player.setLoopAnimation(desktoprobot::getStateAnimation(selectedState));
                        player.playOneShot(desktoprobot::getTestAnimation(control.animation));
                    }
                    else
                    {
                        const bool loop = control.sound == desktoprobot::SoundEffect::Shake1
                            || control.sound == desktoprobot::SoundEffect::Shake2;
                        soundPlayer.play(control.sound, loop);
                        loopingSound = loop ? &control : nullptr;
                    }
                    break;
                }
            }
        }

        const uint32_t now = SDL_GetTicks();
        const uint32_t delta = now - lastFrameTime;
        lastFrameTime = now;
        player.update(delta);
        char clockLabel[180];
        if (automatic) {
            const auto epoch = std::time(nullptr) + timeOffset;
            const std::tm local = *std::localtime(&epoch);
            behavior.update(now, {static_cast<int64_t>((local.tm_year+1899)*365 + (local.tm_year+1899)/4 - (local.tm_year+1899)/100 + (local.tm_year+1899)/400 + local.tm_yday), static_cast<uint32_t>(local.tm_hour*3600+local.tm_min*60+local.tm_sec), true});
            selectedState = static_cast<desktoprobot::RobotState>(behavior.state());
            std::snprintf(clockLabel, sizeof(clockLabel), "AUTO %02d:%02d:%02d | A PREVIEW | T TAP | H HOLD | ARROWS 5 MIN | F7/F8/F9 SCHEDULE",local.tm_hour,local.tm_min,local.tm_sec);
        } else std::snprintf(clockLabel, sizeof(clockLabel), "PREVIEW | A AUTOMATIC BEHAVIOR");
        SDL_SetWindowTitle(window, clockLabel);
        player.renderCurrentFrame(framebuffer);
        updateControlStates(controls, selectedState, player, loopingSound);

        SDL_SetRenderDrawColor(renderer, 10, 10, 12, 255);
        SDL_RenderClear(renderer);

        desktoprobot::drawText(
            renderer,
            "DESKTOP ROBOT TEST SIMULATOR",
            layout.margin,
            std::max(8, layout.panelY / 5),
            2,
            SDL_Color{ 245, 245, 250, 255 }
        );
        desktoprobot::drawText(
            renderer,
            "STATE",
            layout.margin,
            layout.panelY / 2,
            1,
            SDL_Color{ 170, 190, 220, 255 }
        );
        desktoprobot::drawText(
            renderer,
            desktoprobot::getStateName(selectedState),
            layout.margin + 42,
            layout.panelY / 2,
            1,
            SDL_Color{ 255, 255, 255, 255 }
        );
        desktoprobot::drawText(
            renderer,
            "ANIMATION",
            layout.margin + 100,
            layout.panelY / 2,
            1,
            SDL_Color{ 170, 190, 220, 255 }
        );
        desktoprobot::drawText(
            renderer,
            getCurrentAnimationLabel(player),
            layout.margin + 166,
            layout.panelY / 2,
            1,
            SDL_Color{ 255, 255, 255, 255 }
        );

        SDL_Rect displayBackground{
            layout.displayX - 2,
            layout.displayY - 2,
            desktoprobot::Framebuffer::Width * layout.displayScale + 4,
            desktoprobot::Framebuffer::Height * layout.displayScale + 4
        };
        SDL_SetRenderDrawColor(renderer, 25, 25, 30, 255);
        SDL_RenderFillRect(renderer, &displayBackground);
        SDL_SetRenderDrawColor(renderer, 100, 100, 110, 255);
        SDL_RenderDrawRect(renderer, &displayBackground);
        if (!display.render(framebuffer))
        {
            std::cerr << "SDL framebuffer rendering failed: " << SDL_GetError() << '\n';
            running = false;
            continue;
        }

        for (const auto& panel : layout.panels)
        {
            drawPanel(renderer, panel, layout.panelY, layout.panelHeight);
        }
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
