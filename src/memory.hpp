#pragma once

#include <array>
#include <cstdint>

constexpr uint16_t MEMORY_SIZE = 4096;		// Chip-8 RAM is 4kB (address 0x000 (0) to 0xFFF (4095))

class ch8Memory {
private:
	std::array<uint8_t, MEMORY_SIZE> memory;
public:
	ch8Memory();
	void writeAtPos(uint16_t pos, uint8_t val);
	uint8_t readAtPos(uint16_t pos) const;
	uint16_t readInstuctionAtPos(uint16_t pos) const;
};