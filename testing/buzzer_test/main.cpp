#include <windows.h>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <system_error>

void playTone(unsigned int frequency, unsigned int duration)
{
    if (frequency < 37 || frequency > 32767)
    {
        throw std::invalid_argument("Tone frequency must be between 37 and 32767 Hz");
    }

    if (!Beep(frequency, duration))
    {
        throw std::system_error(
            static_cast<int>(GetLastError()),
            std::system_category(),
            "Failed to play tone"
        );
    }
}

void pause(unsigned int duration)
{
    Sleep(duration);
}

void playMelody()
{
    // bzzz - biii - bup - biiip
    playTone(420, 350);
    pause(80);

    playTone(500, 450);
    pause(80);

    playTone(620, 250);
    pause(60);

    playTone(560, 300);
    pause(80);

    playTone(700, 500);
    pause(100);

    playTone(760, 250);
    pause(60);

    playTone(680, 300);
    pause(80);

    playTone(820, 650);
    pause(120);

    // malé "spokojené" zakončení
    playTone(700, 200);
    pause(50);

    playTone(900, 700);
}

int main()
{
    try
    {
        playMelody();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }

    return 0;
}
