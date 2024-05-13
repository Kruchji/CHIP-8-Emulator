# Technical documentation

This project aims to emulate the CHIP-8, for more detailed information about its instructions, memory, etc. see [this technical reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM).

## General overview

The whole program consists of four .cpp files and their header files and two additional header files. Included are also example test ROMs in the ROMs folder and a default buzzer sound in the Assets folder.

## Code

### Code overview

Of the four .cpp files, chip8emu contains the main() function, chip8 contains most of the code of the emulator and includes the two remaining files which emulate the memory and display. The two additional header files are keymap.hpp and fontset.hpp which store the keyboard layout and the included hex font.

### chip8emu

This file contains the main function which parses user provided arguments, creates the emulator instance, loads the ROM and stats up the emulator. Included are also two functions for checking the validity of provided options.

If user inputs any invalid option, the program still runs, but it uses the default value for this option. The only more complicated part of this file is parsing the hex color code:

```
(stoul(argv[5], nullptr, 16) << 8) | 0xFF;
```

stould does the conversion from char* to the correct hex number, but then 0xFF is OR'd to add a full alpha channel. This channel needs to be specified when drawing with raylib, but I felt it would only make it harder when user tries to set a custom color, so full alpha channel is hardcoded here.

### chip8

This file is the core of the emulator as it enables loading and running the ROMs.

The chip8 class has many private members which cover all the registers, stack, memory, and display required to emulate the CHIP-8. Additionally, it has members for generating random numbers (bytes) and storing past instructions and their explanations.

#### Initialization

The constructor sets all of these to their default values and also initializes the display. On this display, the internal variables are also shown, so I've chosen to pass their const references to the display, so it can retrieve them when rendering the frame. I've also considered writing getter methods for all of them, but in my mind, this introduces overhead (at the speed the emulator is running) and makes the values accessible to anyone (as the methods would be public). The last thing in constructor is setting the speed of the emulator. CHIP-8 refreshed the screen (and lowered timers) 60 times per second but ran about 800 instructions per second (default value). When speed of this emulator is lowered below 60, it also lowers the refresh rate of the screen and timers. This behavior was picked by me, as the intended behavior of the CHIP-8 is not defined here.

Fontset is loaded at the beginning of RAM, reasoning is provided in the 'fontset' section.

When loading ROM, any binary file is accepted. This is intended behavior, as any sequence of bytes can be interpreted as CHIP-8 instructions. When an invalid operation (unknown instruction, stack overflow/underflow, out-of-bounds read,...) is to be executed, the emulator handles that specific exception and exits.

The program (program counter) starts at memory location 0x200 (512), so ROM is loaded here. The actual running loop of the emulator has two basic steps - emulate one frame (emulateOneFrame) and then check if user stopped (pressed Space) (checkForPauseInput) or exited the program. When paused it's possible to advance by one instruction at a time using Enter, which is done by calling the first loop step, but specifying to execute only one instruction and immediately render a new frame.

#### Execution loop

When emulating one frame, specified number of instructions are executed. Then Sound and Delay timers are lowered by one if not zero - the original CHIP-8 does this 60 times per second as well. Sound is played if Sound timer is non-zero (and loaded) and then the next frame is rendered.

The last large part of the file is dedicated to decoding and executing different instructions. Decoding is done using an unordered map with opcodes as keys and handler methods as values. There however need to be multiple of these decoding methods (and maps) because different opcodes require different nibbles (parts of the instruction) to match. These methods mask the instruction correctly and then attempt to index using this masked instruction as a key into their handler map. On success a handler method for that opcode is called. On failure an 'Unknown instruction' error is thrown.

The implementation of each opcode handler is usually self-explanatory (especially with the inclusion of explanations for display), so I'll only mention some interesting parts (mainly quirks) of them. **CALL** was mentioned in the technical reference to first increment the stack pointer and then write to stack. I've flipped this behavior as the original would have left the first stack space always empty. **AND, OR, XOR** have a quirk where they also set the flag register to zero. **SHIFT** instructions store the result into the second specified register (not necessarily the shifted one). **SUBTRACT_NEGATIVE** does normal subtraction, just with the operands flipped. **LOAD_KEY** instruction intentionally loops back to itself until a pressed key is detected. **STORE_BCD** takes a number from a register and converts it to its decimal representation (and stores that to memory). **STORE_REGS and LOAD_REGS** also increment the index register. And lastly any unknown opcode throws an exception.

I've intentionally skipped over the **DRAW** opcode as I'll explain the whole frame drawing process in the 'display' section.

#### Helper functions

At the end of the file, helper functions are provided to handle keyboard input (which uses the keymap array - see more in section 'keymap') and storing past instructions and their explanations. One helper function is also stored in the header - char_to_hex. It takes a number from 0 to 15 and converts it to the correct hex digit character.

