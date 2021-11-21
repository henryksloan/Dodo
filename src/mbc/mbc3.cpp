#include "mbc/mbc3.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iterator>

uint8_t Mbc3::readRomLo(uint16_t addr) { return rom[addr]; }

uint8_t Mbc3::readRomHi(uint16_t addr) {
  return rom[((static_cast<size_t>(rom_hi_bank) * 0x4000) + addr) % rom.size()];
}

uint8_t Mbc3::readRam(uint16_t addr) {
  if (!ram_rtc_enabled) return 0xFF;
  if (ram_bank_or_rtc_reg < 0x04) {
    return ram[(static_cast<size_t>(ram_bank_or_rtc_reg) * 0x2000) + addr];
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
    ram[(static_cast<size_t>(ram_bank_or_rtc_reg) * 0x2000) + addr] = data;
  } else if (ram_bank_or_rtc_reg >= 0x08) {
    rtc[ram_bank_or_rtc_reg - 0x08] = data;
    computeRtcBase();
  }
}

void Mbc3::computeRtcBase() {
  const auto now = std::chrono::system_clock::now();
  const auto epoch = now.time_since_epoch();
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(epoch);
  uint64_t diff = static_cast<uint64_t>(seconds.count());

  diff -= rtc[0];
  diff -= static_cast<uint64_t>(rtc[1]) * 60;
  diff -= static_cast<uint64_t>(rtc[2]) * 3600;
  uint16_t days = static_cast<uint16_t>((rtc[4] & 0x1) << 8) | (rtc[3]);
  diff -= static_cast<uint64_t>(days) * 3600 * 24;
  rtc_base = diff;
}

// http://justsolve.archiveteam.org/wiki/GB#MBC3_RTC_save_format
// TODO: Restoring RTC doesn't work, and saving RTC is wrong... but it's a start
void Mbc3::restoreSaveFile(const bool restore_ram, const bool try_restore_rtc) {
  if (!savefile_opt) return;

  std::ifstream file_in;
  file_in.open(*savefile_opt, std::fstream::in | std::fstream::binary);

  if (file_in) {
    file_in.seekg(0, std::ios::end);
    std::streampos filesize = file_in.tellg();
    file_in.seekg(0);

    if (restore_ram) {
      std::copy_n(std::istreambuf_iterator<char>(file_in),
                  std::min(static_cast<size_t>(filesize), ram.capacity()),
                  std::back_inserter(ram));
    }

    if (try_restore_rtc) {
      std::vector<uint8_t> saved_time;
      std::copy(std::istreambuf_iterator<char>(file_in),
                std::istreambuf_iterator<char>(),
                std::back_inserter(saved_time));

      if (saved_time.size() >= 37) {
        rtc_base = saved_time[0] + (60 * saved_time[4]) +
                   (3600 * saved_time[8]) + (24 * 3600 * saved_time[12]) +
                   (256 * 24 * 3600 * saved_time[16]);
        rtc[0] = saved_time[20];
        rtc[1] = saved_time[24];
        rtc[2] = saved_time[28];
        rtc[3] = saved_time[32];
        rtc[4] = saved_time[36];
      }
    }
  }

  ram.resize(ram.capacity(), 0);
}

void Mbc3::writeSaveFile() {
  if (!savefile_opt) return;

  std::ofstream out;
  out.open(*savefile_opt, std::fstream::out | std::fstream::binary);

  std::ostreambuf_iterator<char> out_it(out);
  std::copy(ram.begin(), ram.end(), out_it);

  {
    // http://justsolve.archiveteam.org/wiki/GB#MBC3_RTC_save_format
    // RTC data is stored in the first byte of each DWORD
    const auto push_byte = [&](char data) {
      (*out_it++) = data;
      for (int i = 0; i < 3; i++) (*out_it++) = 0;
    };

    push_byte(rtc_base % 60);
    push_byte((rtc_base / 60) % 60);
    push_byte((rtc_base / 3600) % 24);

    auto days = rtc_base / (3600 * 24);
    push_byte(static_cast<char>(days));
    push_byte(static_cast<char>((days >> 8) & 1));

    for (uint8_t rtc_byte : rtc) {
      push_byte(static_cast<char>(rtc_byte));
    }

    const auto now = std::chrono::system_clock::now();
    const auto epoch = now.time_since_epoch();
    const auto seconds_chron =
        std::chrono::duration_cast<std::chrono::seconds>(epoch);
    uint64_t seconds = static_cast<uint64_t>(seconds_chron.count());

    for (size_t byte_n = 0; byte_n < 8; byte_n++) {
      (*out_it++) = static_cast<char>(seconds >> (8 * byte_n));
    }
  }

  out.close();
}
