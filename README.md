# Lux

## Overview

Lux is a windowing library that provides cross platform window creation, input management, and OpenGL
context creation.

It was written for educational purposes and is likely not very efficient.

## Building

### Dependencies

For Windows, Lux only requires the Win32 API which is always going to be present.

For Linux, Lux requires the development libraries of wayland and egl, as well as mesa egl.

### Compilation

This project uses CMake. The only supported platforms for this project are: Windows (10+), Linux (Wayland).

Assuming you have CMake and a C compiler, you can run the following commands inside the project directory:
1. `cmake -S . -B build`
2. `cmake --build build`

This will build a shared library and and place it inside `./bin/`.