### memory

In this file the emulator's RAM and function to access it are defined. CHIP-8 has 4kB of RAM, so it's represented here as a 4096-byte array. The functions provide read and write access while checking for out-of-bound errors.

### display

The ch8Display class not only handles drawing the game and all relevant information, but it also handles playing the buzzer and drawing the window icon. In its constructor it takes quite a few const references to private members of chip8 - this is done for quick access when displaying them on screen (also access is restricted to only the display class) - for explanation see section 'chip8'. Also in the constructor, the window icon is drawn, and a load of the buzzer sound is attempted.

The display's public functions serve as an interface for chip8 to interact with the display. Methods update and shouldClose are called repeatedly every frame. shouldClose only checks if the user is trying to close the window. update is then the main drawing function.

#### Drawing the frame

One frame of the game is stored as an array of bytes; however each pixel is only represented by one bit. That's why shifting is needed when drawing each pixel (as a rectangle with scaling) of the game screen. Its value then decides if the pixel is in the foreground or background and an appropriate color is then selected. The rest of the drawing functions just go through all the information that should be visible and draw it on the screen. setfill and setw are used here to pad some numbers with leading zeros.

#### Writing to the frame buffer

The second important task the display enables is writing to the frame buffer. This process starts in the **DRAW** instruction in chip8 where the memory location of the sprite to be drawn and coordinates on screen where to draw are extracted. Each row of a sprite is one byte, and this byte is passed to the display (along with coordinates). The beginning coordinates are taken modulo if they are offscreen except in cases where part of the sprite is visible -> then the second part gets clipped (this is a quirk of the CHIP-8). The writeToBuffer function gets the two potentially affected bytes from the frame buffer (sprite row is only one byte, but it can start at any position). They are then XOR'd with the sprite byte (bytes if not clipped). Boolean value is then returned which indicates if any pixel in the original frame buffer was turned from 1 to 0 (and this is tracked over the whole **DRAW** instruction in chip8).

#### Controlling the buzzer

Lastly there are methods for controlling the buzzer. The updateBuzzer needs to be called each frame when the sound is playing for raylib to play the sound. And when buzzer is stopped, it is rewound back to the beginning for better effect.

### keymap

CHIP-8 was originally controlled with 4x4 keypad. This emulator maps these keypad keys to the left side of the keyboard. The array for mapping keypad keys to real keyboard keys is included in this file. Anywhere this array is included, it's possible to get the corresponding keyboard key by directly indexing into the keymap array with the hex digit of the original keypad key.

### fontset

The original CHIP-8 included a default representation (for drawing) of the hex digits 0 to F stored at the address 0x050. The fontset header contains an array of bytes representing these characters. In chip8's constructor it is loaded into the emulator's memory, but I've chosen to store it at the very beginning (0x000). CHIP-8 does contain an instruction to load the digits based on the fontset starting address (FONTSET_START_ADDRESS) (so programs using this instruction to draw them are unaffected by this change), but some programs are instead hardcoded to just look for the fontset at the beginning of the program (so this makes them behave correctly).

## Libraries

This project uses two libraries: [raylib](https://github.com/raysan5/raylib) and [raylib-cpp](https://github.com/RobLoach/raylib-cpp). Both are included in the lib folder and linked on build automatically. For more information on how to build this project, see the README.md inside the src folder. One thing to keep in mind is that while these two libraries are included, they also have their own dependencies which need to be installed when building.

## Included ROMs

In the ROMs folder are included some example ROMs to test the functionality of the emulator. With the exception of **mff.ch8** (which is a simple program that draws the MFF logo) none of these were created by me.

Directly in the folder are actual game/program ROMs. One thing to keep in mind is that most of these were not created for the original hardware, so they require much higher emulation speed that the default (for CHIP-8) to run well. For example, the more advanced ones like tank.ch8, danm8kuTitle.ch8 or glitchGhost.ch8 should be run with around 10000 i/s. The very simple ones (mainly logos) suffer from the exact opposite problem as they are drawn instantly on default speed, so lowering it is recommended to see how the logo is drawn.

In the ROMs folder there is another TestSuite folder which contains the full CHIP-8 test suite by [Timendus](https://github.com/Timendus/chip8-test-suite). This emulator passes all included tests which cover instructions, drawing, flags, keypad, sound and quirks (one quirk is not implemented as it is mainly relevant to how the original hardware refreshed the screen even in the middle of writing to the frame buffer).

## Audio

The emulator does support playing the buzzer when Sound Timer is non-zero, but doesn't generate the sound itself. Instead on initialization the display looks for a 'buzzer.wav' file next to the executable (default sound file is included in the Assets folder). This file is then played from the start each time a buzzer should play. If the file is not found, only an info/warning message is printed to stdout about including this file to make sound play (and no exception is thrown).