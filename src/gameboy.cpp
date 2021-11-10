#include "gameboy.h"

void Gameboy::step() {
  int cpu_tcycles = cpu.step() * 4;
  bus->tick(cpu_tcycles);
}
