
#include "display.hpp"

#include <sstream>
#include <iomanip>		// enables setfill() and setw() to pad numbers with zeros
#include <filesystem>
#include <iostream>

using namespace std;

ch8Display::ch8Display(int SF, int speed, uint16_t const& regPC, uint16_t const& regI, array<uint8_t, VREGS_COUNT> const& regsVx
, uint8_t const& regDT, uint8_t const& regST, uint8_t const& regSP , array<uint16_t, STACK_SIZE> const& stack
, std::vector<std::string> const& explanations, std::array<uint16_t, DISPLAY_LAST_COUNT> const& lastInstructions, bool enableExplanations
, unsigned int mainColor, unsigned int BGColor)
	: scaleFactor(SF), window(screenWidth, screenHeight - (enableExplanations ? 0 : scaleFactor * EXPLANATIONS_HEIGHT), "CHIP-8 Emulator"),	// make window smaller if explanations are disabled
	regPC_(regPC), regI_(regI), regsVx_(regsVx), regDT_(regDT), regST_(regST), regSP_(regSP), stack_(stack),
	explanations_(explanations), lastInstructions_(lastInstructions), enableExplanations_(enableExplanations),
	contentColor(mainColor), backgroundColor(BGColor)
{

	frameBuffer.fill(0);		// initialize as blank screen
	window.SetTargetFPS(speed);
	
	// draw and set window icon (in taskbar and such)
	raylib::Image icon(ICON_SIZE, ICON_SIZE, contentColor);
	icon.DrawText("8", ICON_SIZE/4, ICON_SIZE / 16, ICON_SIZE, BLACK);
	window.SetIcon(icon);

	// load buzzer if possible or print error
	const string buzzerPath = "buzzer.wav";
	if (filesystem::exists(buzzerPath)) {
		buzzer.Load(buzzerPath);
		buzzerLoaded = true;
	}
	else {
		buzzerLoaded = false;
		cout << "Failed to load buzzer sound file. Please make sure buzzer.wav exists." << endl;
	}
}

ch8Display::~ch8Display() noexcept {
	window.Close();
}

// detects when user clicks close on window
bool ch8Display::shouldClose() const {
    return window.ShouldClose();
}


//============ Drawing new frame ============//

void ch8Display::update() {
	window.BeginDrawing();

	window.ClearBackground(BLACK);
	drawScreen();	
	drawMemory();
	if (enableExplanations_) drawInstructions();

	window.EndDrawing();
}

// draw rectangle for each pixel of the game screen
void ch8Display::drawScreen() const {
	for (int y = 0; y < VIDEO_HEIGHT; ++y) {
		for (int x = 0; x < VIDEO_WIDTH; ++x) {

			// pick color according to frame buffer (0 = background pixel)
			// it stores pixels as bits in bytes so shifting is needed
			Color pixelColor = (frameBuffer[(y * VIDEO_LINE_BYTES) + (x / 8)] & (128 >> (x % 8))) ? contentColor : backgroundColor;

			DrawRectangle(x * scaleFactor, y * scaleFactor, scaleFactor, scaleFactor, pixelColor);
		}
	}
}

// draw registers and stack display to the right
void ch8Display::drawMemory() const {
	DrawText(("PC: " + to_string(regPC_)).c_str(), scaleFactor * (VIDEO_WIDTH + 5), scaleFactor * 1, static_cast<int>(scaleFactor * 1.5), WHITE);
	DrawText(("I: " + to_string(regI_)).c_str(), scaleFactor * (VIDEO_WIDTH + 2), scaleFactor * 3, static_cast<int>(scaleFactor * 1.5), PURPLE);
	DrawText(("SP: " + to_string(regSP_)).c_str(), scaleFactor * (VIDEO_WIDTH + 2), scaleFactor * 5, static_cast<int>(scaleFactor * 1.5), BLUE);

	DrawText(("DT: " + to_string(regDT_)).c_str(), scaleFactor * (VIDEO_WIDTH + 10), scaleFactor * 3, static_cast<int>(scaleFactor * 1.5), GREEN);
	DrawText(("ST: " + to_string(regST_)).c_str(), scaleFactor * (VIDEO_WIDTH + 10), scaleFactor * 5, static_cast<int>(scaleFactor * 1.5), GREEN);

	for (int i = 0; i < VREGS_COUNT; ++i) {
		stringstream ss;
		ss << std::hex << i;
		DrawText(("V" + ss.str() + ": " + to_string(regsVx_[i])).c_str(), scaleFactor * (VIDEO_WIDTH + 10), static_cast<int>(scaleFactor * (7 + 1.5 * i)), static_cast<int>(scaleFactor * 1.2), YELLOW);
		DrawText(("S" + ss.str() + ": " + to_string(stack_[i])).c_str(), scaleFactor * (VIDEO_WIDTH + 2), static_cast<int>(scaleFactor * (7 + 1.5 * i)), static_cast<int>(scaleFactor * 1.2), SKYBLUE);
	}

}

