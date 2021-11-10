#ifndef DODO_GAMEBOY_H_
#define DODO_GAMEBOY_H_

#include <memory>

#include "bus.h"
#include "cpu.h"

class Gameboy {
 public:
  Gameboy() : bus(std::make_shared<Bus>()), cpu(bus) {}

 private:
  std::shared_ptr<Bus> bus;
  Cpu cpu;
};

#endif  // DODO_GAMEBOY_H_
