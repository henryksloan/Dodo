#include "bus.h"

void Bus::tick(int cpu_tcycles) {
  int cpu_multiplier = double_speed ? 2 : 1;
  int dma_ticks = progressDma();
  int ppu_ticks = cpu_tcycles / cpu_multiplier + dma_ticks;
  int cpu_ticks = cpu_tcycles + dma_ticks * cpu_multiplier;

  // TODO: Tick devices
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
  // TODO
}

uint8_t Bus::ioRead(uint16_t addr) {
  // TODO: FF56 - Infrared
  // TODO: FF6C - Object priority
  if (addr == 0xFF00) {
    // TODO: Controller
  } else if (addr == 0xFF01 || addr == 0xFF02) {
    // TODO: Communication
  } else if (addr >= 0xFF04 && addr <= 0xFF07) {
    // TODO: Divider and Timer
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
