
#include "raylib.h"

#include "chip8.hpp"

#include <iostream>
#include <iomanip>		// enables setfill() and setw() to pad numbers with zeros
#include <fstream>
#include <stdexcept>
#include <limits>

using namespace std;

chip8::chip8(int scale, int speed, bool enableExplanations, unsigned int mainColor, unsigned int BGColor)
	: generator(rd()), distChar(0, numeric_limits<uint8_t>::max())		// setup RNG
	, display(scale, (speed >= STANDARD_FPS) ? STANDARD_FPS : speed,		// speed too low -> lower framerate
		regPC, regI, regsVx, regDT, regST, regSP, stack,
		explanations, lastInstructions, enableExplanations,
		mainColor, BGColor)
	, enableExplanations(enableExplanations)
{
	// setup starting RAM content
	loadFontset();

	// start with empty registers and stack
	regsVx.fill(0);
	stack.fill(0);

	instructionsPerCycle = (speed >= STANDARD_FPS) ? (speed / STANDARD_FPS) : 1;	// if too low -> 1 instruction per frame

	// empty - no last instructions yet
	lastInstructions.fill(0);
	explanations = {"","",""};
}

// load fontset to RAM
void chip8::loadFontset() {
	for (uint16_t i = 0; i < fontset.size(); ++i) {
		memory.writeAtPos(FONTSET_START_ADDRESS + i, fontset[i]);
	}
}

// attempt to load ROM from specified file path
void chip8::loadROM(const string& fileName) {
	ifstream file(fileName, ios::binary);
	if (!file.good()) throw runtime_error("Couldn't load ROM file!");

	int fileByte = file.get();				// store initially as int -> EOF == -1
	uint16_t position = PC_START_ADDRESS;

	// load ROM = store to RAM
	while (file.good()) {
		memory.writeAtPos(position, static_cast<uint8_t>(fileByte));

		fileByte = file.get();
		++position;
	}
}

//============ Emulator execution loop ============//

// main emulator loop
void chip8::run() {
	regPC = PC_START_ADDRESS;

	// run until user closes the window
	while (!display.shouldClose()) {
		
		emulateOneFrame(instructionsPerCycle);

		checkForPauseInput();

	}
}

void chip8::emulateOneFrame(int IPC) {
	// execute specified number of instructions in one cycle/frame
	for (int i = 0; i < IPC; ++i) {
		uint16_t instruction = memory.readInstuctionAtPos(regPC);

		if (enableExplanations) updateLastInstructions(instruction);

		executeInstruction(instruction);

		regPC += INSTRUCTION_BYTES;		// increment program counter after each instruction
	}

	// lower timers each frame
	if (regDT != 0) --regDT;
	if (regST != 0) {
		--regST;

		// play buzzer sound
		display.updateBuzzer();
		if (regST == 0) display.stopBuzzer();
	}

	// show new frame
	display.update();
}

// enable pausing on space press and instruction advancing with enter
void chip8::checkForPauseInput() {
	if (GetKeyPressed() == KEY_SPACE) {
		while (true) {
			PollInputEvents();					// refrest pressed keys
			int keyPressed = GetKeyPressed();
			if (keyPressed == KEY_ENTER) {
				emulateOneFrame(1);				// advance one instruction
			}
			else if (keyPressed == KEY_SPACE) {
				break;							// resume normal operation
			}
		}
	}
}

// decodes and executes an instruction (based on left nibble) or calls another decoding method
void chip8::executeInstruction(uint16_t instruction) {

	// access handler function for opcode using a map and a masked instruction
	auto it = opcodeHandlers.find(instruction & 0xF000);
	if (it != opcodeHandlers.end()) {
		it->second(instruction);			// call the handler function with the full instruction
	}
	else {
		throw runtime_error("Unknown instruction!");	// throw on any unknown opcode
	}
}

// decodes and executes an instruction (all bits must match with opcode)
void chip8::executeMatchFullInstruction(uint16_t instruction) {
	auto it = opcodeMatchFullHandlers.find(instruction);
	if (it != opcodeMatchFullHandlers.end()) {
		it->second();						// call the handler function
	}
	else {
		throw runtime_error("Unknown instruction!");	// throw on any unknown opcode
	}
}

