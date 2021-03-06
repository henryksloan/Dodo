#include "cpu.h"

#include <iomanip>
#include <iostream>
#include <tuple>
#include <vector>

Cpu::Cpu(std::shared_ptr<Bus> bus) : bus(bus) {
  initOpcodeTables();
  reset(false);
}

int Cpu::step() {
  if (check_for_interrupt()) {
    return 4;
  }

  if (halted) {
    return 1;
  }

  // TODO: Make this part of Bus (maybe a class if it should be more complex)
  if (bus->read(0xFF02) == 0x81) {
    char c = static_cast<char>(bus->read(0xFF01));
    std::cout << c;
    bus->write(0xFF02, 0);
  }

  uint8_t opcode = bus->read(pc.get());
  // std::cout << std::hex << std::setfill('0') << std::setw(4)
  //           << static_cast<int>(pc.get()) << ": " << std::setw(2)
  //           << static_cast<int>(opcode) << std::setw(4)
  //           << " AF:" << std::setfill('0') << std::setw(4)
  //           << static_cast<int>(af.get()) << " BC:" << std::setfill('0')
  //           << std::setw(4) << static_cast<int>(bc.get())
  //           << " DE:" << std::setfill('0') << std::setw(4)
  //           << static_cast<int>(de.get()) << " HL:" << std::setfill('0')
  //           << std::setw(4) << static_cast<int>(hl.get())
  //           << " SP:" << std::setfill('0') << std::setw(4)
  //           << static_cast<int>(sp.get()) << '\n';
  pc.set(pc.get() + 1);
  if (opcode == 0xCB) {
    return execute_cb();
  } else {
    opcodes[opcode]();
    return opcodes_mcycles[opcode];
  }
}

void Cpu::reset(bool cgb_mode) {
  // https://gbdev.io/pandocs/Power_Up_Sequence.html
  // TODO: possibly address edge cases
  af.set(cgb_mode ? 0x11B0 : 0x01B0);
  bc.set(0x0013);
  de.set(0x00D8);
  hl.set(0x014D);
  pc.set(0x0100);
  sp.set(0xFFFE);
}

bool Cpu::check_for_interrupt() {
  // Awakening from a HALT doesn't require the master interrupt enable flag
  if (!ime && !halted) return false;

  uint8_t triggered = bus->get_triggered_interrupts();
  if (triggered == 0) return false;

  halted = false;
  if (ime == false) return false;
  ime = false;

  int bit_n;  // The bit index of first one
  for (bit_n = 0; bit_n < 8 && (((triggered >> bit_n) & 1) == 0); bit_n++)
    ;
  if (bit_n > 4) {
    std::cerr << "invalid interrupt number: " << bit_n << std::endl;
    exit(1);
  }

  bus->clear_interrupt(bit_n);
  sp.set(sp.get() - 2);
  bus->write16(sp.get(), pc.get());
  pc.set(0x0040 | static_cast<uint16_t>(bit_n << 3));

  return true;
}

