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
  SquareChannel()
      : timer{},
        frequency{},
        duty_index{},
        duty_pattern{},
        length{},
        length_enable{},
        envelope{} {}

  void trigger();

  void tickTimer();
  void tickLength();
  void tickEnvelope();

  uint8_t getVolume();

  void write_duty_length(uint8_t data);   // NRx1
  void write_envelope(uint8_t data);      // NRx2
  void write_freq_lo(uint8_t data);       // NRx3
  void write_trig_freq_hi(uint8_t data);  // NRx4

 private:
  // TODO: Channel 1 sweep

  uint16_t timer, frequency;
  uint8_t duty_index, duty_pattern, length;
  bool length_enable;

  struct {
    uint8_t period_timer, period;
    bool sweep_up;                  // Volume sweep direction (0: down; 1: up)
    uint8_t volume, volume_reload;  // 0-15
    bool enable;
  } envelope;
};

#endif  // DODO_SQUARE_CHANNEL_H_
