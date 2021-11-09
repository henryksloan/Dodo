#ifndef DODO_CPU_H_
#define DODO_CPU_H_

#include <cstdint>
#include <functional>

#include "cpu_register.h"

class Cpu {
 public:
  Cpu();

 private:
  CpuRegister af, bc, de, hl, sp, pc;

  using InstrFunc = std::function<void(void)>;
  InstrFunc opcodes[0xFF];     // Lookup table of opcodes to functions
  InstrFunc cb_opcodes[0xFF];  // Table of $CB-prefixed opcodes

  void initOpcodeTables();

  using getter = std::function<uint8_t()>;
  using getters = std::vector<getter>;
  using setter = std::function<void(uint8_t)>;
  using setters = std::vector<setter>;
  using getter16 = std::function<uint16_t()>;
  using getters16 = std::vector<getter16>;
  using setter16 = std::function<void(uint16_t)>;
  using setters16 = std::vector<setter16>;

  InstrFunc ld(setter dst, getter src);
  InstrFunc push(getter16 src);
  InstrFunc pop(setter16 dst);
};

#endif  // DODO_CPU_H_