// decodes and executes an instruction (first and last nibble must match with opcode)
void chip8::executeMatchLastOneInstruction(uint16_t instruction) {
	auto it = opcodeMatchLastOneHandlers.find(instruction & 0xF00F);
	if (it != opcodeMatchLastOneHandlers.end()) {
		it->second(instruction);			// call the handler function with the full instruction
	}
	else {
		throw runtime_error("Unknown instruction!");	// throw on any unknown opcode
	}
}

// decodes and executes an instruction (first and last two nibbles must match with opcode)
void chip8::executeMatchLastTwoInstruction(uint16_t instruction) {
	auto it = opcodeMatchLastTwoHandlers.find(instruction & 0xF0FF);
	if (it != opcodeMatchLastTwoHandlers.end()) {
		it->second(instruction);			// call the handler function with the full instruction
	}
	else {
		throw runtime_error("Unknown instruction!");	// throw on any unknown opcode
	}
}


//============ Opcode handlers ============//

void chip8::clearHandler() {
	display.clear();

	if (enableExplanations) addNewExplanation("Clear the display.");
};

void chip8::returnHandler() {
	if (regSP == 0) throw runtime_error("Stack underflow!");
	--regSP;
	regPC = stack[regSP];

	if (enableExplanations) addNewExplanation("Return from a subroutine.");
};

void chip8::jumpHandler(uint16_t instruction) {
	regPC = instruction & 0x0FFF;
	regPC -= INSTRUCTION_BYTES;		// jump gives exact address -> this prevents increasing PC later

	if (enableExplanations) addNewExplanation("Jump to location " + to_string(instruction & 0x0FFF));
};

void chip8::callHandler(uint16_t instruction) {							// stores current PC on stack
	if (regSP == STACK_SIZE) throw runtime_error("Stack overflow!");

	// some documents say stack pointer should be incremented first but that leaves first stack space empty
	stack[regSP] = regPC;
	++regSP;

	regPC = instruction & 0x0FFF;
	regPC -= INSTRUCTION_BYTES;

	if (enableExplanations) addNewExplanation("Call subroutine at " + to_string(instruction & 0x0FFF));
};

