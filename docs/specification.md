# CHIP-8 Interpreter (Emulator)

## Specification

This project aims to implement a CHIP-8 interpreter. It should include emulation of the CPU (instructions, registers, timers, stack), memory and display with the ability to load CHIP-8 ROMs.

The display would be shown in a GUI with the option to also display instruction being executed and the current state of the registers and stack (and maybe also the memory).

When finished it should be possible to play most CHIP-8 games. The only missing feature might be the sound implementation (or it might only include a very basic version of it).

## Libraries: [raylib](https://github.com/raysan5/raylib), [raygui](https://github.com/raysan5/raygui), [raylib-cpp](https://github.com/RobLoach/raylib-cpp)

Raylib and raygui will be used for the GUI of the emulator. I might also use the raylib-cpp library, since it's a C++ object-oriented wrapper for raylib.

## Interface

The emulator will work as a GUI application, displaying the game, current state of CPU/memory (highlighting changes) and some buttons (close/stop game, load ROM,...). The game itself would be controlled with a keyboard.

It should also support launching the emulator through terminal with the ability to specify ROM path/location and enable/disable viewing current state of CPU/memory.