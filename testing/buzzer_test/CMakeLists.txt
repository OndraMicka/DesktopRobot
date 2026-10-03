cmake_minimum_required(VERSION 3.16)

project(buzzer_test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(buzzer_test
    main.cpp
)
