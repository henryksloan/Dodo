#include "bus.h"

uint8_t Bus::read(uint16_t addr) {
  // TODO
  return 0;
}

void Bus::write(uint16_t addr, uint8_t data) {
  // TODO
}

uint8_t Bus::get_triggered_interrupts() { return int_enable & int_request; }

void Bus::clear_interrupt(int bit_n) { int_request &= !(1 << bit_n); }
