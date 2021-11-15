#include "bus.h"

#include <iomanip>
#include <iostream>
#include <utility>

void Bus::tick(int cpu_tcycles) {
  int cpu_multiplier = double_speed ? 2 : 1;
  int dma_ticks = progressDma();
  int ppu_ticks = cpu_tcycles / cpu_multiplier + dma_ticks;
  int cpu_ticks = cpu_tcycles + dma_ticks * cpu_multiplier;

  bool timer_interrupt = timer.tick(cpu_ticks);
  int_request |= timer_interrupt << kIntOffTimer;

  auto ppu_interrupts = ppu.tick(ppu_ticks);
  int_request |= ppu_interrupts;

  // TODO: Tick other devices
}

void Bus::reset(bool cgb_mode) {
  this->cgb_mode = cgb_mode;
  this->ppu.setCgbMode(cgb_mode);

  // TODO: Reset devices
  // TODO: https://gbdev.io/pandocs/Power_Up_Sequence.html
  ioWrite(0xFF40, 0x91);
  ioWrite(0xFF41, 0x81);
  ioWrite(0xFF05, 0);
  ioWrite(0xFF06, 0);
  ioWrite(0xFF07, 0);
  ioWrite(0xFF10, 0x80);
  ioWrite(0xFF11, 0xBF);
  ioWrite(0xFF12, 0xF3);
  ioWrite(0xFF14, 0xBF);
  ioWrite(0xFF16, 0x3F);
  ioWrite(0xFF16, 0x3F);
  ioWrite(0xFF17, 0);
  ioWrite(0xFF19, 0xBF);
  ioWrite(0xFF1A, 0x7F);
  ioWrite(0xFF1B, 0xFF);
  ioWrite(0xFF1C, 0x9F);
  ioWrite(0xFF1E, 0xFF);
  ioWrite(0xFF20, 0xFF);
  ioWrite(0xFF21, 0);
  ioWrite(0xFF22, 0);
  ioWrite(0xFF23, 0xBF);
  ioWrite(0xFF24, 0x77);
  ioWrite(0xFF25, 0xF3);
  ioWrite(0xFF26, 0xF1);
  ioWrite(0xFF40, 0x91);
  ioWrite(0xFF42, 0);
  ioWrite(0xFF43, 0);
  ioWrite(0xFF45, 0);
  ioWrite(0xFF47, 0xFC);
  ioWrite(0xFF48, 0xFF);
  ioWrite(0xFF49, 0xFF);
  ioWrite(0xFF4A, 0);
  ioWrite(0xFF4B, 0);
}

uint8_t Bus::read(uint16_t addr) {
  if ((addr < 0x8000) || (addr >= 0xA000 && addr < 0xC000)) {
    return mbc ? mbc->read(addr) : 0;
  } else if (addr >= 0x8000 && addr < 0xA000) {
    return ppu.readVram(addr);
  } else if (addr >= 0xC000 && addr < 0xD000) {
    return wram[addr - 0xC000];
  } else if (addr >= 0xD000 && addr < 0xE000) {
    size_t bank = 0x1000 * (cgb_mode ? wram_bank : 0);
    return wram[bank + (addr - 0xC000)];
  } else if (addr >= 0xE000 && addr < 0xFE00) {
    // TODO: Echo RAM
  } else if (addr >= 0xFE00 && addr < 0xFEA0) {
    ppu.readOam(addr);
  } else if (addr >= 0xFF00 && addr < 0xFF80) {
    return ioRead(addr);
  } else if (addr >= 0xFF80 && addr < 0xFFFF) {
    return hram[addr - 0xFF80];
  } else if (addr == 0xFFFF) {
    return int_enable;
  }

  return 0;
}

void Bus::write(uint16_t addr, uint8_t data) {
  if ((addr < 0x4000) || (addr >= 0xA000 && addr < 0xC000)) {
    if (mbc) mbc->write(addr, data);
  } else if (addr >= 0x8000 && addr < 0xA000) {
    ppu.writeVram(addr, data);
  } else if (addr >= 0xC000 && addr < 0xD000) {
    wram[addr - 0xC000] = data;
  } else if (addr >= 0xD000 && addr < 0xE000) {
    size_t bank = 0x1000 * (cgb_mode ? wram_bank : 0);
    wram[bank + (addr - 0xC000)] = data;
  } else if (addr >= 0xE000 && addr < 0xFE00) {
    // TODO: Echo RAM
  } else if (addr >= 0xFE00 && addr < 0xFEA0) {
    ppu.writeOam(addr, data);
  } else if (addr >= 0xFF00 && addr < 0xFF80) {
    ioWrite(addr, data);
  } else if (addr >= 0xFF80 && addr < 0xFFFF) {
    hram[addr - 0xFF80] = data;
  } else if (addr == 0xFFFF) {
    int_enable = data;
  }
}

