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

  const auto [get_af, set_af, get_a, set_a, get_f, set_f] = get_set(af);
  const auto [get_bc, set_bc, get_b, set_b, get_c, set_c] = get_set(bc);
  const auto [get_de, set_de, get_d, set_d, get_e, set_e] = get_set(de);
  const auto [get_hl, set_hl, get_h, set_h, get_l, set_l] = get_set(hl);

  const auto get_mem_hl = [this]() { /* TODO */ return 0; };
  const auto set_mem_hl = [this](uint8_t val) { /* TODO */ };

  // === 8-bit load instructions ===

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

  // === 8-bit arithmetic/logic instructions ===

  // All the opcodes with hi nybble 8, 9, A, and B
  for (int lo = 0; lo < src_8_bit.size(); lo++) {
    auto &src = src_8_bit[lo];
    opcodes[0x80 | lo] = add(src, false);
    opcodes[0x90 | lo] = sub(src, false, false);
    opcodes[0xA0 | lo] = logic_op(src, std::bit_and<uint8_t>(), true);
    opcodes[0xB0 | lo] = logic_op(src, std::bit_or<uint8_t>(), false);
    opcodes[0x80 | (lo + 0x8)] = add(src, true);
    opcodes[0x90 | (lo + 0x8)] = sub(src, true, false);
    opcodes[0xA0 | (lo + 0x8)] = logic_op(src, std::bit_xor<uint8_t>(), false);
    opcodes[0xB0 | (lo + 0x8)] = sub(src, false, true);
  }

  // $C6, $D6, $E6, $F6, $CE, $DE, $EE, $FE
  opcodes[0xC6] = add(d8, false);
  opcodes[0xD6] = sub(d8, false, false);
  opcodes[0xE6] = logic_op(d8, std::bit_and<uint8_t>(), true);
  opcodes[0xF6] = logic_op(d8, std::bit_or<uint8_t>(), true);
  opcodes[0xCE] = add(d8, true);
  opcodes[0xDE] = sub(d8, true, false);
  opcodes[0xEE] = logic_op(d8, std::bit_xor<uint8_t>(), true);
  opcodes[0xFE] = sub(d8, false, true);

  // ${0,1,2,3}{4,5,C,D}
  auto src_8_bit_lo_6 = getters{get_b, get_d, get_h, get_mem_hl};
  auto src_8_bit_lo_e = getters{get_c, get_e, get_l, get_a};
  for (int i = 0; i < 4; i++) {
    opcodes[(i * 0x10) | 0x4] =
        step_op(src_8_bit_lo_6[i], dst_8_bit_lo_6[i], true);
    opcodes[(i * 0x10) | 0x5] =
        step_op(src_8_bit_lo_6[i], dst_8_bit_lo_6[i], false);
    opcodes[(i * 0x10) | 0xC] =
        step_op(src_8_bit_lo_e[i], dst_8_bit_lo_e[i], true);
    opcodes[(i * 0x10) | 0xD] =
        step_op(src_8_bit_lo_e[i], dst_8_bit_lo_e[i], false);

    // TODO: $27 DAA
    opcodes[0x2F] = cpl();
  }
}

Cpu::InstrFunc Cpu::ld(setter dst, getter src) {
  return [=, this] { dst(src()); };
}

Cpu::InstrFunc Cpu::push(getter16 src) {
  return [=, this] {
    sp.set(sp.get() - 2);
    // TODO: Write from src to (SP)
  };
};

Cpu::InstrFunc Cpu::pop(setter16 dst) {
  return [=, this] {
    // TODO: Read from (SP) to dst
    sp.set(sp.get() + 2);
  };
};

Cpu::InstrFunc Cpu::add(getter src, bool carry) {
  return [=, this] {
    uint8_t a = af.get_hi();
    uint8_t b = src();
    uint16_t result = a + b;
    uint8_t carry_if_any = carry && getFlag(kFlagOffC);
    result += carry_if_any;
    af.set_hi(result);

    setFlag(kFlagOffZ, (result & 0xFF) == 0);
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, (((a & 0xf) + (b & 0xf)) & 0x10) == 0x10);
    setFlag(kFlagOffC, (result & 0x100) == 0x100);
  };
};

Cpu::InstrFunc Cpu::sub(getter src, bool carry, bool compare) {
  return [=, this] {
    uint8_t a = af.get_hi();
    uint8_t b = src();
    uint16_t result = a - b;
    uint8_t carry_if_any = carry && getFlag(kFlagOffC);
    result -= carry_if_any;
    if (!compare) {
      af.set_hi(result);
    }

    setFlag(kFlagOffZ, (result & 0xFF) == 0);
    setFlag(kFlagOffN, true);
    // TODO: Is this correct?
    setFlag(kFlagOffH, ((b & 0xF) + carry_if_any) > (a & 0xF));
    setFlag(kFlagOffC, ((uint16_t)b + carry_if_any) > a);
  };
};

Cpu::InstrFunc Cpu::logic_op(getter src,
                             std::function<uint8_t(uint8_t, uint8_t)> op,
                             bool h_flag) {
  return [=, this] {
    uint8_t result = op(af.get_hi(), src());
    af.set_hi(result);

    setFlag(kFlagOffZ, result == 0);
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, h_flag);
    setFlag(kFlagOffC, false);
  };
}

Cpu::InstrFunc Cpu::step_op(getter get, setter set, bool incr) {
  return [=, this] {
    int8_t diff = (incr * 2) - 1;  // true => 1, false => -1
    uint8_t a = get();
    uint8_t result = a + diff;
    set(result);

    setFlag(kFlagOffZ, result == 0);
    setFlag(kFlagOffN, 1 - incr);
    setFlag(kFlagOffH, (((a & 0xf) + 1) & 0x10) == 0x10);
  };
}

Cpu::InstrFunc Cpu::cpl() {
  return [=, this] {
    uint8_t a = af.get_hi();
    uint8_t result = a ^ 0xFF;
    af.set_hi(result);

    setFlag(kFlagOffN, true);
    setFlag(kFlagOffH, true);
  };
}