// https://gbdev.io/pandocs/CPU_Instruction_Set.html
void Cpu::initOpcodeTables() {
  const auto get_set = [](CpuRegister &reg) {
    return std::make_tuple(
        [&] { return reg.get(); }, [&](uint16_t val) { reg.set(val); },
        [&] { return reg.get_hi(); }, [&](uint8_t val) { reg.set_hi(val); },
        [&] { return reg.get_lo(); }, [&](uint8_t val) { reg.set_lo(val); });
  };

  const auto get_a = [&] { return af.get_hi(); };
  const auto set_a = [&](uint8_t val) { af.set_hi(val); };
  const auto get_af = [&] { return af.get(); };
  const auto set_af = [&](uint16_t val) {
    af.set_hi(val >> 8);
    af.set_lo(val & 0xF0);
  };
  const auto [get_bc, set_bc, get_b, set_b, get_c, set_c] = get_set(bc);
  const auto [get_de, set_de, get_d, set_d, get_e, set_e] = get_set(de);
  const auto [get_hl, set_hl, get_h, set_h, get_l, set_l] = get_set(hl);
  const auto get_sp = [this] { return sp.get(); };
  const auto set_sp = [this](uint16_t val) { sp.set(val); };

  const auto get_mem_hl = [this]() { return bus->read(hl.get()); };
  const auto set_mem_hl = [this](uint8_t val) { bus->write(hl.get(), val); };

  // ($CB is a prefix operator, so it is executed through execute_cb)

  // === 8-bit load instructions ===

  // All the opcodes with hi nybble 4, 5, 6, and 7, excluding $76 (HALT)
  auto src_8_bit =
      getters{get_b, get_c, get_d, get_e, get_h, get_l, get_mem_hl, get_a};
  for (size_t lo = 0; lo < src_8_bit.size(); lo++) {
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

  const auto get_mem_bc = [this] { return bus->read(bc.get()); };
  const auto get_mem_de = [this] { return bus->read(de.get()); };
  const auto get_mem_hl_inc = [this] {
    uint16_t temp = hl.get();
    hl.set(temp + 1);
    return bus->read(temp);
  };
  const auto get_mem_hl_dec = [this] {
    uint16_t temp = hl.get();
    hl.set(temp - 1);
    return bus->read(temp);
  };

  const auto set_mem_bc = [this](uint8_t val) { bus->write(bc.get(), val); };
  const auto set_mem_de = [this](uint8_t val) { bus->write(de.get(), val); };
  const auto set_mem_hl_inc = [this](uint8_t val) {
    uint16_t temp = hl.get();
    hl.set(temp + 1);
    bus->write(temp, val);
  };
  const auto set_mem_hl_dec = [this](uint8_t val) {
    uint16_t temp = hl.get();
    hl.set(temp - 1);
    bus->write(temp, val);
  };

  // $02, $12, $22, $32, $0A, $1A, $2A, $3A
  auto src_8_bit_mem =
      getters{get_mem_bc, get_mem_de, get_mem_hl_inc, get_mem_hl_dec};
  auto dst_8_bit_mem =
      setters{set_mem_bc, set_mem_de, set_mem_hl_inc, set_mem_hl_dec};
  for (size_t i = 0; i < 4; i++) {
    opcodes[(i * 0x10) | 0x2] = ld(dst_8_bit_mem[i], get_a);
    opcodes[(i * 0x10) | 0xA] = ld(set_a, src_8_bit_mem[i]);
  }

  // $06, $16, $26, $36, $0E, $1E, $6E, $3E
  const auto d8 = [this] {
    uint16_t temp = pc.get();
    pc.set(temp + 1);
    return bus->read(temp);
  };
  auto dst_8_bit_lo_6 = setters{set_b, set_d, set_h, set_mem_hl};
  auto dst_8_bit_lo_e = setters{set_c, set_e, set_l, set_a};
  for (size_t i = 0; i < 4; i++) {
    opcodes[(i * 0x10) | 0x6] = ld(dst_8_bit_lo_6[i], d8);
    opcodes[(i * 0x10) | 0xE] = ld(dst_8_bit_lo_e[i], d8);
  }

  // $E0, $F0
  const auto get_io_a8 = [this] {
    uint16_t temp = pc.get();
    pc.set(temp + 1);
    return bus->read(0xFF00 + bus->read(temp));
  };
  const auto set_io_a8 = [this](uint8_t val) {
    uint16_t temp = pc.get();
    pc.set(temp + 1);
    return bus->write(0xFF00 + bus->read(temp), val);
  };
  opcodes[0xE0] = ld(set_io_a8, get_a);
  opcodes[0xF0] = ld(set_a, get_io_a8);

  // $E2, $F2
  const auto get_io_c = [this] { return bus->read(0xFF00 + bc.get_lo()); };
  const auto set_io_c = [this](uint8_t val) {
    bus->write(0xFF00 + bc.get_lo(), val);
  };
  opcodes[0xE2] = ld(set_io_c, get_a);
  opcodes[0xF2] = ld(set_a, get_io_c);

  // $EA, $FA
  const auto get_mem_a16 = [this] {
    uint16_t temp = pc.get();
    pc.set(temp + 2);
    return bus->read(bus->read16(temp));
  };
  const auto set_mem_a16 = [this](uint8_t val) {
    uint16_t temp = pc.get();
    pc.set(temp + 2);
    return bus->write(bus->read16(temp), val);
  };
  opcodes[0xEA] = ld(set_mem_a16, get_a);
  opcodes[0xFA] = ld(set_a, get_mem_a16);

  // === 16-bit load instructions ===
  const auto ld16 = [&](setter16 dst, getter16 src) {
    return [=] { dst(src()); };
  };

  // $01, $11, $21, $31
  const auto d16 = [this] {
    uint16_t temp = pc.get();
    pc.set(temp + 2);
    return bus->read16(temp);
  };
  opcodes[0x01] = ld16(set_bc, d16);
  opcodes[0x11] = ld16(set_de, d16);
  opcodes[0x21] = ld16(set_hl, d16);
  opcodes[0x31] = ld16(set_sp, d16);

  // $08
  const auto set_mem16_a16 = [this](uint16_t val) {
    uint16_t temp = pc.get();
    pc.set(temp + 2);
    bus->write16(bus->read16(temp), val);
  };
  opcodes[0x08] = ld16(set_mem16_a16, [this] { return sp.get(); });

  // $C1, $D1, $E1, $F1, $C5, $D5, $E5, $F5
  const auto push_regs = getters16{get_bc, get_de, get_hl, get_af};
  const auto pop_regs = setters16{set_bc, set_de, set_hl, set_af};
  for (size_t i = 0xC; i <= 0xF; i++) {
    opcodes[(i * 0x10) | 0x1] = pop(pop_regs[i - 0xC]);
    opcodes[(i * 0x10) | 0x5] = push(push_regs[i - 0xC]);
  }

  // $F8, $F9
  opcodes[0xF8] = add16_imm(get_sp, set_hl);
  opcodes[0xF9] = ld16(set_sp, get_hl);

  // === 8-bit arithmetic/logic instructions ===

  // All the opcodes with hi nybble 8, 9, A, and B
  for (size_t lo = 0; lo < src_8_bit.size(); lo++) {
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
  opcodes[0xF6] = logic_op(d8, std::bit_or<uint8_t>(), false);
  opcodes[0xCE] = add(d8, true);
  opcodes[0xDE] = sub(d8, true, false);
  opcodes[0xEE] = logic_op(d8, std::bit_xor<uint8_t>(), false);
  opcodes[0xFE] = sub(d8, false, true);

  // ${0,1,2,3}{4,5,C,D}
  auto src_8_bit_lo_6 = getters{get_b, get_d, get_h, get_mem_hl};
  auto src_8_bit_lo_e = getters{get_c, get_e, get_l, get_a};
  for (size_t i = 0; i < 4; i++) {
    opcodes[(i * 0x10) | 0x4] =
        step_op(src_8_bit_lo_6[i], dst_8_bit_lo_6[i], true);
    opcodes[(i * 0x10) | 0x5] =
        step_op(src_8_bit_lo_6[i], dst_8_bit_lo_6[i], false);
    opcodes[(i * 0x10) | 0xC] =
        step_op(src_8_bit_lo_e[i], dst_8_bit_lo_e[i], true);
    opcodes[(i * 0x10) | 0xD] =
        step_op(src_8_bit_lo_e[i], dst_8_bit_lo_e[i], false);
  }

  // $27
  opcodes[0x27] = daa();
  // $2F
  opcodes[0x2F] = cpl();

  // === 16-bit arithmetic/logic instructions ===
  // ${0,1,2,3}{3,9,B}
  auto src_16_bit_arith = getters16{get_bc, get_de, get_hl, get_sp};
  auto dst_16_bit_arith = setters16{set_bc, set_de, set_hl, set_sp};
  for (size_t i = 0; i < 4; i++) {
    opcodes[(i * 0x10) | 0x3] =
        step16_op(src_16_bit_arith[i], dst_16_bit_arith[i], true);
    opcodes[(i * 0x10) | 0x9] = add_hl(src_16_bit_arith[i]);
    opcodes[(i * 0x10) | 0xB] =
        step16_op(src_16_bit_arith[i], dst_16_bit_arith[i], false);
  }
  // $E8
  opcodes[0xE8] = add16_imm(get_sp, set_sp);

  // === Rotate and shift instructions ===

  // $07, $17, $0F, $1F
  opcodes[0x07] = rlc(get_a, set_a, true);
  opcodes[0x17] = rl(get_a, set_a, true);
  opcodes[0x0F] = rrc(get_a, set_a, true);
  opcodes[0x1F] = rr(get_a, set_a, true);

  auto dst_8_bit =
      setters{set_b, set_c, set_d, set_e, set_h, set_l, set_mem_hl, set_a};
  // $CB suffix ${0,1,2,3}*
  for (size_t lo = 0; lo < src_8_bit.size(); lo++) {
    auto &src = src_8_bit[lo];
    auto &dst = dst_8_bit[lo];
    cb_opcodes[0x00 | lo] = rlc(src, dst, false);
    cb_opcodes[0x10 | lo] = rl(src, dst, false);
    cb_opcodes[0x20 | lo] = sla(src, dst);
    cb_opcodes[0x30 | lo] = swap(src, dst);
    cb_opcodes[0x00 | (lo + 0x8)] = rrc(src, dst, false);
    cb_opcodes[0x10 | (lo + 0x8)] = rr(src, dst, false);
    cb_opcodes[0x20 | (lo + 0x8)] = sra(src, dst);
    cb_opcodes[0x30 | (lo + 0x8)] = srl(src, dst);
  }

  // === Single-bit operation instructions ===
  // $CB suffix ${4-F}*
  for (size_t lo = 0; lo < src_8_bit.size(); lo++) {
    auto &src = src_8_bit[lo];
    auto &dst = dst_8_bit[lo];
    for (size_t i = 0; i < 4; i++) {
      cb_opcodes[((0x4 + i) * 0x10) | lo] = bit(src, 2 * i);
      cb_opcodes[((0x4 + i) * 0x10) | (lo + 0x8)] = bit(src, 2 * i + 1);
      cb_opcodes[((0x8 + i) * 0x10) | lo] = set_reset(src, dst, 2 * i, false);
      cb_opcodes[((0x8 + i) * 0x10) | (lo + 0x8)] =
          set_reset(src, dst, 2 * i + 1, false);
      cb_opcodes[((0xC + i) * 0x10) | lo] = set_reset(src, dst, 2 * i, true);
      cb_opcodes[((0xC + i) * 0x10) | (lo + 0x8)] =
          set_reset(src, dst, 2 * i + 1, true);
    }
  }

  // === CPU control instructions ===

  // $3F
  opcodes[0x3F] = [=, this] {
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, false);
    setFlag(kFlagOffC, !getFlag(kFlagOffC));
  };
  // $37
  opcodes[0x37] = [=, this] {
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, false);
    setFlag(kFlagOffC, true);
  };
  // $00
  opcodes[0x00] = [] {};  // NOP
  // $76
  opcodes[0x76] = [=, this] { halted = true; };
  // TODO: Should this eat the garbage byte that follows $10?
  opcodes[0x10] = [=, this] { bus->switchSpeed(); };
  // $F3
  opcodes[0xF3] = [=, this] { ime = false; };
  // $FB
  opcodes[0xFB] = [=, this] { ime = true; };

  // === Jump instructions ===
  const auto get_a16 = d16;
  // $C3, $E9, ${C,D}{2,A}
  opcodes[0xC3] = jump(get_a16);
  opcodes[0xE9] = jump(get_hl);
  opcodes[0xC2] = jump(get_a16, kFlagOffZ, true);
  opcodes[0xD2] = jump(get_a16, kFlagOffC, true);
  opcodes[0xCA] = jump(get_a16, kFlagOffZ, false);
  opcodes[0xDA] = jump(get_a16, kFlagOffC, false);
  // $18, $20, $30, $28, $38
  opcodes[0x18] = jump_relative(d8);
  opcodes[0x20] = jump_relative(d8, kFlagOffZ, true);
  opcodes[0x30] = jump_relative(d8, kFlagOffC, true);
  opcodes[0x28] = jump_relative(d8, kFlagOffZ, false);
  opcodes[0x38] = jump_relative(d8, kFlagOffC, false);
  // $CD, $C4, $D4, $CC, $DC
  opcodes[0xCD] = call(get_a16);
  opcodes[0xC4] = call(get_a16, kFlagOffZ, true);
  opcodes[0xD4] = call(get_a16, kFlagOffC, true);
  opcodes[0xCC] = call(get_a16, kFlagOffZ, false);
  opcodes[0xDC] = call(get_a16, kFlagOffC, false);
  // $C9, $D9, $C0, $D0, $C8, $D8
  opcodes[0xC9] = ret(false);
  opcodes[0xD9] = ret(true);
  opcodes[0xC0] = ret(false, kFlagOffZ, true);
  opcodes[0xD0] = ret(false, kFlagOffC, true);
  opcodes[0xC8] = ret(false, kFlagOffZ, false);
  opcodes[0xD8] = ret(false, kFlagOffC, false);
  // ${C,D,E,F}{7,F}
  for (size_t i = 0; i < 4; i++) {
    opcodes[((0xC + i) * 0x10) | 0x7] = rst(static_cast<uint8_t>(0x10 * i));
    opcodes[((0xC + i) * 0x10) | 0xF] =
        rst(static_cast<uint8_t>(0x10 * i + 0x8));
  }
}

