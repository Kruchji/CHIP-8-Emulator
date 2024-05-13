#pragma once

#include "memory.hpp"
#include "display.hpp"
#include "fontset.hpp"
#include "keymap.hpp"

#include <string>
#include <random>
#include <cstdint>
#include <array>
#include <vector>
#include <unordered_map>
#include <functional>

constexpr uint16_t FONTSET_START_ADDRESS = 0x000;	// might need to be 0x050, depending on game
constexpr uint16_t PC_START_ADDRESS = 0x200;		// 0x200 (512) - Start of most Chip-8 programs
constexpr uint16_t INSTRUCTION_BYTES = 2;			// size of Chip-8 instruction

class chip8 {
private:
	// RAM and display
	ch8Memory memory;
	ch8Display display;

	// registers
	uint16_t regPC = 0;							// program counter register (16-bit)
	uint16_t regI = 0;							// index register (16-bit) - used to store memory addresses
	std::array<uint8_t, VREGS_COUNT> regsVx;	// general Vx registers (8-bit), special case is VF -> used as a flag register

	// delay and sound timer registers (8-bit) - decrease every frame (60 times/second usually)
	uint8_t regDT = 0;
	uint8_t regST = 0;		// plays buzzer while non-zero

	// stack
	uint8_t regSP = 0;							// stack pointer (8-bit)
	std::array<uint16_t, STACK_SIZE> stack;		// stack - stores return address for subroutines

	// random number (byte) generator
	std::random_device rd;
	std::mt19937 generator;
	std::uniform_int_distribution<> distChar;

	// default per frame (cycle) when not paused
	int instructionsPerCycle;

	// store past instructions and explanations
	bool enableExplanations;
	std::vector<std::string> explanations;
	std::array<uint16_t, DISPLAY_LAST_COUNT> lastInstructions;
	void addNewExplanation(const std::string& expl);
	void updateLastInstructions(uint16_t instr);
	
	// loads fontset into RAM - called at the start
	void loadFontset();

	// called each frame
	void emulateOneFrame(int IPC);
	void checkForPauseInput();			// lets user pause the game and advance one instruction at a time

	// all valid opcodes
	enum class Opcode : uint16_t {
		CLEAR = 0x00E0,
		RETURN = 0x00EE,
		JUMP = 0x1000,
		CALL = 0x2000,
		SKIP_IF_EQUAL = 0x3000,
		SKIP_IF_NOT_EQUAL = 0x4000,
		SKIP_IF_REGS_EQUAL = 0x5000,
		LOAD_IMMEDIATE = 0x6000,
		ADD_IMMEDIATE = 0x7000,
		LOAD = 0x8000,
		OR = 0x8001,
		AND = 0x8002,
		XOR = 0x8003,
		ADD = 0x8004,
		SUBTRACT = 0x8005,
		SHIFT_RIGHT = 0x8006,
		SUBTRACT_NEGATIVE = 0x8007,
		SHIFT_LEFT = 0x800E,
		SKIP_IF_REGS_NOT_EQUAL = 0x9000,
		LOAD_ADDRESS = 0xA000,
		JUMP_PLUS_V0 = 0xB000,
		RANDOM = 0xC000,
		DRAW = 0xD000,
		SKIP_IF_KEY = 0xE09E,
		SKIP_IF_NOT_KEY = 0xE0A1,
		LOAD_DELAY = 0xF007,
		LOAD_KEY = 0xF00A,
		SET_DELAY = 0xF015,
		SET_SOUND = 0xF018,
		ADD_TO_I = 0xF01E,
		LOAD_DIGIT = 0xF029,
		STORE_BCD = 0xF033,				// BCD = Binary-coded decimal
		STORE_REGS_TO_MEMORY = 0xF055,
		LOAD_REGS_FROM_MEMORY = 0xF065,

	};

	// opcode handler methods for executing one instruction
	void clearHandler();
	void returnHandler();
	void jumpHandler(uint16_t instruction);
	void callHandler(uint16_t instruction);
	void skipIfEqualHandler(uint16_t instruction);
	void skipIfNotEqualHandler(uint16_t instruction);
	void skipIfRegsEqualHandler(uint16_t instruction);
	void loadImmediateHandler(uint16_t instruction);
	void addImmediateHandler(uint16_t instruction);
	void loadHandler(uint16_t instruction);
	void orHandler(uint16_t instruction);
	void andHandler(uint16_t instruction);
	void xorHandler(uint16_t instruction);
	void addHandler(uint16_t instruction);
	void subtractHandler(uint16_t instruction);
	void shiftRightHandler(uint16_t instruction);
	void subtractNegativeHandler(uint16_t instruction);
	void shiftLeftHandler(uint16_t instruction);
	void skipIfRegsNotEqualHandler(uint16_t instruction);
	void loadAddressHandler(uint16_t instruction);
	void jumpPlusV0Handler(uint16_t instruction);
	void randomHandler(uint16_t instruction);
	void drawHandler(uint16_t instruction);
	void skipIfKeyHandler(uint16_t instruction);
	void skipIfNotKeyHandler(uint16_t instruction);
	void loadDelayHandler(uint16_t instruction);
	void loadKeyHandler(uint16_t instruction);
	void setDelayHandler(uint16_t instruction);
	void setSoundHandler(uint16_t instruction);
	void addToIHandler(uint16_t instruction);
	void loadDigitHandler(uint16_t instruction);
	void storeBCDHandler(uint16_t instruction);
	void storeRegsToMemoryHandler(uint16_t instruction);
	void loadRegsFromMemoryHandler(uint16_t instruction);

