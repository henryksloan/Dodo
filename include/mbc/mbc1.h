#ifndef DODO_MBC1_H_
#define DODO_MBC1_H_

#include <cstdint>
#include <vector>

#include "mbc.h"

class Mbc1 : public Mbc {
 public:
  Mbc1(const std::vector<uint8_t> data, const size_t ram_size)
      : rom(data),
        ram(ram_size),
        ram_enabled(),
        rom_bank_lo(1),
        ram_bank_or_rom_bank_hi(0),
        bank_mode() {}

 private:
  std::vector<uint8_t> rom, ram;

  bool ram_enabled;
  uint8_t rom_bank_lo, ram_bank_or_rom_bank_hi;
  bool bank_mode;

  virtual uint8_t readRomLo(uint16_t addr);
  virtual uint8_t readRomHi(uint16_t addr);
  virtual uint8_t readRam(uint16_t addr);

  virtual void writeRomLo(uint16_t addr, uint8_t data);
  virtual void writeRomHi(uint16_t addr, uint8_t data);
  virtual void writeRam(uint16_t addr, uint8_t data);
};

#endif  // DODO_MBC1_H_
