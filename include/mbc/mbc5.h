#ifndef DODO_MBC5_H_
#define DODO_MBC5_H_

#include <cstdint>
#include <vector>

#include "mbc.h"

class Mbc5 : public Mbc {
 public:
  Mbc5(const std::vector<uint8_t> data, const size_t ram_size)
      : rom(data),
        ram(ram_size),
        ram_enabled(),
        rom_bank_lo(1),
        ram_bank(0),
        rom_bank_hi() {}

 private:
  std::vector<uint8_t> rom, ram;

  bool ram_enabled;
  uint8_t rom_bank_lo, ram_bank;
  bool rom_bank_hi;

  virtual uint8_t readRomLo(uint16_t addr);
  virtual uint8_t readRomHi(uint16_t addr);
  virtual uint8_t readRam(uint16_t addr);

  virtual void writeRomLo(uint16_t addr, uint8_t data);
  virtual void writeRomHi(uint16_t addr, uint8_t data);
  virtual void writeRam(uint16_t addr, uint8_t data);
};

#endif  // DODO_MBC5_H_
