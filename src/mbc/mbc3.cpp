#include "mbc/mbc3.h"

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
    rom_hi_bank = (data == 0) ? 1 : (data & 0x1F);
  }
}

void Mbc3::writeRomHi(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    ram_bank_or_rtc_reg = data;
  } else {
    bool old_rtc_latch = rtc_latch;
    rtc_latch = data & 1;
    if (!old_rtc_latch && rtc_latch) {
      // TODO: Populate RTC
    }
  }
}

void Mbc3::writeRam(uint16_t addr, uint8_t data) {
  if (!ram_rtc_enabled) return;
  if (ram_bank_or_rtc_reg < 0x04) {
    ram[(ram_bank_or_rtc_reg * 0x2000) + addr] = data;
  } else if (ram_bank_or_rtc_reg >= 0x08 && ram_bank_or_rtc_reg == 0x0C) {
    rtc[4] &= ~(1 << 6);
    rtc[4] |= data & (1 << 6);
  }
}