Cpu::InstrFunc Cpu::ld(setter dst, getter src) {
  return [=] { dst(src()); };
}

Cpu::InstrFunc Cpu::push(getter16 src) {
  return [=, this] {
    sp.set(sp.get() - 2);
    bus->write16(sp.get(), src());
  };
};

Cpu::InstrFunc Cpu::pop(setter16 dst) {
  return [=, this] {
    dst(bus->read16(sp.get()));
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
    af.set_hi(static_cast<uint8_t>(result));

    setFlag(kFlagOffZ, (result & 0xFF) == 0);
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, (((a & 0xf) + (b & 0xf) + carry_if_any) & 0x10) == 0x10);
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
      af.set_hi(static_cast<uint8_t>(result));
    }

    setFlag(kFlagOffZ, (result & 0xFF) == 0);
    setFlag(kFlagOffN, true);
    setFlag(kFlagOffH, ((b & 0xF) + carry_if_any) > (a & 0xF));
    setFlag(kFlagOffC, (static_cast<uint16_t>(b) + carry_if_any) > a);
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
    uint8_t result = static_cast<uint8_t>(a + diff);
    set(result);

    setFlag(kFlagOffZ, result == 0);
    setFlag(kFlagOffN, 1 - incr);
    if (incr) {
      setFlag(kFlagOffH, (((a & 0xf) + 1) & 0x10) == 0x10);
    } else {
      setFlag(kFlagOffH, (a & 0x0f) == 0);
    }
  };
}