	// methods and maps for decoding (correct masking) one instruction and calling its handler
	void executeInstruction(uint16_t instr);						// mask: 0xF000
	void executeMatchFullInstruction(uint16_t instruction);			// mask: 0xFFFF
	void executeMatchLastOneInstruction(uint16_t instruction);		// mask: 0xF00F
	void executeMatchLastTwoInstruction(uint16_t instruction);		// mask: 0xF0FF
	const std::unordered_map<uint16_t, std::function<void(uint16_t)>> opcodeHandlers = {		// mask: 0xF000
		// direct handler calls
		{static_cast<uint16_t>(Opcode::JUMP), [this](uint16_t instruction) { this->jumpHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::CALL), [this](uint16_t instruction) { this->callHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::SKIP_IF_EQUAL), [this](uint16_t instruction) { this->skipIfEqualHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::SKIP_IF_NOT_EQUAL), [this](uint16_t instruction) { this->skipIfNotEqualHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::LOAD_IMMEDIATE), [this](uint16_t instruction) { this->loadImmediateHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::ADD_IMMEDIATE), [this](uint16_t instruction) { this->addImmediateHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::LOAD_ADDRESS), [this](uint16_t instruction) { this->loadAddressHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::JUMP_PLUS_V0), [this](uint16_t instruction) { this->jumpPlusV0Handler(instruction); }},
		{static_cast<uint16_t>(Opcode::RANDOM), [this](uint16_t instruction) { this->randomHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::DRAW), [this](uint16_t instruction) { this->drawHandler(instruction); }},

		// method calls that mask less of the instruction
		{0x0000, [this](uint16_t instruction) { this->executeMatchFullInstruction(instruction); }},
		{0x5000, [this](uint16_t instruction) { this->executeMatchLastOneInstruction(instruction); }},
		{0x8000, [this](uint16_t instruction) { this->executeMatchLastOneInstruction(instruction); }},
		{0x9000, [this](uint16_t instruction) { this->executeMatchLastOneInstruction(instruction); }},
		{0xE000, [this](uint16_t instruction) { this->executeMatchLastTwoInstruction(instruction); }},
		{0xF000, [this](uint16_t instruction) { this->executeMatchLastTwoInstruction(instruction); }},
	};
	const std::unordered_map<uint16_t, std::function<void()>> opcodeMatchFullHandlers = {		// mask: 0xFFFF
		// 0x0000
		{static_cast<uint16_t>(Opcode::CLEAR), [this]() { this->clearHandler(); }},
		{static_cast<uint16_t>(Opcode::RETURN), [this]() { this->returnHandler(); }},
	};
	const std::unordered_map<uint16_t, std::function<void(uint16_t)>> opcodeMatchLastOneHandlers = {		// mask: 0xF00F
		// 0x5000
		{static_cast<uint16_t>(Opcode::SKIP_IF_REGS_EQUAL), [this](uint16_t instruction) { this->skipIfRegsEqualHandler(instruction); }},

		// 0x8000
		{static_cast<uint16_t>(Opcode::LOAD), [this](uint16_t instruction) { this->loadHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::OR), [this](uint16_t instruction) { this->orHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::AND), [this](uint16_t instruction) { this->andHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::XOR), [this](uint16_t instruction) { this->xorHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::ADD), [this](uint16_t instruction) { this->addHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::SUBTRACT), [this](uint16_t instruction) { this->subtractHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::SHIFT_RIGHT), [this](uint16_t instruction) { this->shiftRightHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::SUBTRACT_NEGATIVE), [this](uint16_t instruction) { this->subtractNegativeHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::SHIFT_LEFT), [this](uint16_t instruction) { this->shiftLeftHandler(instruction); }},

		// 0x9000
		{static_cast<uint16_t>(Opcode::SKIP_IF_REGS_NOT_EQUAL), [this](uint16_t instruction) { this->skipIfRegsNotEqualHandler(instruction); }},
	};
	const std::unordered_map<uint16_t, std::function<void(uint16_t)>> opcodeMatchLastTwoHandlers = {		// mask: 0xF0FF
		// 0xE000
		{static_cast<uint16_t>(Opcode::SKIP_IF_KEY), [this](uint16_t instruction) { this->skipIfKeyHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::SKIP_IF_NOT_KEY), [this](uint16_t instruction) { this->skipIfNotKeyHandler(instruction); }},

		// 0xF000
		{static_cast<uint16_t>(Opcode::LOAD_DELAY), [this](uint16_t instruction) { this->loadDelayHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::LOAD_KEY), [this](uint16_t instruction) { this->loadKeyHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::SET_DELAY), [this](uint16_t instruction) { this->setDelayHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::SET_SOUND), [this](uint16_t instruction) { this->setSoundHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::ADD_TO_I), [this](uint16_t instruction) { this->addToIHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::LOAD_DIGIT), [this](uint16_t instruction) { this->loadDigitHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::STORE_BCD), [this](uint16_t instruction) { this->storeBCDHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::STORE_REGS_TO_MEMORY), [this](uint16_t instruction) { this->storeRegsToMemoryHandler(instruction); }},
		{static_cast<uint16_t>(Opcode::LOAD_REGS_FROM_MEMORY), [this](uint16_t instruction) { this->loadRegsFromMemoryHandler(instruction); }},
	};

	// keyboard input
	bool checkKeyDown(uint8_t key) const;
	uint8_t getKeypadPressed() const;

	// prints whole RAM to console - unused right now
	void printWholeMemory() const;

public:
	chip8(int scale, int speed, bool enableExplanations, unsigned int mainColor, unsigned int BGColor);
	void loadROM(const std::string& fileName);
	void run();		// begins executing instructions
};


// converting value between 0 - 15 to hex digits (used when displaying values)
constexpr std::string_view hexDigits = "0123456789ABCDEF";
inline char char_to_hex(uint8_t character) {
	if (character < hexDigits.length()) return hexDigits[character];

	return 'X';  // Invalid input
}