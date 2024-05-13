#pragma once

#include "raylib.h"

#include <array>

constexpr int KEYPAD_KEYS = 16;

// used when mapping Chip-8 keypad controls to keyboards
constexpr std::array<KeyboardKey, KEYPAD_KEYS> keymap{
	KEY_X, KEY_ONE, KEY_TWO, KEY_THREE,				// 0 - 3
	KEY_Q, KEY_W, KEY_E, KEY_A, KEY_S, KEY_D,		// 4 - 9
	KEY_Z, KEY_C, KEY_FOUR, KEY_R, KEY_F, KEY_V		// A - F
};