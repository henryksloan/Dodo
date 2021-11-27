#include "apu/square_channel.h"

void SquareChannel::tickTimer() {
  if (timer == 0) {
    timer = frequency;
    duty_index = (duty_index + 1) % 8;
  } else {
    timer--;
  }
}

void SquareChannel::tickLength() {
  if (length_enable && length > 0) {
    length--;

    if (length == 0) {
      enable = false;
    }
  }
}

void SquareChannel::tickEnvelope() {
  if (!envelope.enable || envelope.period == 0) return;

  if (envelope.timer == 0) {
    envelope.timer = envelope.period;

    int8_t volume_delta =
        static_cast<int8_t>(envelope.sweep_up) * 2 - 1;  // 1 or -1
    uint8_t new_volume = static_cast<uint8_t>(envelope.volume + volume_delta);
    if (0 <= new_volume && new_volume <= 15) {
      envelope.volume = new_volume;
    } else {
      envelope.enable = false;
    }
  } else {
    envelope.timer--;
  }
}

void SquareChannel::tickSweep() {
  if (!has_sweep || !sweep.enable || sweep.period == 0) return;

  if (sweep.timer == 0) {
    sweep.timer = sweep.period;
    calculateSweep(false);
  } else {
    sweep.timer--;
  }
}

uint8_t SquareChannel::getVolume() {
  if (!enable || !dac_enable) return 0;
  bool duty_bit = (duty_pattern >> (7 - duty_index)) & 1;
  return envelope.volume * duty_bit;
}

void SquareChannel::writeSweep(uint8_t data) {
  sweep.shift = data & 0x7;
  sweep.sweep_down = (data >> 3) & 1;
  sweep.period = (data >> 4) & 0x7;
}

void SquareChannel::writeDutyLength(uint8_t data) {
  length = ((~data) & 0x3F) + 1;
  duty_pattern = kDutyPatterns[data >> 6];
}

void SquareChannel::writeEnvelope(uint8_t data) {
  envelope.volume_reload = data >> 4;
  envelope.sweep_up = (data >> 3) & 1;
  envelope.period = data & 0x7;

  // "any time the DAC is off the channel is kept disabled
  // (but turning the DAC back on does NOT enable the channel)."
  if ((data & 0xF8) == 0) {
    dac_enable = false;
    enable = false;
  } else {
    dac_enable = true;
  }
}

void SquareChannel::writeFreqLo(uint8_t data) {
  frequency &= 0x700;
  frequency |= ~data;
}

void SquareChannel::writeTrigFreqHi(uint8_t data) {
  frequency &= 0xFF;
  frequency |= ((~data) & 0x7) << 8;
  length_enable = (data >> 6) & 1;
  if (data >> 7) {
    trigger();
  }
}

void SquareChannel::trigger() {
  enable = dac_enable;
  if (length == 0) length = 64;
  timer = frequency;
  envelope.enable = true;
  envelope.timer = envelope.period;
  envelope.volume = envelope.volume_reload;

  if (has_sweep) {
    sweep.shadow_frequency = frequency;
    sweep.timer = sweep.period;
    sweep.enable = (sweep.period != 0 || sweep.shift != 0);
    if (sweep.shift != 0) calculateSweep(true);
  }
}

void SquareChannel::calculateSweep(bool write_back) {
  int32_t delta = sweep.shadow_frequency >> sweep.shift;
  if (sweep.sweep_down) delta = -delta;
  uint16_t new_frequency =
      static_cast<uint16_t>(sweep.shadow_frequency + delta);

  if (new_frequency > 2047) {
    enable = false;
  } else if (write_back) {
    // If the new frequency is 2047 or less and the sweep shift is not zero,
    // this new frequency is written back to the shadow frequency and square
    // 1's frequency in NR13 and NR14, then frequency calculation and overflow
    // check are run AGAIN immediately using this new value, but this second
    // new frequency is not written back.
    frequency = new_frequency;
    sweep.shadow_frequency = new_frequency;
    calculateSweep(false);
  }
}
