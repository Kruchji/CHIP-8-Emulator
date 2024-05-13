# CHIP-8 Emulator

CHIP-8 Emulator with memory + instruction display and frame advance created in C++ with raylib as a school project.

### Running the emulator

See the instructions in [user documentation](docs/user_documentation.md).

![Running GlitchGhost game](/Previews/preview1.png?raw=true "Running GlitchGhost game")

### Build instructions

To build this project first make sure raylib dependencie are installed, for example on Fedora:
```
dnf install alsa-lib-devel mesa-libGL-devel libX11-devel libXrandr-devel libXi-devel libXcursor-devel libXinerama-devel libatomic
```

Then:
```
cd src
mkdir build
cd build
cmake ..
cmake --build .
```

Raylib and Raylib-cpp are already included in [lib](lib) folder and linked automatically on build.

[chip8emu.cpp](src/chip8emu.cpp) is the main cpp file.

![MFF Logo example](/Previews/preview2.png?raw=true "MFF Logo example")