void chip8::skipIfEqualHandler(uint16_t instruction) {
	if (regsVx[(instruction & 0x0F00) >> 8] == (instruction & 0x00FF)) {	// by shifting we can directly index into Vx registers
		regPC += INSTRUCTION_BYTES;
	}

	if (enableExplanations) addNewExplanation("Skip next instruction if V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" == ") + to_string(instruction & 0x00FF));
};

void chip8::skipIfNotEqualHandler(uint16_t instruction) {
	if (regsVx[(instruction & 0x0F00) >> 8] != (instruction & 0x00FF)) {
		regPC += INSTRUCTION_BYTES;
	}

	if (enableExplanations) addNewExplanation("Skip next instruction if V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" != ") + to_string(instruction & 0x00FF));
};

void chip8::skipIfRegsEqualHandler(uint16_t instruction) {
	if (regsVx[(instruction & 0x0F00) >> 8] == regsVx[(instruction & 0x00F0) >> 4]) {
		regPC += INSTRUCTION_BYTES;
	}

	if (enableExplanations) addNewExplanation("Skip next instruction if V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" == V") + string(1, char_to_hex((instruction & 0x00F0) >> 4)));
};

void chip8::loadImmediateHandler(uint16_t instruction) {
	regsVx[(instruction & 0x0F00) >> 8] = instruction & 0x00FF;

	if (enableExplanations) addNewExplanation("Set V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" = ") + to_string(instruction & 0x00FF));
};

void chip8::addImmediateHandler(uint16_t instruction) {
	regsVx[(instruction & 0x0F00) >> 8] += instruction & 0x00FF;

	if (enableExplanations) addNewExplanation("Add " + to_string(instruction & 0x00FF) + string(" to V") + string(1, char_to_hex((instruction & 0x0F00) >> 8)));
};

void chip8::loadHandler(uint16_t instruction) {
	regsVx[(instruction & 0x0F00) >> 8] = regsVx[(instruction & 0x00F0) >> 4];

	if (enableExplanations) addNewExplanation("Set V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" = V") + string(1, char_to_hex((instruction & 0x00F0) >> 4)));
};

void chip8::orHandler(uint16_t instruction) {
	regsVx[(instruction & 0x0F00) >> 8] |= regsVx[(instruction & 0x00F0) >> 4];
	regsVx[0xF] = 0;	// quirk: "The AND, OR and XOR opcodes (8xy1, 8xy2 and 8xy3) reset the flags register to zero."

	if (enableExplanations) addNewExplanation("Set V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" = V") + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" OR V") + string(1, char_to_hex((instruction & 0x00F0) >> 4)));
};

void chip8::andHandler(uint16_t instruction) {
	regsVx[(instruction & 0x0F00) >> 8] &= regsVx[(instruction & 0x00F0) >> 4];
	regsVx[0xF] = 0;

	if (enableExplanations) addNewExplanation("Set V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" = V") + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" AND V") + string(1, char_to_hex((instruction & 0x00F0) >> 4)));
};

void chip8::xorHandler(uint16_t instruction) {
	regsVx[(instruction & 0x0F00) >> 8] ^= regsVx[(instruction & 0x00F0) >> 4];
	regsVx[0xF] = 0;

	if (enableExplanations) addNewExplanation("Set V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" = V") + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" XOR V") + string(1, char_to_hex((instruction & 0x00F0) >> 4)));
};

void chip8::addHandler(uint16_t instruction) {
	uint8_t oldVx = regsVx[(instruction & 0x0F00) >> 8];

	regsVx[(instruction & 0x0F00) >> 8] += regsVx[(instruction & 0x00F0) >> 4];

	// set flag register accordingly to carry
	if (oldVx > regsVx[(instruction & 0x0F00) >> 8]) {
		regsVx[0xF] = 1;
	}
	else {
		regsVx[0xF] = 0;
	}

	if (enableExplanations) addNewExplanation("Add V" + string(1, char_to_hex((instruction & 0x00F0) >> 4)) + string(" to V") + string(1, char_to_hex((instruction & 0x0F00) >> 8)));
};

void chip8::subtractHandler(uint16_t instruction) {
	bool flagBit = regsVx[(instruction & 0x0F00) >> 8] >= regsVx[(instruction & 0x00F0) >> 4];
	regsVx[(instruction & 0x0F00) >> 8] -= regsVx[(instruction & 0x00F0) >> 4];

	// set flag register accordingly to not borrow
	if (flagBit) {
		regsVx[0xF] = 1;
	}
	else {
		regsVx[0xF] = 0;
	}

	if (enableExplanations) addNewExplanation("Subtract V" + string(1, char_to_hex((instruction & 0x00F0) >> 4)) + string(" from V") + string(1, char_to_hex((instruction & 0x0F00) >> 8)));
};

void chip8::shiftRightHandler(uint16_t instruction) {
	uint8_t flagBit = regsVx[(instruction & 0x00F0) >> 4] & 0x01;
	regsVx[(instruction & 0x0F00) >> 8] = regsVx[(instruction & 0x00F0) >> 4] >> 1;		// quirk -> stores shifted Vy into Vx
	regsVx[0xF] = flagBit;

	if (enableExplanations) addNewExplanation("Set V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" = V") + string(1, char_to_hex((instruction & 0x00F0) >> 4)) + string(" shifted right by 1"));
};

void chip8::subtractNegativeHandler(uint16_t instruction) {
	bool flagBit = regsVx[(instruction & 0x00F0) >> 4] >= regsVx[(instruction & 0x0F00) >> 8];
	regsVx[(instruction & 0x0F00) >> 8] = regsVx[(instruction & 0x00F0) >> 4] - regsVx[(instruction & 0x0F00) >> 8];

	// set flag register accordingly to not borrow
	if (flagBit) {
		regsVx[0xF] = 1;
	}
	else {
		regsVx[0xF] = 0;
	}

	if (enableExplanations) addNewExplanation("Set V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" = V") + string(1, char_to_hex((instruction & 0x00F0) >> 4)) + string(" - V") + string(1, char_to_hex((instruction & 0x0F00) >> 8)));
};

void chip8::shiftLeftHandler(uint16_t instruction) {
	unsigned char flagBit = (regsVx[(instruction & 0x00F0) >> 4] & 0x80) >> 7;
	regsVx[(instruction & 0x0F00) >> 8] = regsVx[(instruction & 0x00F0) >> 4] << 1;		// quirk - see SHIFT_RIGHT
	regsVx[0xF] = flagBit;

	if (enableExplanations) addNewExplanation("Set V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" = V") + string(1, char_to_hex((instruction & 0x00F0) >> 4)) + string(" shifted left by 1"));
};

void chip8::skipIfRegsNotEqualHandler(uint16_t instruction) {
	if (regsVx[(instruction & 0x0F00) >> 8] != regsVx[(instruction & 0x00F0) >> 4]) {
		regPC += INSTRUCTION_BYTES;
	}

	if (enableExplanations) addNewExplanation("Skip next instruction if V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" != V") + string(1, char_to_hex((instruction & 0x00F0) >> 4)));
};

void chip8::loadAddressHandler(uint16_t instruction) {
	regI = instruction & 0x0FFF;

	if (enableExplanations) addNewExplanation("Load address " + to_string(instruction & 0x0FFF) + string(" to I"));
};

void chip8::jumpPlusV0Handler(uint16_t instruction) {
	regPC = (instruction & 0x0FFF) + regsVx[0x0];
	regPC -= INSTRUCTION_BYTES;						// jump gives exact address -> this prevents increasing PC later

	if (enableExplanations) addNewExplanation("Jump to location " + to_string(instruction & 0x0FFF) + string(" + V0"));
};

void chip8::randomHandler(uint16_t instruction) {
	uint8_t randomNum = static_cast<uint8_t>(distChar(generator));
	regsVx[(instruction & 0x0F00) >> 8] = randomNum & (instruction & 0x00FF);

	if (enableExplanations) addNewExplanation("Set V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" = random byte AND ") + to_string(instruction & 0x00FF));
};

void chip8::drawHandler(uint16_t instruction) {
	// sprite coordinates on screen
	uint16_t xCoord = regsVx[(instruction & 0x0F00) >> 8] % VIDEO_WIDTH;
	uint16_t yCoord = regsVx[(instruction & 0x00F0) >> 4] % VIDEO_HEIGHT;

	// get sprite location in memory
	uint16_t spriteStart = regI;
	uint16_t spriteEnd = spriteStart + (instruction & 0x000F);

	bool erasedPixels = false;
	for (uint16_t iSprite = spriteStart; iSprite < spriteEnd; ++iSprite) {		// draw all bytes of the sprite
		uint8_t spriteByte = memory.readAtPos(iSprite);
		erasedPixels = display.writeToBuffer(spriteByte, xCoord, yCoord + (iSprite - spriteStart)) || erasedPixels;		// tracks if pixels were erased at any point
	}

	// sets flag register to 1 if any pixels were erased
	if (erasedPixels) {
		regsVx[0xF] = 1;
	}
	else {
		regsVx[0xF] = 0;
	}

	if (enableExplanations) addNewExplanation("Draw " + to_string(instruction & 0x000F) + string("-byte sprite starting at memory location I at (V") + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(", V") + string(1, char_to_hex((instruction & 0x00F0) >> 4)) + string(")"));
};

void chip8::skipIfKeyHandler(uint16_t instruction) {
	if (checkKeyDown(regsVx[(instruction & 0x0F00) >> 8])) {
		regPC += INSTRUCTION_BYTES;
	}

	if (enableExplanations) addNewExplanation("Skip next instruction if key with the value of V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" is pressed"));
};

void chip8::skipIfNotKeyHandler(uint16_t instruction) {
	if (!checkKeyDown(regsVx[(instruction & 0x0F00) >> 8])) {
		regPC += INSTRUCTION_BYTES;
	}

	if (enableExplanations) addNewExplanation("Skip next instruction if key with the value of V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" is not pressed"));
};

void chip8::loadDelayHandler(uint16_t instruction) {
	regsVx[(instruction & 0x0F00) >> 8] = regDT;

	if (enableExplanations) addNewExplanation("Set V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" to delay timer value"));
};

void chip8::loadKeyHandler(uint16_t instruction) {
	uint8_t pressedKey = getKeypadPressed();
	if (pressedKey > 0x0F) {				// > 0x0F -> nothing on keypad pressed
		regPC -= INSTRUCTION_BYTES;			// waits for key input
	}
	else {
		regsVx[(instruction & 0x0F00) >> 8] = pressedKey;
	}

	if (enableExplanations) addNewExplanation("Wait for a key press, store the value of the key in V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)));
};

void chip8::setDelayHandler(uint16_t instruction) {
	regDT = regsVx[(instruction & 0x0F00) >> 8];

	if (enableExplanations) addNewExplanation("Set delay timer to V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)));
};

void chip8::setSoundHandler(uint16_t instruction) {
	regST = regsVx[(instruction & 0x0F00) >> 8];

	if (enableExplanations) addNewExplanation("Set sound timer to V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)));
};

void chip8::addToIHandler(uint16_t instruction) {
	regI += regsVx[(instruction & 0x0F00) >> 8];

	if (enableExplanations) addNewExplanation("Add V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" to I"));
};

void chip8::loadDigitHandler(uint16_t instruction) {
	if (regsVx[(instruction & 0x0F00) >> 8] > (FONTSET_CHAR_COUNT - 1)) throw runtime_error("Trying to access font symbol out of range!");
	regI = FONTSET_START_ADDRESS + (CHARACTER_BYTES * regsVx[(instruction & 0x0F00) >> 8]);		// move to the correct hex character

	if (enableExplanations) addNewExplanation("Set I to the location of sprite for digit V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)));
};

void chip8::storeBCDHandler(uint16_t instruction) {					// BCD = Binary-coded decimal
	uint8_t hundreds = regsVx[(instruction & 0x0F00) >> 8] / 100;
	uint8_t tens = (regsVx[(instruction & 0x0F00) >> 8] - (hundreds * 100)) / 10;
	uint8_t ones = regsVx[(instruction & 0x0F00) >> 8] - ((hundreds * 100) + (tens * 10));
	memory.writeAtPos(regI, hundreds);
	memory.writeAtPos(regI + 1, tens);
	memory.writeAtPos(regI + 2, ones);

	if (enableExplanations) addNewExplanation("Store BCD representation of V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" in memory locations I, I+1 and I+2"));
};

void chip8::storeRegsToMemoryHandler(uint16_t instruction) {
	for (uint16_t i = 0; i <= ((instruction & 0x0F00) >> 8); ++i) {
		memory.writeAtPos(regI + i, regsVx[i]);
	}
	++regI;			// quirk - "The save and load opcodes (Fx55 and Fx65) increment the index register"

	if (enableExplanations) addNewExplanation("Store registers V0 through V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" in memory starting at location I"));
};

void chip8::loadRegsFromMemoryHandler(uint16_t instruction) {
	for (uint16_t i = 0; i <= ((instruction & 0x0F00) >> 8); ++i) {
		regsVx[i] = memory.readAtPos(regI + i);
	}
	++regI;		// quirk - see above

	if (enableExplanations) addNewExplanation("Read registers V0 through V" + string(1, char_to_hex((instruction & 0x0F00) >> 8)) + string(" from memory starting at location I"));
};


//============ Keyboard input ============//

// uses keymap to get keyboard keys corresponding to chip-8 keypad

// check if key on keypad is pressed
bool chip8::checkKeyDown(uint8_t key) const {
	for (uint8_t i = 0; i < KEYPAD_KEYS; ++i){
		if (key == i) return IsKeyDown(keymap[i]);
	}

	throw runtime_error("Checked status of an invalid key!");	// key not in keymap
}

// gets (lowest) pressed key or 0xFF if nothing is pressed
uint8_t chip8::getKeypadPressed() const {
	for (uint8_t i = 0; i < KEYPAD_KEYS; ++i) {
		if (IsKeyPressed(keymap[i])) return i;
	}

	return numeric_limits<uint8_t>::max();	// not holding any of the keypad keys
}


//============ Storing past instructions and explanations ============//

void chip8::addNewExplanation(const string& expl) {
	for (int i = 0; i < DISPLAY_LAST_COUNT - 1; ++i) {
		explanations[i] = explanations[i + 1];
	}
	explanations[DISPLAY_LAST_COUNT - 1] = expl;
}

void chip8::updateLastInstructions(uint16_t instr) {
	for (int i = 0; i < DISPLAY_LAST_COUNT - 1; ++i) {
		lastInstructions[i] = lastInstructions[i+1];
	}
	lastInstructions[DISPLAY_LAST_COUNT - 1] = instr;
}


//============ Printing RAM to console (unused) ============//

// overwrites printed data on subsequent calls (\r and flush)
void chip8::printWholeMemory() const {
	cout << "\r";
	for (uint16_t i = 0; i < MEMORY_SIZE; i++) {
		if (i % 16 == 0) {
			cout << setfill('0') << setw(3) << i << ": ";		// show address of current line
		}

		uint8_t readByte = memory.readAtPos(i);
		cout << setfill('0') << setw(2) << hex << readByte << " ";		// each byte is padded to two digits
	}
	cout << flush;
}