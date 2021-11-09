#ifndef DODO_CPU_REGISTER_H_
#define DODO_CPU_REGISTER_H_

#include <cstdint>

class CpuRegister {
 public:
  uint16_t get() const { return (hi << 8) | lo; };
  void set(const uint16_t val) {
    lo = val & 0xFF;
    hi = val >> 8;
  };

  uint8_t get_lo() const { return lo; }
  void set_lo(const uint8_t val) { lo = val; }

  uint8_t get_hi() const { return hi; }
  void set_hi(const uint8_t val) { hi = val; }

 private:
  uint8_t lo, hi;
};

#endif  // DODO_CPU_REGISTER_H_
