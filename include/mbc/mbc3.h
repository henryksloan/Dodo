#ifndef DODO_MBC3_H_
#define DODO_MBC3_H_

#include <cstdint>
#include <vector>

#include "mbc.h"

class Mbc3 : public Mbc {
 public:
  Mbc3(const std::vector<uint8_t> data, const size_t ram_size)
      : rom(data),
        ram(ram_size),
        ram_rtc_enabled(),
        rtc_latch(),
        rom_hi_bank(1),
        ram_bank_or_rtc_reg(0),
        rtc{},
        rtc_base(0) {}

 private:
  std::vector<uint8_t> rom, ram;

  bool ram_rtc_enabled, rtc_latch;
  uint8_t rom_hi_bank;
  uint8_t ram_bank_or_rtc_reg;

  uint8_t rtc[5];
  uint64_t rtc_base;

  virtual uint8_t readRomLo(uint16_t addr);
  virtual uint8_t readRomHi(uint16_t addr);
  virtual uint8_t readRam(uint16_t addr);

  virtual void writeRomLo(uint16_t addr, uint8_t data);
  virtual void writeRomHi(uint16_t addr, uint8_t data);
  virtual void writeRam(uint16_t addr, uint8_t data);

  void computeRtcBase();
};

#endif  // DODO_MBC3_H_
