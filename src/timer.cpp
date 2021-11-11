#include "timer.h"

bool Timer::tick(int cpu_ticks) {
  bool interrupt = false;

  // The timer ticks every 256th CPU tick;
  // cpu_tick_divider accumulates the leftover ticks
  cpu_tick_divider += cpu_ticks;
  while (cpu_tick_divider >= 256) {
    divider++;
    cpu_tick_divider -= 256;
  }

  if (enable) {
    counter_divider += cpu_ticks;
    while (counter_divider >= getClockStep()) {
      counter++;
      if (counter == 0) {
        counter = modulo;
        interrupt = true;
      }
      counter_divider -= getClockStep();
    }
  }

  return interrupt;
}

uint8_t Timer::read(uint16_t addr) {
  switch (addr) {
    case 0xFF04:
      return divider;
    case 0xFF05:
      return counter;
    case 0xFF06:
      return modulo;
    case 0xFF07:
      return (enable << 2) | clock_select;
  }
  return 0;
}

void Timer::write(uint16_t addr, uint8_t data) {
  switch (addr) {
    case 0xFF04:
      divider = 0;
    case 0xFF05:
      counter = data;
    case 0xFF06:
      modulo = data;
    case 0xFF07:
      enable = data & 0b100;
      data = data & 0b11;
  }
}
