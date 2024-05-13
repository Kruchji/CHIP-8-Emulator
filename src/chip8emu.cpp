
#include "raylib.h"

#include "chip8emu.hpp"
#include "chip8.hpp"

#include <iostream>
#include <stdexcept>

using namespace std;

int main(int argc, char** argv)
{
    SetTraceLogLevel(LOG_WARNING);    // suppress raylib info messages

    //============ Parse args ============//

    // display help message when no ROM file path is provided
    if (argc < 2) {
        cout << "Usage: chip8emu filepath [scale] [instr/sec] [explanations] [color] [BGcolor]" << endl;
        cout << "(default: scale = 16, instr/sec = 840, explanations = false, color = ffcc01, BGcolor = 996700)" << endl;
        return 1;
    }

    // get option values if provided, fallback to default on incorrect format

    int scale = 16;     // modifies size of the window
    if (argc >= 3 && isNumber(argv[2])) scale = stoi(argv[2]);

    int speed = 840;    // instructions per second
	if (argc >= 4 && isNumber(argv[3])) speed = stoi(argv[3]);

    bool enableExplanations = false;    // show instruction explanations at the bottom
    if (argc >= 5 && string(argv[4]) == "true") enableExplanations = true;

    unsigned int mainColor = 0xffcc01FF;    // color of displayed pixels
    if (argc >= 6 && isHexColor(argv[5])) mainColor = (stoul(argv[5], nullptr, 16) << 8) | 0xFF;   // converts string of hex digits to number, then appends full alpha channel (0xFF)

    unsigned int BGColor = 0x996700FF;      // color of background pixels
    if (argc >= 7 && isHexColor(argv[6])) BGColor = (stoul(argv[6], nullptr, 16) << 8) | 0xFF;     // see above


    //============ Run emulator ============//

    try {
        chip8 CHIP(scale, speed, enableExplanations, mainColor, BGColor);
        CHIP.loadROM(argv[1]);
        CHIP.run();
    }
    catch (const std::runtime_error& error) {
        cout << "Exception occured: " << error.what() << endl;
        return 1;
    }

    return 0;
}

// checks if all character all digits
bool isNumber(char* strNum) {

    while (*strNum) {
        if (!isdigit(*strNum)) return false;
        strNum++;
    }

    return true;
}


// checks if all characters are hex digits and length
bool isHexColor(char* strNum) {
    int hexLen = 0;
    while (*strNum) {
        if (!isxdigit(*strNum)) return false;
        strNum++;
        ++hexLen;
    }
    if (hexLen != 6) return false;      // color hex should be 6 characters

    return true;
}
