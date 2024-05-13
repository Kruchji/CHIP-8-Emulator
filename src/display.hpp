#pragma once

#include "raylib.h"
#include "../lib/raylib-cpp-5.0.0/include/raylib-cpp.hpp"

#include <array>
#include <string>
#include <vector>
#include <cstdint>

constexpr int STACK_SIZE = 16;
constexpr int VREGS_COUNT = 16;			// number of Vx registers
constexpr int DISPLAY_LAST_COUNT = 3;	// number of displayed instructions on screen

// video = game/program screen
constexpr int VIDEO_WIDTH = 64;
constexpr int VIDEO_HEIGHT = 32;
constexpr int VIDEO_LINE_BYTES = VIDEO_WIDTH / 8;		// bytes to store one line
constexpr int EXPLANATIONS_HEIGHT = 8;		// height of added space for instruction explanations

constexpr int STANDARD_FPS = 60;	// applied unless cycle/frame is below this value

constexpr int ICON_SIZE = 256;		// window icon (in taskbar and such)

class ch8Display {
private:
	// size of window with scaling applied
	int scaleFactor = 16;
	int screenWidth = scaleFactor * VIDEO_WIDTH + scaleFactor * 16;
	int screenHeight = scaleFactor * VIDEO_HEIGHT + scaleFactor * EXPLANATIONS_HEIGHT;
	raylib::Window window;
	std::array<uint8_t, (VIDEO_WIDTH / 8) * VIDEO_HEIGHT> frameBuffer;		// stores all pixels of one frame

	// readonly references to emulator internals for display - for explanations see chip8.hpp/cpp
	uint16_t const& regPC_;
	uint16_t const& regI_;
	std::array<uint8_t, VREGS_COUNT> const& regsVx_;
	uint8_t const& regDT_;
	uint8_t const& regST_;
	uint8_t const& regSP_;
	std::array<uint16_t, STACK_SIZE> const& stack_;

	// readonly references to display last few instructions and their explanations
	std::vector<std::string> const& explanations_;
	std::array<uint16_t, DISPLAY_LAST_COUNT> const& lastInstructions_;
	bool enableExplanations_;

	// audio/buzzer members
	raylib::AudioDevice audio;  // Initialize audio device
	raylib::Music buzzer;
	bool buzzerLoaded;

	// color used when drawing
	raylib::Color contentColor;
	raylib::Color backgroundColor;
	
	// drawing methods called in update
	void drawScreen() const;
	void drawMemory() const;
	void drawInstructions() const;

public:
	ch8Display(int SF, int speed, uint16_t const& regPC, uint16_t const& regI, std::array<uint8_t, VREGS_COUNT> const& regsVx,
		uint8_t const& regDT, uint8_t const& regST, uint8_t const& regSP, std::array<uint16_t, STACK_SIZE> const& stack,
		std::vector<std::string> const& explanations, std::array<uint16_t, DISPLAY_LAST_COUNT> const& lastInstructions, bool enableExplanations,
		unsigned int mainColor, unsigned int BGColor);
	~ch8Display() noexcept;

	// called every frame
	void update();
	bool shouldClose() const;
	
	// frame buffer modification
	void clear();
	bool writeToBuffer(uint8_t spriteByte, uint16_t xCoord, uint16_t yCoord);

	// buzzer control
	void updateBuzzer();
	void stopBuzzer();
};