#include "apu/square_channel.h"

void SquareChannel::trigger() {
  if (length == 0) length = 64;
  timer = frequency;
  envelope.period_timer = envelope.period;
  envelope.volume = envelope.volume_reload;
  // TODO: Trigger sweep
}

void SquareChannel::tickTimer() {
  if (timer == 0) {
    timer = frequency;
    duty_index = (duty_index + 1) % 8;
  }
}

void SquareChannel::tickLength() {
  if (length_enable && length > 0) {
    length--;
  }
}

void SquareChannel::tickEnvelope() {
  if (envelope.enable && envelope.period != 0) {
    if (envelope.period_timer == 0) {
      envelope.period_timer = envelope.period;

      int8_t volume_delta =
          static_cast<int8_t>(envelope.sweep_up) * 2 - 1;  // 1 or -1
      uint8_t new_volume = static_cast<uint8_t>(envelope.volume + volume_delta);
      if (0 <= new_volume && new_volume <= 15) {
        envelope.volume = new_volume;
      } else {
        envelope.enable = false;
      }
    } else {
      envelope.period_timer--;
    }
  }
}

uint8_t SquareChannel::getVolume() {
  // TODO: DAC enabled?
  if (length == 0) return 0;
  bool duty_bit = (duty_pattern >> (7 - duty_index)) & 1;
  return envelope.volume * duty_bit;
}

void SquareChannel::write_duty_length(uint8_t data) {
  length = data & 0x3F;
  duty_pattern = kDutyPatterns[data >> 6];
}

void SquareChannel::write_envelope(uint8_t data) {
  envelope.volume_reload = data >> 4;
  envelope.sweep_up = (data >> 3) & 1;
  envelope.period = data & 0x7;
}

void SquareChannel::write_freq_lo(uint8_t data) {
  frequency &= 0x700;
  frequency |= data;
}

void SquareChannel::write_trig_freq_hi(uint8_t data) {
  frequency &= 0xFF;
  frequency |= data & 0x7;
  length_enable = (data >> 6) & 1;
  if (data >> 7) {
    trigger();
  }
}
