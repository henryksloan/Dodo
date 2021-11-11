#include "bus.h"

void Bus::tick(int cpu_tcycles) {
  int cpu_multiplier = double_speed ? 2 : 1;
  int dma_ticks = progressDma();
  int ppu_ticks = cpu_tcycles / cpu_multiplier + dma_ticks;
  int cpu_ticks = cpu_tcycles + dma_ticks * cpu_multiplier;

  // TODO: Tick devices
  bool timer_interrupt = timer.tick(cpu_ticks);
  int_request |= timer_interrupt << kIntOffTimer;
}

void Bus::reset() {
  // TODO: Reset devices
}

uint8_t Bus::read(uint16_t addr) {
  if ((addr < 0x4000) || (addr >= 0xA000 || addr < 0xC000)) {
    return mbc ? mbc->read(addr) : 0;
  } else if (addr >= 0x8000 && addr < 0xA000) {
    size_t bank = 0x2000 * (cgb_mode ? vram_bank : 0);
    return (*vram)[bank + (addr - 0x8000)];
  } else if (addr >= 0xC000 && addr < 0xD000) {
    return wram[addr - 0xC000];
  } else if (addr >= 0xD000 && addr < 0xE000) {
    size_t bank = 0x1000 * (cgb_mode ? wram_bank : 0);
    return wram[bank + (addr - 0xC000)];
  } else if (addr >= 0xE000 && addr < 0xFE00) {
    // TODO: Echo RAM
  } else if (addr >= 0xFE00 && addr < 0xFEA0) {
    return (*oam)[addr - 0xFE00];
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
  if ((addr < 0x4000) || (addr >= 0xA000 || addr < 0xC000)) {
    if (mbc) mbc->write(addr, data);
  } else if (addr >= 0x8000 && addr < 0xA000) {
    size_t bank = 0x2000 * (cgb_mode ? vram_bank : 0);
    (*vram)[bank + (addr - 0x8000)] = data;
  } else if (addr >= 0xC000 && addr < 0xD000) {
    wram[addr - 0xC000] = data;
  } else if (addr >= 0xD000 && addr < 0xE000) {
    size_t bank = 0x1000 * (cgb_mode ? wram_bank : 0);
    wram[bank + (addr - 0xC000)] = data;
  } else if (addr >= 0xE000 && addr < 0xFE00) {
    // TODO: Echo RAM
  } else if (addr >= 0xFE00 && addr < 0xFEA0) {
    (*oam)[addr - 0xFE00] = data;
  } else if (addr >= 0xFF00 && addr < 0xFF80) {
    ioWrite(addr, data);
  } else if (addr >= 0xFF80 && addr < 0xFFFF) {
    hram[addr - 0xFF80] = data;
  } else if (addr == 0xFFFF) {
    int_enable = data;
  }
}

uint8_t Bus::ioRead(uint16_t addr) {
  // TODO: FF56 - Infrared
  // TODO: FF6C - Object priority
  if (addr == 0xFF00) {
    // TODO: Controller
  } else if (addr == 0xFF01 || addr == 0xFF02) {
    // TODO: Communication
  } else if (addr >= 0xFF04 && addr <= 0xFF07) {
    return timer.read(addr);
  } else if (addr == 0xFF0F) {
    return int_request;
  } else if (addr >= 0xFF10 && addr <= 0xFF26) {
    // TODO: Sound
  } else if (addr >= 0xFF30 && addr <= 0xFF3F) {
    // TODO :Waveform RAM
  } else if (addr >= 0xFF40 && addr <= 0xFF4B) {
    // TODO: LCD
  } else if (addr == 0xFF4D) {
    return (double_speed << 7) | prepare_speed_switch;
  } else if (addr == 0xFF4F) {
    return vram_bank;
  } else if (addr == 0xFF50) {
    // TODO: Set to non-zero to disable boot ROM
  } else if (addr >= 0xFF51 && addr <= 0xFF54) {
    return hdma_src_dst[addr - 0xFF51];
  } else if (addr == 0xFF55) {
    return ((hdma_mode != HdmaMode::kHdmaNone) << 7) | hdma_len;
  } else if (addr == 0xFF68 || addr == 0xFF69) {
    // TODO: BG/OBJ Palettes
  } else if (addr == 0xFF70) {
    return wram_bank;
  }

  return 0;
}

void Bus::ioWrite(uint16_t addr, uint8_t data) {
  if (addr == 0xFF00) {
    // TODO: Controller
  } else if (addr == 0xFF01 || addr == 0xFF02) {
    // TODO: Communication
  } else if (addr >= 0xFF04 && addr <= 0xFF07) {
    timer.write(addr, data);
  } else if (addr == 0xFF0F) {
    int_request = data;
  } else if (addr >= 0xFF10 && addr <= 0xFF26) {
    // TODO: Sound
  } else if (addr >= 0xFF30 && addr <= 0xFF3F) {
    // TODO :Waveform RAM
  } else if (addr >= 0xFF40 && addr <= 0xFF4B) {
    // TODO: LCD
  } else if (addr == 0xFF4D) {
    prepare_speed_switch = data & 1;
  } else if (addr == 0xFF4F) {
    vram_bank = data & 1;
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
  } else if (addr == 0xFF68 || addr == 0xFF69) {
    // TODO: BG/OBJ Palettes
  } else if (addr == 0xFF70) {
    wram_bank = data & 0b111;
  }
}

uint8_t Bus::get_triggered_interrupts() { return int_enable & int_request; }

void Bus::clear_interrupt(int bit_n) { int_request &= !(1 << bit_n); }

int Bus::progressDma() {
  // TODO: Progress any active DMA
  return 0;
}

void Bus::switchSpeed() {
  if (prepare_speed_switch) {
    double_speed = !double_speed;
    prepare_speed_switch = false;
  }
}