// https://ehaskins.com/2018-01-30%20Z80%20DAA/
Cpu::InstrFunc Cpu::daa() {
  return [=, this] {
    bool new_carry = false;
    uint8_t a = af.get_hi();
    uint8_t correction = 0;
    if (getFlag(kFlagOffH) || (!getFlag(kFlagOffN) && (a & 0xf) > 9)) {
      correction |= 0x6;
    }
    if (getFlag(kFlagOffC) || (!getFlag(kFlagOffN) && a > 0x99)) {
      correction |= 0x60;
      new_carry = true;
    }
    a += getFlag(kFlagOffN) ? -correction : correction;
    af.set_hi(a);

    setFlag(kFlagOffZ, a == 0);
    setFlag(kFlagOffH, false);
    setFlag(kFlagOffC, new_carry);
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

Cpu::InstrFunc Cpu::step16_op(getter16 get, setter16 set, bool incr) {
  return [=] {
    int16_t diff = (incr * 2) - 1;  // true => 1, false => -1
    uint16_t a = get();
    uint16_t result = static_cast<uint16_t>(a + diff);
    set(result);
  };
}

Cpu::InstrFunc Cpu::add_hl(getter16 src) {
  return [=, this] {
    uint16_t a = hl.get();
    uint16_t b = src();
    uint32_t result = a + b;
    hl.set(static_cast<uint16_t>(result));

    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, ((a & 0x7ff) + (b & 0x7ff)) > 0x7ff);
    setFlag(kFlagOffC, (result & 0x10000) == 0x10000);
  };
}

Cpu::InstrFunc Cpu::add16_imm(getter16 src, setter16 dst) {
  return [=, this] {
    uint16_t a = src();
    int8_t b = static_cast<int8_t>(bus->read(pc.get()));
    pc.set(pc.get() + 1);
    uint32_t result = static_cast<uint32_t>(a + b);
    dst(static_cast<uint16_t>(result));

    setFlag(kFlagOffZ, false);
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, ((a & 0xf) + (b & 0xf)) > 0xf);
    setFlag(kFlagOffC, ((a & 0xff) + (b & 0xff)) > 0xff);
  };
}

