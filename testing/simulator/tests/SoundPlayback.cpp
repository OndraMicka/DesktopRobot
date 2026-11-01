#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "sound/SoundPlayer.h"
#include "sound/backends/pc/SDLAudioBackend.h"
#include <cassert>
#include <iostream>

int main() {
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_AUDIODRIVER, "dummy");
    desktoprobot::SDLAudioBackend backend;
    assert(!backend.isPlaying());
    if (!backend.initialize()) {
        std::cerr << "Dummy SDL audio initialization failed: " << SDL_GetError() << '\n';
        return 1;
    }
    desktoprobot::SoundPlayer player(backend);
    using desktoprobot::SoundEffect;
    for (unsigned id=0;id<static_cast<unsigned>(SoundEffect::Count);++id) {
        player.play(static_cast<SoundEffect>(id),true);
        assert(player.isPlaying());
        player.stop(); assert(!player.isPlaying());
    }
    player.play(SoundEffect::Tap1);
    const auto started=SDL_GetTicks();
    assert(player.isPlaying());
    while (player.isPlaying() && SDL_GetTicks()-started<2000) SDL_Delay(5);
    assert(!player.isPlaying()); // Natural completion, including the pause.
    player.play(SoundEffect::Shake1,true);
    SDL_Delay(350); assert(player.isPlaying()); // Survives a full loop.
    player.play(SoundEffect::Hold); assert(player.isPlaying());
    player.stop(); assert(!player.isPlaying());
    player.play(SoundEffect::Count); assert(!player.isPlaying());
    backend.shutdown(); assert(!player.isPlaying());
    std::cout << "PASS: shared scores through SDL, completion, loop, replacement, stop and playback status\n";
}
