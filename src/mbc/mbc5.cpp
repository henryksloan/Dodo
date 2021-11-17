#include "mbc/mbc5.h"

#include <iostream>

uint8_t Mbc5::readRomLo(uint16_t addr) { return rom[addr]; }

uint8_t Mbc5::readRomHi(uint16_t addr) {
  uint16_t bank = static_cast<uint16_t>(rom_bank_hi << 8) | rom_bank_lo;
  return rom[(bank * 0x4000) + addr];
}

uint8_t Mbc5::readRam(uint16_t addr) {
  if (!ram_enabled) return 0;
  return ram[(ram_bank * 0x2000) + addr];
}

void Mbc5::writeRomLo(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    ram_enabled = (data & 0xF) == 0xA;
  } else {
    rom_bank_lo = data;
  }
}

void Mbc5::writeRomHi(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    rom_bank_hi = data & 1;
  } else {
    ram_bank = data;
  }
}

void Mbc5::writeRam(uint16_t addr, uint8_t data) {
  if (!ram_enabled) return;
  ram[(ram_bank * 0x2000) + addr] = data;
}
