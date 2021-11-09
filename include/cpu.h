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
};

#endif  // DODO_CPU_H_
