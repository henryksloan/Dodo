#ifndef DODO_SQUARE_CHANNEL_H_
#define DODO_SQUARE_CHANNEL_H_

#include <cstdint>

const uint8_t kDutyPatterns[4] = {
    0b00000001,
    0b10000001,
    0b10000111,
    0b01111110,
};

class SquareChannel {
 public:
  SquareChannel(const bool has_sweep)
      : has_sweep(has_sweep),
        length_enable{},
        dac_enable{},
        timer{},
        frequency{},
        duty_index{},
        duty_pattern{},
        length{},
        envelope{} {}

  void tickTimer();
  void tickLength();
  void tickEnvelope();
  void tickSweep();

  uint8_t getVolume();

  void writeSweep(uint8_t data);       // NRx0 (square1)
  void writeDutyLength(uint8_t data);  // NRx1
  void writeEnvelope(uint8_t data);    // NRx2
  void writeFreqLo(uint8_t data);      // NRx3
  void writeTrigFreqHi(uint8_t data);  // NRx4

 private:
  const bool has_sweep;
  bool enable, length_enable, dac_enable;
  uint16_t timer, frequency;
  uint8_t duty_index, duty_pattern, length;

  struct {
    uint8_t timer, period;
    bool sweep_up;                  // Volume sweep direction (0: down; 1: up)
    uint8_t volume, volume_reload;  // 0-15
    bool enable;
  } envelope;

  struct {
    uint8_t timer, period;
    uint8_t shift;
    bool sweep_down;  // Sweep Direction (1: decrease, 0: increase)
    uint16_t shadow_frequency;
    bool enable;
  } sweep;

  void trigger();

  void calculateSweep(bool write_back);
};

#endif  // DODO_SQUARE_CHANNEL_H_
