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
    if (this->lcd_y == 144) {
      interrupts |= kIntMaskVblank;
      if (this->mode_1_interrupt) interrupts |= kIntMaskStat;
    }
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

  drawBg(frame);
  drawWin(frame);
  drawObj(frame);

  return frame;
}

void Ppu::drawBg(std::array<std::array<uint16_t, 160>, 144> &frame) {
  bool bg_win_enable = control & 1;
  if (!bg_win_enable) return;

  bool signed_addressing = ((control >> 4) & 1) == 0;
  uint16_t tile_data_base = signed_addressing ? 0x9000 : 0x8000;
  uint16_t tile_map_base = ((control >> 3) & 1) ? 0x9C00 : 0x9800;

  for (int tile_row = 0; tile_row < 19; tile_row++) {
    for (int tile_col = 0; tile_col < 21; tile_col++) {
      uint16_t tile_row_index = ((tile_row + (scroll_y / 8)) % 32) * 32;
      uint16_t tile_col_index = (tile_col + (scroll_x / 8)) % 32;
      uint8_t tile_index =
          readVram(tile_map_base + tile_row_index + tile_col_index);
      uint16_t tile_start =
          tile_data_base +
          (signed_addressing ? (int8_t)tile_index : tile_index) * 16;

      size_t top_y = tile_row * 8 - (scroll_y % 8);
      size_t left_x = tile_col * 8 - (scroll_x % 8);
      for (int line_n = 0; line_n < 8; line_n++) {
        uint8_t least_sig_bits = readVram(tile_start + line_n * 2);
        uint8_t most_sig_bits = readVram(tile_start + line_n * 2 + 1);

        for (int pixel = 0; pixel < 8; pixel++) {
          size_t pixel_index_y = top_y + line_n;
          size_t pixel_index_x = left_x + pixel;
          if (pixel_index_y >= 144 || pixel_index_x >= 160) continue;

          uint8_t palette_i = (((most_sig_bits >> (7 - pixel)) & 1) << 1) |
                              ((least_sig_bits >> (7 - pixel)) & 1);
          uint8_t color_i = (dmg_bg_palette >> (palette_i * 2)) & 0b11;
          uint16_t color = dmg_colors[color_i];
          frame[pixel_index_y][pixel_index_x] = color;
        }
      }
    }
  }
}

void Ppu::drawWin(std::array<std::array<uint16_t, 160>, 144> &frame) {
  bool win_enable = (control >> 5) & 1;
  if (!win_enable) return;

  bool signed_addressing = ((control >> 4) & 1) == 0;
  uint16_t tile_data_base = signed_addressing ? 0x9000 : 0x8000;
  uint16_t tile_map_base = ((control >> 6) & 1) ? 0x9C00 : 0x9800;

  for (int tile_row = 0; tile_row < 19; tile_row++) {
    for (int tile_col = 0; tile_col < 21; tile_col++) {
      uint16_t tile_row_index = tile_row * 32;
      uint16_t tile_col_index = tile_col;
      uint8_t tile_index =
          readVram(tile_map_base + tile_row_index + tile_col_index);
      uint16_t tile_start =
          tile_data_base +
          (signed_addressing ? (int8_t)tile_index : tile_index) * 16;

      size_t top_y = tile_row * 8 + window_y;
      size_t left_x = tile_col * 8 + window_x - 7;
      for (int line_n = 0; line_n < 8; line_n++) {
        uint8_t least_sig_bits = readVram(tile_start + line_n * 2);
        uint8_t most_sig_bits = readVram(tile_start + line_n * 2 + 1);

        for (int pixel = 0; pixel < 8; pixel++) {
          size_t pixel_index_y = top_y + line_n;
          size_t pixel_index_x = left_x + pixel;
          if (pixel_index_y >= 144 || pixel_index_x >= 160) continue;

          uint8_t palette_i = (((most_sig_bits >> (7 - pixel)) & 1) << 1) |
                              ((least_sig_bits >> (7 - pixel)) & 1);
          uint8_t color_i = (dmg_bg_palette >> (palette_i * 2)) & 0b11;
          uint16_t color = dmg_colors[color_i];
          frame[pixel_index_y][pixel_index_x] = color;
        }
      }
    }
  }
}

void Ppu::drawObj(std::array<std::array<uint16_t, 160>, 144> &frame) {
  bool obj_enable = (control >> 1) & 1;
  if (!obj_enable) return;

  bool large_obj = (control >> 2) & 1;
  for (int oam_index = 0; oam_index < 40; oam_index++) {
    uint8_t y = oam[oam_index * 4 + 0];
    uint8_t x = oam[oam_index * 4 + 1];
    if (x == 0 || x >= 168 || (large_obj && y == 0) || (!large_obj && y <= 8))
      continue;
    int16_t y_signed = (int16_t)y - 16;
    int16_t x_signed = (int16_t)x - 8;

    uint8_t tile_index = oam[oam_index * 4 + 2];
    if (large_obj) tile_index &= ~1;
    uint8_t attrs = oam[oam_index * 4 + 3];
    bool bg_win_over_obj = (attrs >> 7) & 1;
    bool y_flip = (attrs >> 6) & 1;
    bool x_flip = (attrs >> 5) & 1;
    bool palette_num = (attrs >> 4) & 1;

    int n_tiles = (large_obj ? 2 : 1);
    for (int tile = 0; tile < n_tiles; tile++) {
      uint16_t tile_start =
          0x8000 + (tile_index + (y_flip ? (n_tiles - tile - 1) : tile)) * 16;
      for (int line_index = 0; line_index < 8; line_index++) {
        int line_n = y_flip ? 7 - line_index : line_index;
        uint8_t least_sig_bits = readVram(tile_start + line_n * 2);
        uint8_t most_sig_bits = readVram(tile_start + line_n * 2 + 1);

        for (int pixel = 0; pixel < 8; pixel++) {
          size_t pixel_index_y = y_signed + line_index + tile * 8;
          size_t pixel_index_x = x_signed + pixel;
          if (pixel_index_y >= 144 || pixel_index_x >= 160) continue;

          if (bg_win_over_obj && frame[pixel_index_y][pixel_index_x] !=
                                     dmg_colors[dmg_bg_palette & 0b11])
            continue;

          size_t pixel_num = x_flip ? pixel : 7 - pixel;
          uint8_t palette_i = (((most_sig_bits >> pixel_num) & 1) << 1) |
                              ((least_sig_bits >> pixel_num) & 1);
          if (palette_i == 0) continue;
          uint8_t color_i =
              (dmg_obj_palette[palette_num] >> (palette_i * 2)) & 0b11;
          uint16_t color = dmg_colors[color_i];
          frame[pixel_index_y][pixel_index_x] = color;
        }
      }
    }
  }
}
