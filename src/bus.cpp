#include "bus.h"

void Bus::tick(int cpu_tcycles) {
  int cpu_multiplier = double_speed ? 2 : 1;
  int dma_ticks = progressDma();
  int ppu_ticks = cpu_tcycles / cpu_multiplier + dma_ticks;
  int cpu_ticks = cpu_tcycles + dma_ticks * cpu_multiplier;
}

uint8_t Bus::read(uint16_t addr) {
  // TODO
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
