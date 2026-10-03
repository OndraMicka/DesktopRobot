cmake_minimum_required(VERSION 3.16)

project(desktop_robot_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 CONFIG REQUIRED)

add_executable(desktop_robot_simulator
    src/main.cpp
    src/Framebuffer.cpp
    src/SDLRenderer.cpp
    src/UI.cpp
    src/animation/Animation.cpp
    src/animation/AnimationPlayer.cpp
    src/animation/TestAnimations.cpp
    src/sound/SoundPlayer.cpp
    src/sound/backends/pc/SDLAudioBackend.cpp
)

target_include_directories(desktop_robot_simulator PRIVATE src)
target_link_libraries(desktop_robot_simulator PRIVATE SDL2::SDL2)
