#ifndef DODO_MBC3_H_
#define DODO_MBC3_H_

#include <cstdint>
#include <optional>
#include <vector>

#include "mbc.h"

class Mbc3 : public Mbc {
 public:
  Mbc3(const std::string_view filename, const uint8_t type,
       const std::vector<uint8_t> data, const size_t ram_size)
      : rom(data),
        ram_rtc_enabled(),
        rtc_latch(),
        rom_hi_bank(1),
        ram_bank_or_rtc_reg(0),
        rtc{},
        rtc_base(0) {
    if (type == 0x0F || type == 0x10 || type == 0x13) {
      ram.reserve(ram_size);
      savefile_opt = this->saveFileName(filename);
      const bool restore_ram = (type == 0x10 || type == 0x13);
      const bool try_restore_rtc = (type == 0x0F || type == 0x10);
      restoreSaveFile(restore_ram, try_restore_rtc);
    } else {
      savefile_opt = std::nullopt;
      ram.resize(ram_size, 0);
    }
  }

  ~Mbc3() { writeSaveFile(); }

 private:
  std::vector<uint8_t> rom, ram;

  bool ram_rtc_enabled, rtc_latch;
  uint8_t rom_hi_bank;
  uint8_t ram_bank_or_rtc_reg;

  uint8_t rtc[5];
  uint64_t rtc_base;

  std::optional<std::string> savefile_opt;

  virtual uint8_t readRomLo(uint16_t addr);
  virtual uint8_t readRomHi(uint16_t addr);
  virtual uint8_t readRam(uint16_t addr);

  virtual void writeRomLo(uint16_t addr, uint8_t data);
  virtual void writeRomHi(uint16_t addr, uint8_t data);
  virtual void writeRam(uint16_t addr, uint8_t data);

  void computeRtcBase();

  void restoreSaveFile(const bool restore_ram, const bool try_restore_rtc);
  void writeSaveFile();
};

#endif  // DODO_MBC3_H_
