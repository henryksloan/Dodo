#ifndef DODO_TIMER_H_
#define DODO_TIMER_H_

#include <cstdint>

const int kClockStep[4] = {1024, 16, 64, 256};

class Timer {
 public:
  Timer()
      : divider(),
        counter(),
        modulo(),
        cpu_tick_divider(),
        enable(),
        clock_select() {}

  bool tick(int cpu_ticks);

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t data);

 private:
  uint8_t divider, counter, modulo;
  int cpu_tick_divider, counter_divider;
  bool enable;
  uint8_t clock_select;

  int getClockStep() const { return kClockStep[clock_select]; }
};

#endif  // DODO_TIMER_H_
