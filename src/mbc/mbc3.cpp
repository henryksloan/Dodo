#include "mbc/mbc3.h"

#include <chrono>
#include <ctime>

uint8_t Mbc3::readRomLo(uint16_t addr) { return rom[addr]; }

uint8_t Mbc3::readRomHi(uint16_t addr) {
  return rom[(rom_hi_bank * 0x4000) + addr];
}

uint8_t Mbc3::readRam(uint16_t addr) {
  if (!ram_rtc_enabled) return 0;
  if (ram_bank_or_rtc_reg < 0x04) {
    return ram[(ram_bank_or_rtc_reg * 0x2000) + addr];
  } else if (ram_bank_or_rtc_reg >= 0x08 && ram_bank_or_rtc_reg <= 0x0C) {
    return rtc[ram_bank_or_rtc_reg - 0x08];
  }
  return 0;
}

void Mbc3::writeRomLo(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    ram_rtc_enabled = (data & 0xF) == 0xA;
  } else {
    rom_hi_bank = (data == 0) ? 1 : (data & 0x7F);
  }
}

void Mbc3::writeRomHi(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    ram_bank_or_rtc_reg = data;
  } else {
    bool old_rtc_latch = rtc_latch;
    rtc_latch = data & 1;
    if (!old_rtc_latch && rtc_latch && ((rtc[4] >> 6) & 1) == 0) {
      const auto now = std::chrono::system_clock::now();
      const auto epoch = now.time_since_epoch();
      const auto chron_seconds =
          std::chrono::duration_cast<std::chrono::seconds>(epoch);
      uint64_t seconds =
          static_cast<uint64_t>(chron_seconds.count()) - rtc_base;

      rtc[0] = (seconds % 60);
      rtc[1] = ((seconds / 60) % 60);
      rtc[2] = ((seconds / 3600) % 24);
      auto days = seconds / (3600 * 24);
      rtc[3] = static_cast<uint8_t>(days);
      rtc[4] = (rtc[4] & 0xFE) | (((days >> 8) & 0x01));
      if (days >= 512) {
        rtc[4] |= 0x80;
        computeRtcBase();
      }
    }
  }
}

void Mbc3::writeRam(uint16_t addr, uint8_t data) {
  if (!ram_rtc_enabled) return;
  if (ram_bank_or_rtc_reg < 0x04) {
    ram[(ram_bank_or_rtc_reg * 0x2000) + addr] = data;
  } else if (ram_bank_or_rtc_reg >= 0x08) {
    rtc[data - 0x08] = data;
    computeRtcBase();
  }
}

void Mbc3::computeRtcBase() {
  const auto now = std::chrono::system_clock::now();
  const auto epoch = now.time_since_epoch();
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);
  uint64_t diff = static_cast<uint64_t>(seconds.count());

  diff -= rtc[0];
  diff -= rtc[1] * 60;
  diff -= rtc[2] * 3600;
  uint16_t days = static_cast<uint16_t>((rtc[4] & 0x1) << 8) | (rtc[3]);
  diff -= days * 3600 * 24;
  rtc_base = diff;
}
