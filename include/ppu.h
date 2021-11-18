#ifndef DODO_PPU_H_
#define DODO_PPU_H_

#include <array>
#include <cstdint>
#include <memory>

const size_t kVramSize = 0x4000;
const size_t kOamSize = 0xA0;

const int kIntMaskVblank = 0b1;
const int kIntMaskStat = 0b10;

const uint16_t dmg_colors[4] = {0x7FFF, 0x6318, 0x4210, 0x0000};

class Ppu {
 public:
  Ppu() : vram(), oam() {}

  // Returns (interrupt triggered mask, new frame ready)
  uint8_t tick(int ppu_ticks);

  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t data);

  uint8_t readVram(uint16_t addr) const {
    return vram[translateVramAddr(addr)];
  }
  void writeVram(uint16_t addr, uint8_t data) {
    vram[translateVramAddr(addr)] = data;
  }

  uint8_t readOam(uint16_t addr) const { return oam[addr - 0xFE00]; }
  void writeOam(uint16_t addr, uint8_t data) { oam[addr - 0xFE00] = data; }

  bool getVramBank() const { return vram_bank; }
  void setVramBank(bool vram_bank_) { this->vram_bank = vram_bank_; }

  void setCgbMode(bool cgb_mode_) { this->cgb_mode = cgb_mode_; }

  bool inHblank() { return stat_mode == kModeHblank; }

  const std::array<std::array<uint16_t, 160>, 144> &getFrame() const {
    return framebuffer;
  }

 private:
  std::array<uint8_t, kVramSize> vram;
  std::array<uint8_t, kOamSize> oam;

  int ppu_tick_divider;

  bool vram_bank;

  bool cgb_mode;

  uint8_t control;
  bool compare_interrupt, mode_0_interrupt, mode_1_interrupt, mode_2_interrupt,
      mode_3_interrupt;
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

  uint8_t cgb_bg_palette_index, cgb_obj_palette_index;
  bool cgb_bg_palette_auto_incr, cgb_obj_palette_auto_incr;
  uint8_t cgb_bg_palette[64];
  uint8_t cgb_obj_palette[64];

  int16_t window_start_line;
  uint8_t window_internal_line;

  std::array<std::array<uint16_t, 160>, 144> framebuffer;

  uint16_t translateVramAddr(const uint16_t addr) const {
    uint16_t bank = 0x2000 * (cgb_mode ? vram_bank : 0);
    return bank + (addr - 0x8000);
  }

  uint8_t readVramBank0(const uint16_t addr) const {
    return vram[addr - 0x8000];
  }

  uint8_t readVramBank1(const uint16_t addr) const {
    return vram[addr - 0x8000 + 0x2000];
  }

  void drawLine();
  void drawBgLine();
  void drawWinLine();
  void drawObjLine();

  void drawObj(std::array<std::array<uint16_t, 160>, 144> &frame);

  struct OamEntry {
    uint8_t y, x, tile_index, attrs;
  };
};

#endif  // DODO_PPU_H_