uint8_t Bus::ioRead(uint16_t addr) {
  // TODO: Limit some to CGB mode
  // TODO: FF56 - Infrared
  // TODO: FF6C - Object priority
  if (addr == 0xFF00) {
    // 0 means selected, and 0 means pressed
    // Don't ask me why
    uint8_t masked_actions = select_action_buttons ? 0 : action_buttons_pressed;
    uint8_t masked_dirs = select_dir_buttons ? 0 : dir_buttons_pressed;
    return (select_action_buttons << 5) | (select_dir_buttons << 4) |
           masked_actions | masked_dirs;
  } else if (addr == 0xFF01 || addr == 0xFF02) {
    // TODO: Communication
    return serial_temp[addr - 0xFF01];
  } else if (addr >= 0xFF04 && addr <= 0xFF07) {
    return timer.read(addr);
  } else if (addr == 0xFF0F) {
    return int_request;
  } else if (addr >= 0xFF10 && addr <= 0xFF26) {
    // TODO: Sound
  } else if (addr >= 0xFF30 && addr <= 0xFF3F) {
    // TODO :Waveform RAM
  } else if (addr >= 0xFF40 && addr <= 0xFF4B) {
    return ppu.read(addr);
  } else if (addr == 0xFF4D) {
    return (double_speed << 7) | prepare_speed_switch;
  } else if (addr == 0xFF4F) {
    return ppu.getVramBank();
  } else if (addr == 0xFF50) {
    // TODO: Set to non-zero to disable boot ROM
  } else if (addr >= 0xFF51 && addr <= 0xFF54) {
    return hdma_src_dst[addr - 0xFF51];
  } else if (addr == 0xFF55) {
    return ((hdma_mode != HdmaMode::kHdmaNone) << 7) | hdma_len;
  } else if (addr >= 0xFF68 && addr <= 0xFF6B) {
    return ppu.read(addr);
  } else if (addr == 0xFF70) {
    return wram_bank;
  }

  return 0;
}

void Bus::ioWrite(uint16_t addr, uint8_t data) {
  // TODO: Limit some to CGB mode
  if (addr == 0xFF00) {
    select_action_buttons = (data >> 5) & 1;
    select_dir_buttons = (data >> 4) & 1;
  } else if (addr == 0xFF01 || addr == 0xFF02) {
    // TODO: Communication
    serial_temp[addr - 0xFF01] = data;
  } else if (addr >= 0xFF04 && addr <= 0xFF07) {
    timer.write(addr, data);
  } else if (addr == 0xFF0F) {
    int_request = data;
  } else if (addr >= 0xFF10 && addr <= 0xFF26) {
    // TODO: Sound
  } else if (addr >= 0xFF30 && addr <= 0xFF3F) {
    // TODO :Waveform RAM
  } else if (addr == 0xFF46) {
    oamdma((uint16_t)data * 0x100);
  } else if (addr >= 0xFF40 && addr <= 0xFF4B) {
    ppu.write(addr, data);
  } else if (addr == 0xFF4D) {
    prepare_speed_switch = data & 1;
  } else if (addr == 0xFF4F) {
    ppu.setVramBank(data & 1);
  } else if (addr == 0xFF50) {
    // TODO: Set to non-zero to disable boot ROM
  } else if (addr >= 0xFF51 && addr <= 0xFF54) {
    // Bits 4-16 of src are respected, and bits 4-12 of dst
    static const uint8_t mask[4] = {0xFF, 0xF0, 0x1F, 0xF0};
    hdma_src_dst[addr - 0xFF51] = data & mask[addr - 0xFF51];
  } else if (addr == 0xFF55) {
    hdma_len = data & ~(1 << 7);
    hdma_src = (((uint16_t)hdma_src_dst[0]) << 8) | hdma_src_dst[1];
    hdma_dst = 0x8000 | (((uint16_t)hdma_src_dst[2]) << 8) | hdma_src_dst[3];
    // TODO: Validate source

    bool bit_7 = data >> 7;
    if (hdma_mode != HdmaMode::kHdmaNone && !bit_7) {
      // Writing zero to bit 7 can cancel a transfer
      hdma_mode = HdmaMode::kHdmaNone;
    } else {
      hdma_mode = bit_7 ? HdmaMode::kHdmaHBlank : HdmaMode::kHdmaGeneral;
    }
  } else if (addr >= 0xFF68 && addr <= 0xFF6B) {
    ppu.write(addr, data);
  } else if (addr == 0xFF70) {
    wram_bank = data & 0b111;
  }
}

int Bus::progressDma() {
  switch (hdma_mode) {
    case HdmaMode::kHdmaGeneral:
      return hdmaTransferLines(hdma_len + 1);
    case HdmaMode::kHdmaHBlank:
      return ppu.inHblank() ? hdmaTransferLines() : 0;
    case HdmaMode::kHdmaNone:
      return 0;
  }
  return 0;
}

int Bus::hdmaTransferLines(int n_lines /* = 1 */) {
  for (int i = 0; i < n_lines; i++) {
    for (int j = 0; j < 0x10; j++) {
      ppu.writeVram(hdma_dst, this->read(hdma_src));
    }
    hdma_src += 0x10;
    hdma_dst += 0x10;

    if (hdma_len == 0) {
      hdma_len = 0x7F;
      hdma_mode = HdmaMode::kHdmaNone;
    } else {
      hdma_len--;
    }
  }
  return 8 * n_lines;
}

void Bus::switchSpeed() {
  if (prepare_speed_switch) {
    double_speed = !double_speed;
    prepare_speed_switch = false;
  }
}

void Bus::oamdma(uint16_t addr) {
  for (int i = 0; i < 0x9F; i++) {
    ppu.writeOam(0xFE00 + i, this->read(addr + i));
  }
}
