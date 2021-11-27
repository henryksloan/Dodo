#ifndef DODO_APU_H_
#define DODO_APU_H_

#include <vector>

#include "square_channel.h"

const uint8_t kTimerDividerPeriod = 8;  // 32;
const uint16_t kFrameDividerPeriod = 512;

const int kCpuClockRate = 4194304;
const int kSampleRate = 44100;  // 96000
const int kSampleDividerPeriod = kCpuClockRate / kSampleRate;

// https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
class Apu {
 public:
  Apu()
      : square1(true),
        square2(false),
        timer_divider{},
        frame_divider{},
        frame_seq_step{},
        sample_divider{} {}

  void tick(int ticks);

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t data);

  std::vector<float> takeAudioBuffer() {
    std::vector<float> buffer_copy(sample_buffer);
    sample_buffer.clear();
    return buffer_copy;
  }

 private:
  SquareChannel square1, square2;

  uint8_t timer_divider;   // 0-31 divider for channels 1-3
  uint16_t frame_divider;  // 0-511 divider for frame sequencers
  uint8_t frame_seq_step;

  int sample_divider;

  std::vector<float> sample_buffer;
};

#endif  // DODO_APU_H_
