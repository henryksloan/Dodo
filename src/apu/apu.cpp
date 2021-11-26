#include "apu/apu.h"

void Apu::tick(int ticks) {
  timer_divider += ticks;
  while (timer_divider >= kTimerDividerPeriod) {
    timer_divider -= kTimerDividerPeriod;
    square1.tickTimer();
    square2.tickTimer();
    // TODO: Tick wave timer
  }

  frame_divider += ticks;
  while (frame_divider >= kFrameDividerPeriod) {
    frame_divider -= kFrameDividerPeriod;

    if (frame_seq_step % 2 == 0) {
      square1.tickLength();
      square2.tickLength();
      // TODO: Tick wave and noise lengths
    }

    if (frame_seq_step == 7) {
      square1.tickEnvelope();
      square2.tickEnvelope();
      // TODO: Tick wave envelope
    }

    if (frame_seq_step == 2 || frame_seq_step == 6) {
      square1.tickSweep();
    }

    frame_seq_step = (frame_seq_step + 1) % 8;
  }

  sample_divider += ticks;
  while (sample_divider >= kSampleDividerPeriod) {
    sample_divider -= kSampleDividerPeriod;
    // auto x = square1.getVolume() / 15.0f + square2.getVolume() / 15.0f;
    auto x = square2.getVolume() / 15.0f;
    // auto x = square1.getVolume() / 15.0f;
    sample_buffer.push_back(x);
  }
}

uint8_t Apu::read(uint16_t /* addr */) {
  // TODO
  return 0;
}

void Apu::write(uint16_t addr, uint8_t data) {
  switch (addr) {
    case 0xFF10:
      square1.write_sweep(data);
      break;
    case 0xFF11:
      square1.write_duty_length(data);
      break;
    case 0xFF12:
      square1.write_envelope(data);
      break;
    case 0xFF13:
      square1.write_freq_lo(data);
      break;
    case 0xFF14:
      square1.write_trig_freq_hi(data);
      break;

    case 0xFF16:
      square2.write_duty_length(data);
      break;
    case 0xFF17:
      square2.write_envelope(data);
      break;
    case 0xFF18:
      square2.write_freq_lo(data);
      break;
    case 0xFF19:
      square2.write_trig_freq_hi(data);
      break;

    // TODO: Wave
    case 0xFF1A:
      break;
    case 0xFF1B:
      break;
    case 0xFF1C:
      break;
    case 0xFF1D:
      break;
    case 0xFF1E:
      break;

    // TODO: Noise
    case 0xFF1F:
      break;
    case 0xFF20:
      break;
    case 0xFF21:
      break;
    case 0xFF22:
      break;
    case 0xFF23:
      break;

    // TODO: Control/status
    case 0xFF24:
      break;
    case 0xFF25:
      break;
    case 0xFF26:
      break;
  }
}
