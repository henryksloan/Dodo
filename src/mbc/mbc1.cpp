#include "mbc/mbc1.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>

uint8_t Mbc1::readRomLo(uint16_t addr) {
  // TODO: It should behave like this on "Large ROM" cartridges:
  // size_t bank = bank_mode ? ram_bank_or_rom_bank_hi << 5 : 0;
  // return rom[(bank * 0x4000) + addr];
  return rom[addr];
}

uint8_t Mbc1::readRomHi(uint16_t addr) {
  size_t bank = static_cast<size_t>(ram_bank_or_rom_bank_hi << 5) | rom_bank_lo;
  return rom[((bank * 0x4000) + addr) % rom.size()];
}

uint8_t Mbc1::readRam(uint16_t addr) {
  if (!ram_enabled) return 0xFF;
  size_t bank = bank_mode ? ram_bank_or_rom_bank_hi : 0;
  return ram[((bank * 0x2000) + addr) % ram.size()];
}

void Mbc1::writeRomLo(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    ram_enabled = (data & 0xF) == 0xA;
  } else {
    rom_bank_lo = data & 0x1F;
    if (rom_bank_lo == 0) rom_bank_lo |= 1;
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
  ram[((bank * 0x2000) + addr) % ram.size()] = data;
}

void Mbc1::restoreSaveFile() {
  if (!savefile_opt) return;

  std::ifstream file_in;
  file_in.open(*savefile_opt, std::fstream::in | std::fstream::binary);

  if (file_in) {
    file_in.seekg(0, std::ios::end);
    std::streampos filesize = file_in.tellg();
    file_in.seekg(0);

    std::copy_n(std::istreambuf_iterator<char>(file_in),
                std::min(static_cast<size_t>(filesize), ram.capacity()),
                std::back_inserter(ram));
  }

  ram.resize(ram.capacity(), 0);
}

void Mbc1::writeSaveFile() {
  if (!savefile_opt) return;

  std::ofstream out;
  out.open(*savefile_opt, std::fstream::out | std::fstream::binary);

  std::ostreambuf_iterator<char> out_it(out);
  std::copy(ram.begin(), ram.end(), out_it);

  out.close();
}