Cpu::InstrFunc Cpu::rlc(getter src, setter dst, bool reg_a) {
  return [=, this] {
    uint8_t a = src();
    bool top = (a >> 7) == 1;
    a = static_cast<uint8_t>(a << 1) | top;
    dst(a);

    setFlag(kFlagOffZ, !reg_a && (a == 0));
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, false);
    setFlag(kFlagOffC, top);
  };
}

Cpu::InstrFunc Cpu::rl(getter src, setter dst, bool reg_a) {
  return [=, this] {
    bool old_carry = getFlag(kFlagOffC);
    uint8_t a = src();
    bool top = (a >> 7) == 1;
    a = static_cast<uint8_t>(a << 1) | old_carry;
    dst(a);

    setFlag(kFlagOffZ, !reg_a && (a == 0));
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, false);
    setFlag(kFlagOffC, top);
  };
}

Cpu::InstrFunc Cpu::rrc(getter src, setter dst, bool reg_a) {
  return [=, this] {
    uint8_t a = src();
    bool bottom = a & 1;
    a = static_cast<uint8_t>(bottom << 7) | static_cast<uint8_t>(a >> 1);
    dst(a);

    setFlag(kFlagOffZ, !reg_a && (a == 0));
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, false);
    setFlag(kFlagOffC, bottom);
  };
}

