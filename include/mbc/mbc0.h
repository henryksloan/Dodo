#ifndef DODO_MBC0_H_
#define DODO_MBC0_H_

#include <array>
#include <cstdint>

// An abstract "Memory Bus Controller" - dispatches accesses to cartridge memory
class Mbc0 {
 private:
  std::array<uint8_t, 0x8000> rom;
  std::array<uint8_t, 0x2000> ram;

  virtual uint8_t readRomLo(uint16_t addr) { return rom[addr]; };
  virtual uint8_t readRomHi(uint16_t addr) { return rom[addr + 0x4000]; };
  virtual uint8_t readRam(uint16_t addr) { return ram[addr]; };

  virtual void writeRomLo(uint16_t addr, uint8_t data){};
  virtual void writeRomHi(uint16_t addr, uint8_t data){};
  virtual void writeRam(uint16_t addr, uint8_t data) { ram[addr] = data; };
};

#endif  // DODO_MBC0_H_
