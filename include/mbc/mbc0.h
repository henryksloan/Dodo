#ifndef DODO_MBC0_H_
#define DODO_MBC0_H_

#include <cstdint>
#include <vector>

#include "mbc.h"

class Mbc0 : public Mbc {
 public:
  Mbc0(const std::vector<uint8_t> data, const size_t ram_size)
      : rom(data), ram(ram_size) {}

 private:
  std::vector<uint8_t> rom, ram;

  virtual uint8_t readRomLo(uint16_t addr) { return rom[addr]; };
  virtual uint8_t readRomHi(uint16_t addr) { return rom[addr + 0x4000]; };
  virtual uint8_t readRam(uint16_t addr) { return ram[addr]; };

  virtual void writeRomLo(uint16_t addr, uint8_t data){};
  virtual void writeRomHi(uint16_t addr, uint8_t data){};
  virtual void writeRam(uint16_t addr, uint8_t data) { ram[addr] = data; };
};

#endif  // DODO_MBC0_H_
