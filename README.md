# Lux

## Overview

Lux is a windowing library that provides cross platform window creation, input management, and OpenGL
context creation.

It was written for educational purposes and is likely not very efficient.

## Building

This project uses CMake. The only supported platforms for this project are: Windows (10+), Linux (Wayland).

Assuming you have CMake and a C compiler, you can run the following commands inside the project directory:
1. `cmake -S . -B build`
2. `cmake --build build`

This will build a dynamic library and and place it inside `./bin/`.

If you want to build the test executable, append `-D BUILD_TEST_EXECUTABLE=1` to the first build step.