Cpu::InstrFunc Cpu::rr(getter src, setter dst, bool reg_a) {
  return [=, this] {
    bool old_carry = getFlag(kFlagOffC);
    uint8_t a = src();
    bool bottom = a & 1;
    a = static_cast<uint8_t>(old_carry << 7) | static_cast<uint8_t>(a >> 1);
    dst(a);

    setFlag(kFlagOffZ, !reg_a && (a == 0));
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, false);
    setFlag(kFlagOffC, bottom);
  };
}

Cpu::InstrFunc Cpu::sla(getter src, setter dst) {
  return [=, this] {
    uint8_t a = src();
    bool top = (a >> 7) == 1;
    a = static_cast<uint8_t>(a << 1);
    dst(a);

    setFlag(kFlagOffZ, a == 0);
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, false);
    setFlag(kFlagOffC, top);
  };
}

Cpu::InstrFunc Cpu::swap(getter src, setter dst) {
  return [=, this] {
    uint8_t a = src();
    uint8_t temp = a & 0xFF;
    a = static_cast<uint8_t>(temp << 4) | static_cast<uint8_t>(a >> 4);
    dst(a);

    setFlag(kFlagOffZ, a == 0);
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, false);
    setFlag(kFlagOffC, false);
  };
}

Cpu::InstrFunc Cpu::sra(getter src, setter dst) {
  return [=, this] {
    uint8_t a = src();
    uint8_t top = a & 0x80;
    bool bottom = a & 1;
    a = top | (a >> 1);
    dst(a);

    setFlag(kFlagOffZ, a == 0);
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, false);
    setFlag(kFlagOffC, bottom);
  };
}