// draw past instructions and their explanations at the bottom
void ch8Display::drawInstructions() const{
	for (int i = 0; i < DISPLAY_LAST_COUNT; ++i) {
		stringstream ss;
		ss << std::hex << std::uppercase << setfill('0') << setw(4) << lastInstructions_[i];	// show as uppercase hex number padded by zeros to 4 digits

		// draw just executed instruction white, rest of them gray
		DrawText((ss.str() + ": " + explanations_[i]).c_str(), scaleFactor * 4, scaleFactor * ((VIDEO_HEIGHT + 1) + 2 * i), static_cast<int>(scaleFactor * 1.5), (i == DISPLAY_LAST_COUNT - 1) ? WHITE : GRAY);
	}
}


//============ Writing to frame buffer ============//

void ch8Display::clear() {
	frameBuffer.fill(0);
}

// returns true if any pixel was erased
bool ch8Display::writeToBuffer(uint8_t spriteByte, uint16_t xCoord, uint16_t yCoord) {
	xCoord %= VIDEO_WIDTH;		// wrap around if offscreen at the start

	if (yCoord < VIDEO_HEIGHT) {	// clip sprite that is partially offscreen (starting yCoord is already modulo VIDEO_HEIGHT)

		// buffer saves whole bytes but sprite can start in the middle of a byte -> get current value of both possibly affected bytes
		uint8_t oldFirstBufferVal = frameBuffer[(yCoord * VIDEO_LINE_BYTES) + (xCoord / 8)];
		uint8_t oldSecondBufferVal = frameBuffer[(yCoord * VIDEO_LINE_BYTES) + (((xCoord / 8) + 1) % VIDEO_LINE_BYTES)];	// % is here because second one could be wrapped

		// yCoord * VIDEO_LINE_BYTES to skip whole line(s)
		// XOR new values with current values
		frameBuffer[(yCoord * VIDEO_LINE_BYTES) + (xCoord / 8)] ^= spriteByte >> (xCoord % 8);
		// condition to clip sprite if only half is offscreen
		if ((xCoord / 8) + 1 < VIDEO_LINE_BYTES) frameBuffer[(yCoord * VIDEO_LINE_BYTES) + (((xCoord / 8) + 1) % VIDEO_LINE_BYTES)] ^= spriteByte << (8 - (xCoord % 8));

		// check if any pixel was erased by drawing this -> look at zeros in frameBuffer now and bitmask with previous
		return ((oldFirstBufferVal & ~frameBuffer[(yCoord * VIDEO_LINE_BYTES) + (xCoord / 8)]) != 0) ||
			((oldSecondBufferVal & ~frameBuffer[(yCoord * VIDEO_LINE_BYTES) + (((xCoord / 8) + 1) % 8)]) != 0);
	}
	
	return false;	// nothing drawn -> no pixels erased
}


//============ Buzzer control ============//

void ch8Display::updateBuzzer(){
	if (buzzerLoaded) {
		if (!buzzer.IsPlaying()) buzzer.Play();
		buzzer.Update();	// needs to be called every frame (when the sound is playing)
	}
}

void ch8Display::stopBuzzer(){
	if (buzzerLoaded) {
		buzzer.Pause();
		buzzer.Seek(0);		// always play from the start
	}
}