#ifndef DODO_CPU_H_
#define DODO_CPU_H_

#include <cstdint>
#include <functional>
#include <memory>

#include "bus.h"
#include "cpu_register.h"

const int kFlagOffZ = 7;
const int kFlagOffN = 6;
const int kFlagOffH = 5;
const int kFlagOffC = 4;

class Cpu {
 public:
  Cpu(std::shared_ptr<Bus> bus);

 private:
  std::shared_ptr<Bus> bus;

  CpuRegister af, bc, de, hl, sp, pc;
  bool ime;  // Master interrupt enable flag

  bool getFlag(int offset) { return ((af.get_lo() >> offset) & 1) == 1; }
  void setFlag(int offset, bool val) {
    uint8_t new_f = af.get_lo();
    new_f &= !(1 << offset);
    new_f |= val << offset;
    af.set_lo(new_f);
  }

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
  InstrFunc add(getter src, bool carry);
  InstrFunc sub(getter src, bool carry, bool compare);
  InstrFunc logic_op(getter src, std::function<uint8_t(uint8_t, uint8_t)> op,
                     bool h_flag);
  InstrFunc step_op(getter get, setter set, bool incr);
  InstrFunc daa();
  InstrFunc cpl();
  InstrFunc step16_op(getter16 get, setter16 set, bool incr);
  InstrFunc add_hl(getter16 src);
  InstrFunc add_sp();
  InstrFunc rlc(getter src, setter dst, bool reg_a);
  InstrFunc rl(getter src, setter dst, bool reg_a);
  InstrFunc rrc(getter src, setter dst, bool reg_a);
  InstrFunc rr(getter src, setter dst, bool reg_a);
  InstrFunc sla(getter src, setter dst);
  InstrFunc swap(getter src, setter dst);
  InstrFunc sra(getter src, setter dst);
  InstrFunc srl(getter src, setter dst);
  InstrFunc bit(getter src, int bit_n);
  InstrFunc set_reset(getter src, setter dst, int bit_n, bool set);
  InstrFunc cb();
  InstrFunc jump(getter16 src, bool relative, int condition_off = 0,
                 bool negate_condition = false);
  InstrFunc call(getter16 src, int condition_off = 0,
                 bool negate_condition = false);
  InstrFunc ret(bool enable_interrupt, int condition_off = 0,
                bool negate_condition = false);
  InstrFunc rst(uint8_t addr);
};

#endif  // DODO_CPU_H_