Cpu::InstrFunc Cpu::srl(getter src, setter dst) {
  return [=, this] {
    uint8_t a = src();
    bool bottom = a & 1;
    a >>= 1;
    dst(a);

    setFlag(kFlagOffZ, a == 0);
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, false);
    setFlag(kFlagOffC, bottom);
  };
}

Cpu::InstrFunc Cpu::bit(getter src, size_t bit_n) {
  return [=, this] {
    bool val = (src() >> bit_n) & 1;

    setFlag(kFlagOffZ, !val);
    setFlag(kFlagOffN, false);
    setFlag(kFlagOffH, true);
  };
}

Cpu::InstrFunc Cpu::set_reset(getter src, setter dst, size_t bit_n, bool set) {
  return [=] {
    uint8_t a = src();
    a &= ~(1 << bit_n);
    if (set) a |= (1 << bit_n);
    dst(a);
  };
}

int Cpu::execute_cb() {
  uint8_t cb_opcode = bus->read(pc.get());
  pc.set(pc.get() + 1);
  cb_opcodes[cb_opcode]();
  return cb_opcodes_mcycles[cb_opcode];
}

Cpu::InstrFunc Cpu::jump(getter16 src, int condition_off /* = 0 */,
                         bool negate_condition /* = false */) {
  return [=, this] {
    uint16_t addr = src();
    if ((condition_off == 0) || (getFlag(condition_off) != negate_condition)) {
      pc.set(addr);
    }
  };
}

Cpu::InstrFunc Cpu::jump_relative(getter src, int condition_off /* = 0 */,
                                  bool negate_condition /* = false */) {
  return [=, this] {
    int8_t off = static_cast<int8_t>(src());
    if ((condition_off == 0) || (getFlag(condition_off) != negate_condition)) {
      pc.set(static_cast<uint16_t>(pc.get() + off));
    }
  };
}

Cpu::InstrFunc Cpu::call(getter16 src, int condition_off /* = 0 */,
                         bool negate_condition /* = false */) {
  return [=, this] {
    uint16_t addr = src();
    if ((condition_off == 0) || (getFlag(condition_off) != negate_condition)) {
      sp.set(sp.get() - 2);
      bus->write16(sp.get(), pc.get());
      pc.set(addr);
    }
  };
}

Cpu::InstrFunc Cpu::ret(bool enable_interrupt, int condition_off /* = 0 */,
                        bool negate_condition /* = false */) {
  return [=, this] {
    if ((condition_off == 0) || (getFlag(condition_off) != negate_condition)) {
      pc.set(bus->read16(sp.get()));
      sp.set(sp.get() + 2);
      if (enable_interrupt) {
        ime = true;
      }
    }
  };
}

Cpu::InstrFunc Cpu::rst(uint8_t addr) {
  return call([=] { return addr; });
}
