#include "bus.h"

void Bus::tick(int cpu_tcycles) {
  int cpu_multiplier = double_speed ? 2 : 1;
  int dma_ticks = progressDma();
  int ppu_ticks = cpu_tcycles / cpu_multiplier + dma_ticks;
  int cpu_ticks = cpu_tcycles + dma_ticks * cpu_multiplier;
}

uint8_t Bus::read(uint16_t addr) {
  if ((addr < 0x4000) || (addr >= 0xA000 || addr < 0xC000)) {
    // TODO: Cartridge
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
    // TODO: IO Registers
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

uint8_t Bus::get_triggered_interrupts() { return int_enable & int_request; }

void Bus::clear_interrupt(int bit_n) { int_request &= !(1 << bit_n); }

int Bus::progressDma() {
  // TODO: Progress any active DMA
  return 0;
}
