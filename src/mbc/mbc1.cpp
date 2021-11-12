#include "mbc/mbc1.h"

uint8_t Mbc1::readRomLo(uint16_t addr) {
  // TODO: It should behave like this on "Large ROM" cartridges:
  // size_t bank = bank_mode ? ram_bank_or_rom_bank_hi << 5 : 0;
  // return rom[(bank * 0x4000) + addr];
  return rom[addr];
}

uint8_t Mbc1::readRomHi(uint16_t addr) {
  size_t bank = (ram_bank_or_rom_bank_hi << 5) | rom_bank_lo;
  return rom[(bank * 0x4000) + addr];
}

uint8_t Mbc1::readRam(uint16_t addr) {
  if (!ram_enabled) return 0;
  size_t bank = bank_mode ? ram_bank_or_rom_bank_hi : 0;
  return ram[(bank * 0x2000) + addr];
}

void Mbc1::writeRomLo(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    ram_enabled = (data & 0xF) == 0xA;
  } else {
    rom_bank_lo = data & 0x1F;
  }
}

void Mbc1::writeRomHi(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    ram_bank_or_rom_bank_hi = data & 0b11;
  } else {
    bank_mode = data & 1;
  }
}

void Mbc1::writeRam(uint16_t addr, uint8_t data) {
  if (!ram_enabled) return;
  size_t bank = bank_mode ? ram_bank_or_rom_bank_hi : 0;
  ram[(bank * 0x2000) + addr] = data;
}
