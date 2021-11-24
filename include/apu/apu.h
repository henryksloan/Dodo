#ifndef DODO_APU_H_
#define DODO_APU_H_

#include "square_channel.h"

const uint8_t kTimerDividerPeriod = 32;
const uint16_t kFrameDividerPeriod = 512;

// https://gbdev.gg8.se/wiki/articles/Gameboy_sound_hardware
class Apu {
 public:
  Apu()
      : square1{},
        square2{},
        timer_divider{},
        frame_divider{},
        frame_seq_step{} {}

  void tick(int ticks);

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t data);

 private:
  SquareChannel square1, square2;

  uint8_t timer_divider;   // 0-31 divider for channels 1-3
  uint16_t frame_divider;  // 0-511 divider for frame sequencers
  uint8_t frame_seq_step;
};

#endif  // DODO_APU_H_
