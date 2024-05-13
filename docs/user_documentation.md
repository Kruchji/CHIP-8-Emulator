# User documentation

## Launching the emulator

Emulator is launched from the command line. The syntax help can be viewed by launching it with no options:

```
Usage: chip8emu filepath [scale] [instr/sec] [explanations] [color] [BGcolor]
(default: scale = 16, instr/sec = 840, explanations = false, color = ffcc01, BGcolor = 996700)
```

To launch the emulator with the default settings, just launch it with the path to your CHIP-8 ROM like so:

```
chip8emu ROMs/game.ch8
```

Any further options are optional, but they need to be specified in the correct order and format to take effect. Incorrectly formatted options are ignored, and default values are selected instead.

Options explanation:
 - **scale**: Specifies size of the window (scale * default CHIP-8 screen size)
 - **instr/sec**: Specifies the speed of the emulation, as this value varies between games. Beware that setting this value below 60 executes only one instruction per frame and lowers framerate (below intended 60 on CHIP-8).
 - **explanations**: Enables display of past instructions and their explanations.
 - **color**: Specifies primary color of pixels.
 - **BGcolor**: Specifies secondary color of pixels.

## Playing games

Any game inside the emulator is controlled using the CHIP-8 keypad layout which is mapped to the keyboard like this:

Keypad:
```
1 2 3 C
4 5 6 D
7 8 9 E
A 0 B F
```

Keyboard:
```
1 2 3 4
Q W E R
A S D F
Z X C V
```

The emulator itself also supports pressing Space to pause and Enter (when game is paused) to advance by one instruction.

For sound to be enabled, include a 'buzzer.wav' file next to the emulator executable. The default one is provided in the Assets folder.