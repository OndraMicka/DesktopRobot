cmake_minimum_required(VERSION 3.16)

project(desktop_robot_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CHARACTER_ANIMATION_SOURCE ../../esp32/src/display/animations/CharacterAnimations.cpp)

find_package(SDL2 CONFIG REQUIRED)

add_executable(desktop_robot_simulator
    src/main.cpp
    ../../esp32/src/display/animations/TouchWakeAnimation.cpp
    ../../esp32/src/display/animations/TouchLookUpAnimation.cpp
    ../../esp32/src/behavior/BehaviourProgram.cpp
    ../../esp32/src/sound/SoundData.cpp
    src/Framebuffer.cpp
    src/SDLRenderer.cpp
    src/ui/UI.cpp
    src/animation/Animation.cpp
    src/animation/AnimationPlayer.cpp
    src/animation/TestAnimations.cpp
    src/animation/PowerOnAnimation.cpp
    src/animation/PreviewAnimations.cpp
    ${CHARACTER_ANIMATION_SOURCE}
    src/sound/SoundPlayer.cpp
    src/sound/backends/pc/SDLAudioBackend.cpp
)

target_include_directories(desktop_robot_simulator PRIVATE src)
target_link_libraries(desktop_robot_simulator PRIVATE SDL2::SDL2)

include(CTest)
if(BUILD_TESTING)
    add_executable(sound_playback tests/SoundPlayback.cpp
        ../../esp32/src/sound/SoundData.cpp
        src/sound/SoundPlayer.cpp src/sound/backends/pc/SDLAudioBackend.cpp)
    target_include_directories(sound_playback PRIVATE src)
    target_link_libraries(sound_playback PRIVATE SDL2::SDL2)
    add_test(NAME sound_playback COMMAND sound_playback)
endif()

if(BUILD_TESTING)
    add_executable(preview_animations tests/PreviewAnimations.cpp src/Framebuffer.cpp
        src/animation/Animation.cpp src/animation/TestAnimations.cpp src/animation/PreviewAnimations.cpp ${CHARACTER_ANIMATION_SOURCE})
    target_include_directories(preview_animations PRIVATE src)
    add_test(NAME preview_animations COMMAND preview_animations)

    add_executable(export_character_animations tests/ExportCharacterAnimations.cpp src/animation/PreviewAnimations.cpp)
    target_include_directories(export_character_animations PRIVATE src)

    add_executable(state_transitions tests/StateTransitions.cpp src/Framebuffer.cpp src/animation/Animation.cpp src/animation/AnimationPlayer.cpp src/animation/TestAnimations.cpp ${CHARACTER_ANIMATION_SOURCE})
    target_include_directories(state_transitions PRIVATE src)
    add_test(NAME state_transitions COMMAND state_transitions)
endif()

if(BUILD_TESTING)
    file(GLOB_RECURSE FIRMWARE_TEST_SOURCES CONFIGURE_DEPENDS ../../esp32/src/*.cpp)
    list(FILTER FIRMWARE_TEST_SOURCES EXCLUDE REGEX "/main.cpp$")
    add_executable(behavior_tests ../../esp32/host/tests.cpp ${FIRMWARE_TEST_SOURCES})
    target_include_directories(behavior_tests PRIVATE ../../esp32/host/stubs ../../esp32/src)
    target_compile_definitions(behavior_tests PRIVATE ROBOT_COMPACT_ANIMATIONS)
    add_test(NAME behavior_tests COMMAND behavior_tests)
endif()
