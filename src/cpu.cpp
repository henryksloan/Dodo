#include "cpu.h"

#include <iostream>
#include <tuple>
#include <vector>

Cpu::Cpu() { initOpcodeTables(); }

// https://gbdev.io/pandocs/CPU_Instruction_Set.html
void Cpu::initOpcodeTables() {
  const auto get_set = [](CpuRegister &reg) {
    return std::make_tuple(
        [&] { return reg.get(); }, [&](uint16_t val) { reg.set(val); },
        [&] { return reg.get_hi(); }, [&](uint8_t val) { reg.set_hi(val); },
        [&] { return reg.get_lo(); }, [&](uint8_t val) { reg.set_lo(val); });
  };

  using getter = std::function<uint8_t()>;
  using getters = std::vector<getter>;
  using setter = std::function<void(uint8_t)>;
  using setters = std::vector<setter>;

  const auto [get_af, set_af, get_a, set_a, get_f, set_f] = get_set(af);
  const auto [get_bc, set_bc, get_b, set_b, get_c, set_c] = get_set(bc);
  const auto [get_de, set_de, get_d, set_d, get_e, set_e] = get_set(de);
  const auto [get_hl, set_hl, get_h, set_h, get_l, set_l] = get_set(hl);

  const auto get_mem_hl = [this]() { /* TODO */ return 0; };
  const auto set_mem_hl = [this](uint8_t val) { /* TODO */ };

  // === 8-bit load instructions ===
  const auto ld = [&](setter dst, getter src) { return [=] { dst(src()); }; };

  // All the opcodes with hi nybble 4, 5, 6, and 7, excluding $76 (HALT)
  auto src_8_bit =
      getters{get_b, get_c, get_d, get_e, get_h, get_l, get_mem_hl, get_a};
  for (int lo = 0; lo < src_8_bit.size(); lo++) {
    auto &src = src_8_bit[lo];
    opcodes[0x40 | lo] = ld(set_b, src);
    opcodes[0x50 | lo] = ld(set_d, src);
    opcodes[0x60 | lo] = ld(set_h, src);
    if (lo != 0x6) opcodes[0x70 | lo] = ld(set_mem_hl, src);
    opcodes[0x40 | (lo + 0x8)] = ld(set_c, src);
    opcodes[0x50 | (lo + 0x8)] = ld(set_e, src);
    opcodes[0x60 | (lo + 0x8)] = ld(set_l, src);
    opcodes[0x70 | (lo + 0x8)] = ld(set_a, src);
  }

  const auto get_mem_bc = [this] { /* TODO */ return 0; };
  const auto get_mem_de = [this] { /* TODO */ return 0; };
  const auto get_mem_hl_inc = [this] { /* TODO */ return 0; };
  const auto get_mem_hl_dec = [this] { /* TODO */ return 0; };

  const auto set_mem_bc = [this](uint8_t val) { /* TODO */ };
  const auto set_mem_de = [this](uint8_t val) { /* TODO */ };
  const auto set_mem_hl_inc = [this](uint8_t val) { /* TODO */ };
  const auto set_mem_hl_dec = [this](uint8_t val) { /* TODO */ };

  // $02, $12, $22, $32, $0A, $1A, $2A, $3A
  auto src_8_bit_mem =
      getters{get_mem_bc, get_mem_de, get_mem_hl_inc, get_mem_hl_dec};
  auto dst_8_bit_mem =
      setters{set_mem_bc, set_mem_de, set_mem_hl_inc, set_mem_hl_dec};
  for (int i = 0; i < 4; i++) {
    opcodes[(i * 0x10) | 0x2] = ld(dst_8_bit_mem[i], get_a);
    opcodes[(i * 0x10) | 0xA] = ld(set_a, src_8_bit_mem[i]);
  }

  // $06, $16, $26, $36, $0E, $1E, $6E, $3E
  const auto d8 = [this] { /* TODO */ return 0; };
  auto dst_8_bit_lo_6 = setters{set_b, set_d, set_h, set_mem_hl};
  auto dst_8_bit_lo_e = setters{set_c, set_e, set_l, set_a};
  for (int i = 0; i < 4; i++) {
    opcodes[(i * 0x10) | 0x6] = ld(dst_8_bit_lo_6[i], d8);
    opcodes[(i * 0x10) | 0xE] = ld(dst_8_bit_lo_e[i], d8);
  }

  // $E0, $F0
  const auto get_io_a8 = [this] { /* TODO */ return 0; };
  const auto set_io_a8 = [this](uint8_t val) { /* TODO */ };
  opcodes[0xE0] = ld(set_io_a8, get_a);
  opcodes[0xF0] = ld(set_a, get_io_a8);

  // $E2, $F2
  const auto get_mem_c = [this] { /* TODO */ return 0; };
  const auto set_mem_c = [this](uint8_t val) { /* TODO */ };
  opcodes[0xE2] = ld(set_mem_c, get_a);
  opcodes[0xF2] = ld(set_a, get_mem_c);

  // $EA, $FA
  const auto get_mem_a16 = [this] { /* TODO */ return 0; };
  const auto set_mem_a16 = [this](uint8_t val) { /* TODO */ };
  opcodes[0xEA] = ld(set_mem_a16, get_a);
  opcodes[0xFA] = ld(set_a, get_mem_a16);

  // === 16-bit load instructions ===
  using getter16 = std::function<uint16_t()>;
  using getters16 = std::vector<getter16>;
  using setter16 = std::function<void(uint16_t)>;
  using setters16 = std::vector<setter16>;
  const auto ld16 = [&](setter16 dst, getter16 src) {
    return [=] { dst(src()); };
  };

  // $01, $11, $21, $31
  const auto d16 = [this] { /* TODO */ return 0; };
  opcodes[0x01] = ld16(set_bc, d16);
  opcodes[0x11] = ld16(set_de, d16);
  opcodes[0x21] = ld16(set_hl, d16);
  opcodes[0x31] = ld16([this](uint16_t val) { sp.set(val); }, d16);

  // $08
  const auto set_mem16_a16 = [this](uint16_t val) { /* TODO */ };
  opcodes[0x08] = ld16(set_mem_a16, [this] { return sp.get(); });

  // $C1, $D1, $E1, $F1, $C5, $D5, $E5, $F5
  const auto push = [&](getter16 src) {
    return [=] {
      sp.set(sp.get() - 2);
      // TODO: Write from src to (SP)
    };
  };
  const auto pop = [&](setter16 dst) {
    return [=] {
      // TODO: Read from (SP) to dst
      sp.set(sp.get() + 2);
    };
  };
  const auto push_regs = getters16{get_bc, get_de, get_hl, get_af};
  const auto pop_regs = setters16{set_bc, set_de, set_hl, set_af};
  for (int i = 0xC; i <= 0xF; i++) {
    opcodes[(i * 0x10) | 0x1] = push(push_regs[i]);
    opcodes[(i * 0x10) | 0x5] = pop(pop_regs[i]);
  }

  // $F8, $F9
  const auto sp_plus_r8 = [this] { /* TODO */ return 0; };
  opcodes[0xF8] = ld16(set_hl, sp_plus_r8);
  opcodes[0xF9] = ld16([this](uint16_t val) { sp.set(val); }, get_hl);
}
