#ifndef DODO_BUS_H_
#define DODO_BUS_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "apu/apu.h"
#include "mbc/mbc.h"
#include "ppu.h"
#include "timer.h"

const size_t kWramSize = 0x8000;
const size_t kHramSize = 0x80;

const int kIntOffVBlank = 0;
const int kIntOffStat = 1;
const int kIntOffTimer = 2;
const int kIntOffSerial = 3;
const int kIntOffJoypad = 4;

class Bus {
 public:
  Bus() : wram(), hram(), ppu(), timer(), wram_bank(1) {}

  // Returns true if there is a new frame ready
  bool tick(int cpu_tcycles);

  void loadMbc(std::unique_ptr<Mbc> mbc_) { this->mbc = std::move(mbc_); }

  void reset(bool cgb_mode);

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t data);

  uint16_t read16(uint16_t addr) {
    uint8_t lo = read(addr);
    return static_cast<uint16_t>((read(addr + 1)) << 8) | lo;
  }
  void write16(uint16_t addr, uint16_t data) {
    write(addr, data & 0xFF);
    write(addr + 1, data >> 8);
  }

  uint8_t ioRead(uint16_t addr);
  void ioWrite(uint16_t addr, uint8_t data);

  uint8_t get_triggered_interrupts() const { return int_enable & int_request; }
  void clear_interrupt(int bit_n) { int_request &= ~(1 << bit_n); }

  int progressDma();
  int hdmaTransferLines(int n_lines = 1);

  void switchSpeed();

  void setButtonsPressed(uint8_t action_buttons_pressed_,
                         uint8_t dir_buttons_pressed_) {
    this->action_buttons_pressed = action_buttons_pressed_;
    this->dir_buttons_pressed = dir_buttons_pressed_;
  }

  void oamdma(uint16_t addr);

  const std::array<std::array<uint16_t, 160>, 144> &getFrame() const {
    return ppu.getFrame();
  }

  std::vector<float> takeAudioBuffer() { return apu.takeAudioBuffer(); }

 private:
  std::array<uint8_t, kWramSize> wram;
  std::array<uint8_t, kHramSize> hram;
  uint8_t serial_temp[2];

  std::unique_ptr<Mbc> mbc;

  Ppu ppu;
  Apu apu;
  Timer timer;

  uint8_t int_enable, int_request;  // $FFFF IE and $FF0F IF
  bool double_speed, prepare_speed_switch;
  bool cgb_mode;

  uint8_t wram_bank;

  enum class HdmaMode { kHdmaNone, kHdmaGeneral, kHdmaHBlank } hdma_mode;
  uint8_t hdma_src_dst[4];  // The temporary registers, not the active transfer
  uint8_t hdma_len;
  uint16_t hdma_src, hdma_dst;

  bool select_action_buttons, select_dir_buttons;
  uint8_t action_buttons_pressed, dir_buttons_pressed;
};

#endif  // DODO_BUS_H_
