#include "mbc/mbc5.h"

#include <fstream>

uint8_t Mbc5::readRomLo(uint16_t addr) { return rom[addr]; }

uint8_t Mbc5::readRomHi(uint16_t addr) {
  size_t bank = static_cast<size_t>(rom_bank_hi << 8) | rom_bank_lo;
  return rom[((bank * 0x4000) + addr) % rom.size()];
}

uint8_t Mbc5::readRam(uint16_t addr) {
  if (!ram_enabled) return 0xFF;
  return ram[((static_cast<size_t>(ram_bank) * 0x2000) + addr) % ram.size()];
}

void Mbc5::writeRomLo(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    ram_enabled = (data & 0xF) == 0xA;
  } else if (addr < 0x3000) {
    rom_bank_lo = data;
  } else {
    rom_bank_hi = data & 1;
  }
}

void Mbc5::writeRomHi(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    ram_bank = data & 0xF;
  }
}

void Mbc5::writeRam(uint16_t addr, uint8_t data) {
  if (!ram_enabled) return;
  ram[((static_cast<size_t>(ram_bank) * 0x2000) + addr) % ram.size()] = data;
}

void Mbc5::restoreSaveFile() {
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

void Mbc5::writeSaveFile() {
  if (!savefile_opt) return;

  std::ofstream out;
  out.open(*savefile_opt, std::fstream::out | std::fstream::binary);

  std::ostreambuf_iterator<char> out_it(out);
  std::copy(ram.begin(), ram.end(), out_it);

  out.close();
}
