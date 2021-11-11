#ifndef DODO_PPU_H_
#define DODO_PPU_H_

#include <array>
#include <cstdint>
#include <memory>

const size_t kVramSize = 0x4000;
const size_t kOamSize = 0xA0;

class Ppu {
 public:
  Ppu(std::shared_ptr<std::array<uint8_t, kVramSize>> vram,
      std::shared_ptr<std::array<uint8_t, kOamSize>> oam)
      : vram(vram), oam(oam) {}

  bool tick(int ppu_ticks);

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t data);

 private:
  std::shared_ptr<std::array<uint8_t, kVramSize>> vram;
  std::shared_ptr<std::array<uint8_t, kOamSize>> oam;

  bool cgb_mode;

  uint8_t control;
  bool compare_interrupt, mode_0_interrupt, mode_1_interrupt, mode_2_interrupt;
  enum StatMode {
    kModeHblank,
    kModeVblank,
    kModeOamSearch,
    kModeTransfer
  } stat_mode;
  uint8_t scroll_x, scroll_y;
  uint8_t lcd_y, lcd_y_compare;
  uint8_t window_x, window_y;
  uint8_t dmg_bg_palette;
  uint8_t dmg_obj_palette[2];
};

#endif  // DODO_PPU_H_
