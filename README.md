# Desktop Pet

A small electronic desktop companion with an animated face.

The device is built around an ESP32-C3 and uses a 1.3" OLED display to show different facial expressions and animations. It can react to touch and movement.

## Features

* Animated face
* Different facial expressions
* Eye movement and blinking
* Touch interaction
* Movement detection
* Sound effects
* Sleep mode in night
* Time synchronization via Wi-Fi

## Hardware

* ESP32-C3 Super Mini
* 1.3" 128×64 SH1106 OLED display
* TTP223 capacitive touch sensor
* MPU6050 accelerometer and gyroscope
* Passive piezo buzzer

## Software

The project is written in **C++**.

### PC Simulator

A C++/SDL2 simulator develops and tests the face, behavior and sounds before
running them on the physical device. On Windows with MSYS2 UCRT64 and SDL2:

```powershell
./testing/simulator/build.ps1
./testing/build/desktop_robot_simulator.exe
```

The local VS Code task `Build DesktopRobot Simulator` invokes the same build
script before `Run DesktopRobot Simulator`. The script includes shared firmware
sound data and copies the runtime DLLs beside the executable, so direct launch
does not depend on VS Code's PATH. Pass `-Compiler <path-to-g++.exe>` if needed.

The simulator will emulate the 128×64 OLED display on a computer.

### ESP32

The final firmware will run on the ESP32-C3 and control the physical OLED display, sensors and buzzer.

## Project Structure

```text
desktop-robot/
│
├── testing/            # PC testing applications
│   ├── simulator/       # PC simulator
│   │   ├── src/
│   │   │   ├── animation/
│   │   │   ├── sound/
│   │   │   │   └── backends/pc/
│   │   │   └── ui/
│   │   └── CMakeLists.txt
│   ├── testingFilmware/ # ESP32-C3 OLED and touch hardware test (PlatformIO)
│   └── CMakeLists.txt   # Builds the PC simulator
│
├── esp32/              # Modular ESP32-C3 firmware (PlatformIO)
│
└── docs/               # Project documentation
```

Build all PC testing applications with CMake using `testing/` as the source
directory. Build the ESP32 hardware test with
`pio run -d testing/testingFilmware -e esp32c3`; wiring and expected output are
documented in [the hardware test guide](docs/DesktopRobot_hardware_test.md).
The modular firmware lives in `esp32/`: build it with
`pio run -d esp32 -e esp32c3`. See [the firmware guide](esp32/README.md) for
wiring, architecture, upload instructions and verification.
Simulator sound descriptions are separate from the PC audio
backend, allowing the sound behavior to be reused with ESP32 buzzer output.

## Development

The project is currently in development.

The first step is to create a PC simulator that emulates the 128×64 OLED display. Facial expressions and animations will be developed and tested in the simulator before being implemented on the physical ESP32 device.

The goal is to keep the face and animation logic as independent from the display hardware as possible, allowing it to be reused between the simulator and the ESP32.

## Status

* [x] Project structure
* [x] C++ OLED simulator
* [x] Basic face
* [x] Facial animations
* [x] OLED implementation
* [x] Touch interaction
* [x] Movement detection
* [x] Sound effects
* [x] Sleep mode
* [x] Wi-Fi time synchronization
* [ ] 3D printed enclosure
* [ ] Final assembly

## Goal

The goal is to create a small interactive device that feels like a simple digital companion.
