#ifndef DODO_BUS_H_
#define DODO_BUS_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

const size_t kWramSize = 0x8000;
const size_t kHramSize = 0x7E;
const size_t kVramSize = 0x4000;
const size_t kOamSize = 0x4000;

class Bus {
 public:
  Bus() : wram(), hram(), vram(), oam() {}

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t data);

  uint16_t read16(uint16_t addr) {
    uint8_t lo = read(addr);
    return (read(addr + 1) << 8) | lo;
  }
  void write16(uint16_t addr, uint16_t data) {
    write(addr, data & 0xFF);
    write(addr + 1, data >> 8);
  }

 private:
  std::array<uint8_t, kWramSize> wram;
  std::array<uint8_t, kHramSize> hram;
  std::shared_ptr<std::array<uint8_t, kVramSize>> vram;
  std::shared_ptr<std::array<uint8_t, kOamSize>> oam;
};

#endif  // DODO_BUS_H_
