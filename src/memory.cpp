
#include "memory.hpp"
#include <stdexcept>

using namespace std;

ch8Memory::ch8Memory() {
	memory.fill(0);		// initialize to zeros
}

// write one byte to memory
void ch8Memory::writeAtPos(uint16_t pos, uint8_t val) {
	if (pos > MEMORY_SIZE - 1 || pos < 0) throw runtime_error("Trying to write outside of memory space!");

	memory[pos] = val;
}

// read one byte from memory
uint8_t ch8Memory::readAtPos(uint16_t pos) const {
	if (pos > MEMORY_SIZE - 1 || pos < 0) throw runtime_error("Trying to read outside of memory space!");
	return memory[pos];
}

// read two bytes from memory (one instruction)
uint16_t ch8Memory::readInstuctionAtPos(uint16_t pos) const {
	if (pos > MEMORY_SIZE - 2 || pos < 0) throw runtime_error("Trying to read instruction outside of memory space!");

	uint16_t instruction = (memory[pos] << 8) | memory[pos + 1];
	return instruction;
}