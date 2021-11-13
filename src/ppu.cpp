#include "ppu.h"

#include <algorithm>
#include <iostream>

uint8_t Ppu::tick(int ppu_ticks) {
  if (((control >> 7) & 1) == 0) return 0;
  uint8_t interrupts = 0;

  while (ppu_ticks > 0) {
    int delta_t = std::min(ppu_ticks, 80);
    ppu_ticks -= delta_t;
    this->ppu_tick_divider += delta_t;

    // Progress a line every 456 dots
    while (this->ppu_tick_divider >= 4 * 114) {
      this->ppu_tick_divider -= 4 * 114;
      this->lcd_y = (this->lcd_y + 1) % 154;

      if (this->compare_interrupt && (this->lcd_y == this->lcd_y_compare)) {
        interrupts |= kIntMaskStat;
      }
    }
  }

  if (this->lcd_y >= 144) {
    // VBlank line
    this->stat_mode = kModeVblank;
    if (this->mode_1_interrupt && this->lcd_y == 144)
      interrupts |= kIntMaskVblank;
  } else {
    // Non-VBlank line
    if (this->ppu_tick_divider < 80) {
      this->stat_mode = kModeOamSearch;
      if (this->mode_2_interrupt && this->ppu_tick_divider == 0)
        interrupts |= kIntMaskStat;
    } else if (this->ppu_tick_divider < 80 + 172) {
      this->stat_mode = kModeTransfer;
      if (this->mode_3_interrupt && this->ppu_tick_divider == 80)
        interrupts |= kIntMaskStat;
    } else {
      this->stat_mode = kModeHblank;
      if (this->mode_0_interrupt && this->ppu_tick_divider == 80 + 172)
        interrupts |= kIntMaskStat;
    }
  }

  return interrupts;
}

uint8_t Ppu::read(uint16_t addr) {
  switch (addr) {
    case 0xFF40:
      return control;
    case 0xFF41:
      return (compare_interrupt << 6) | (mode_2_interrupt << 5) |
             (mode_1_interrupt << 4) | (mode_0_interrupt << 3) |
             ((lcd_y == lcd_y_compare) << 2) | stat_mode;
    case 0xFF42:
      return scroll_y;
    case 0xFF43:
      return scroll_x;
    case 0xFF44:
      return lcd_y;
    case 0xFF45:
      return lcd_y_compare;
    case 0xFF47:
      return dmg_bg_palette;
    case 0xFF48:
      return dmg_obj_palette[0];
    case 0xFF49:
      return dmg_obj_palette[1];
    case 0xFF4A:
      return window_y;
    case 0xFF4B:
      return window_x;
  }
  return 0;
}

void Ppu::write(uint16_t addr, uint8_t data) {
  switch (addr) {
    case 0xFF40:
      control = data;
      break;
    case 0xFF41:
      compare_interrupt = (data >> 6) & 1;
      mode_0_interrupt = (data >> 5) & 1;
      mode_1_interrupt = (data >> 4) & 1;
      mode_2_interrupt = (data >> 3) & 1;
      break;
    case 0xFF42:
      scroll_y = data;
      break;
    case 0xFF43:
      scroll_x = data;
      break;
    case 0xFF45:
      lcd_y_compare = data;
      break;
    case 0xFF47:
      dmg_bg_palette = data;
      break;
    case 0xFF48:
      dmg_obj_palette[0] = data;
      break;
    case 0xFF49:
      dmg_obj_palette[1] = data;
      break;
    case 0xFF4A:
      window_y = data;
      break;
    case 0xFF4B:
      window_x = data;
      break;
  }
}

std::array<std::array<uint16_t, 160>, 144> Ppu::frameTest() {
  std::array<std::array<uint16_t, 160>, 144> frame;

  uint16_t tile_data_base = 0x8800;
  uint16_t tile_map_base = ((control >> 3) & 1) ? 0x9C00 : 0x9800;

  for (int tile_row = 0; tile_row < 18; tile_row++) {
    for (int tile_col = 0; tile_col < 20; tile_col++) {
      uint8_t tile_index = readVram(tile_map_base + (tile_row * 32) + tile_col);
      uint16_t tile_start = tile_data_base + tile_index * 16;
      size_t top_y = tile_row * 8;
      size_t left_x = tile_col * 8;
      for (int line_n = 0; line_n < 8; line_n++) {
        uint8_t least_sig_bits = readVram(tile_start + line_n * 2);
        uint8_t most_sig_bits = readVram(tile_start + line_n * 2 + 1);
        for (int pixel = 0; pixel < 8; pixel++) {
          uint8_t color_i = (((most_sig_bits >> (7 - pixel)) & 1) << 1) |
                            ((least_sig_bits >> (7 - pixel)) & 1);
          // TODO: Check BGP
          const uint16_t colors[4] = {0x7FFF, 0x6318, 0x4210, 0x0000};
          uint16_t color = colors[color_i];
          frame[top_y + line_n][left_x + pixel] = color;
        }
      }
    }
  }

  return frame;
}
