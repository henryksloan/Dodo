#include "ppu.h"

#include <algorithm>
#include <iostream>
#include <vector>

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

      if (this->lcd_y >= 144 && this->stat_mode != kModeVblank) {
        // VBlank line
        this->stat_mode = kModeVblank;
        interrupts |= kIntMaskVblank;
        if (this->mode_1_interrupt) interrupts |= kIntMaskStat;

        window_start_line = -1;
        window_internal_line = 0;
      }
    }

    if (this->lcd_y < 144) {
      // Non-VBlank line
      if (this->ppu_tick_divider < 80) {
        if (this->stat_mode != kModeOamSearch) {
          this->stat_mode = kModeOamSearch;
          if (this->mode_2_interrupt) interrupts |= kIntMaskStat;
        }
      } else if (this->ppu_tick_divider < 80 + 172) {
        if (this->stat_mode != kModeTransfer) {
          // if (this->lcd_y == 0)
          //   std::fill(framebuffer.begin(), framebuffer.end(),
          //             std::array<uint16_t, 160>{0x7FFF});
          drawLine();
          this->stat_mode = kModeTransfer;
          if (this->mode_3_interrupt) interrupts |= kIntMaskStat;
        }
      } else {
        if (this->stat_mode != kModeHblank) {
          this->stat_mode = kModeHblank;
          if (this->mode_0_interrupt) interrupts |= kIntMaskStat;
        }
      }
    }
  }

  return interrupts;
}

uint8_t Ppu::read(uint16_t addr) {
  switch (addr) {
    case 0xFF40:
      return control;
    case 0xFF41:
      return static_cast<uint8_t>(compare_interrupt << 6) |
             static_cast<uint8_t>(mode_2_interrupt << 5) |
             static_cast<uint8_t>(mode_1_interrupt << 4) |
             static_cast<uint8_t>(mode_0_interrupt << 3) |
             static_cast<uint8_t>((lcd_y == lcd_y_compare) << 2) | stat_mode;
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

    case 0xFF69:
      return cgb_bg_palette[cgb_bg_palette_index];
    case 0xFF6B:
      return cgb_obj_palette[cgb_obj_palette_index];
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

    case 0xFF68:
      cgb_bg_palette_auto_incr = data >> 7;
      cgb_bg_palette_index = data & 0x3F;
      break;
    case 0xFF69:
      cgb_bg_palette[cgb_bg_palette_index] = data;
      if (cgb_bg_palette_auto_incr)
        cgb_bg_palette_index = (cgb_bg_palette_index + 1) % 0x40;
      break;
    case 0xFF6A:
      cgb_obj_palette_auto_incr = data >> 7;
      cgb_obj_palette_index = data & 0x3F;
      break;
    case 0xFF6B:
      cgb_obj_palette[cgb_obj_palette_index] = data;
      if (cgb_obj_palette_auto_incr)
        cgb_obj_palette_index = (cgb_obj_palette_index + 1) % 0x40;
      break;
  }
}

void Ppu::drawLine() {
  if (lcd_y >= 144) throw "Attempted to draw at invalid line";

  drawBgLine();
  drawWinLine();
  drawObjLine();
}

// TODO: Factor some code out of this and drawWinLine
void Ppu::drawBgLine() {
  bool bg_win_enable = cgb_mode || (control & 1);
  if (!bg_win_enable) return;

  uint16_t tile_map_base = ((control >> 3) & 1) ? 0x9C00 : 0x9800;
  uint16_t tile_row_index = (((lcd_y + scroll_y) / 8) % 32) * 32;
  for (uint16_t tile_col = 0; tile_col < 21; tile_col++) {
    uint16_t tile_col_index = (tile_col + (scroll_x / 8)) % 32;

    bool signed_addressing = ((control >> 4) & 1) == 0;
    uint16_t tile_data_base = signed_addressing ? 0x9000 : 0x8000;

    uint8_t tile_index =
        readVramBank0(tile_map_base + tile_row_index + tile_col_index);

    uint8_t attrs = 0;
    if (cgb_mode)
      attrs = readVramBank1(tile_map_base + tile_row_index + tile_col_index);
    bool y_flip = cgb_mode ? ((attrs >> 6) & 1) : false;
    bool x_flip = cgb_mode ? ((attrs >> 5) & 1) : false;

    uint16_t tile_start = static_cast<uint16_t>(
        tile_data_base +
        (signed_addressing ? static_cast<int8_t>(tile_index) : tile_index) *
            16);
    if (cgb_mode && ((attrs >> 3) & 1)) tile_start += 0x2000;

    size_t left_x = tile_col * 8 - (scroll_x % 8);
    uint16_t line_index = (lcd_y + scroll_y) % 8;
    size_t line_n = y_flip ? 7 - line_index : line_index;
    uint8_t least_sig_bits =
        readVramBank0(static_cast<uint16_t>(tile_start + line_n * 2));
    uint8_t most_sig_bits =
        readVramBank0(static_cast<uint16_t>(tile_start + line_n * 2 + 1));

    for (size_t pixel = 0; pixel < 8; pixel++) {
      size_t pixel_num = x_flip ? pixel : 7 - pixel;
      size_t pixel_index_x = left_x + pixel;
      if (pixel_index_x >= 160) continue;

      uint16_t color;
      if (cgb_mode) {
        uint8_t palette_num = attrs & 0b111;
        uint8_t palette_i =
            static_cast<uint8_t>(((most_sig_bits >> pixel_num) & 1) << 1) |
            static_cast<uint8_t>((least_sig_bits >> pixel_num) & 1);
        uint8_t color_i = palette_num * 8 + palette_i * 2;
        color = static_cast<uint16_t>((cgb_bg_palette[color_i + 1]) << 8) |
                cgb_bg_palette[color_i];
      } else {
        uint8_t palette_i =
            static_cast<uint8_t>(((most_sig_bits >> (7 - pixel)) & 1) << 1) |
            static_cast<uint8_t>((least_sig_bits >> (7 - pixel)) & 1);
        uint8_t color_i = (dmg_bg_palette >> (palette_i * 2)) & 0b11;
        color = dmg_colors[color_i];
      }
      framebuffer[lcd_y][pixel_index_x] = color;
    }
  }
}

void Ppu::drawWinLine() {
  bool win_enable = (control >> 5) & 1;
  bool bg_win_enable = cgb_mode || (control & 1);
  if (!win_enable || !bg_win_enable) return;
  if (window_y > lcd_y || window_x > 166) return;

  if (window_start_line == -1) window_start_line = lcd_y;

  uint16_t tile_map_base = ((control >> 6) & 1) ? 0x9C00 : 0x9800;

  uint16_t window_line =
      static_cast<uint8_t>(window_start_line) + window_internal_line - window_y;

  for (uint16_t tile_col = 0; tile_col < 21; tile_col++) {
    uint16_t tile_row_index = (window_line / 8) * 32;
    uint16_t tile_col_index = tile_col;

    bool signed_addressing = ((control >> 4) & 1) == 0;
    uint16_t tile_data_base = signed_addressing ? 0x9000 : 0x8000;

    uint8_t tile_index =
        readVramBank0(tile_map_base + tile_row_index + tile_col_index);

    uint8_t attrs = 0;
    if (cgb_mode)
      attrs = readVramBank1(tile_map_base + tile_row_index + tile_col_index);
    bool y_flip = cgb_mode ? ((attrs >> 6) & 1) : false;
    bool x_flip = cgb_mode ? ((attrs >> 5) & 1) : false;

    uint16_t tile_start = static_cast<uint16_t>(
        tile_data_base +
        (signed_addressing ? static_cast<int8_t>(tile_index) : tile_index) *
            16);
    if (cgb_mode && ((attrs >> 3) & 1)) tile_start += 0x2000;

    size_t left_x = tile_col * 8 + (window_x - 7);
    uint16_t line_index = window_line % 8;
    size_t line_n = y_flip ? 7 - line_index : line_index;
    uint8_t least_sig_bits =
        readVramBank0(static_cast<uint16_t>(tile_start + line_n * 2));
    uint8_t most_sig_bits =
        readVramBank0(static_cast<uint16_t>(tile_start + line_n * 2 + 1));

    for (size_t pixel = 0; pixel < 8; pixel++) {
      size_t pixel_num = x_flip ? pixel : 7 - pixel;
      size_t pixel_index_x = left_x + pixel;
      if (pixel_index_x >= 160) continue;

      uint16_t color;
      if (cgb_mode) {
        uint8_t palette_num = attrs & 0b111;
        uint8_t palette_i =
            static_cast<uint8_t>(((most_sig_bits >> pixel_num) & 1) << 1) |
            static_cast<uint8_t>((least_sig_bits >> pixel_num) & 1);
        uint8_t color_i = palette_num * 8 + palette_i * 2;
        color = static_cast<uint16_t>((cgb_bg_palette[color_i + 1]) << 8) |
                cgb_bg_palette[color_i];
      } else {
        uint8_t palette_i =
            static_cast<uint8_t>(((most_sig_bits >> (7 - pixel)) & 1) << 1) |
            static_cast<uint8_t>((least_sig_bits >> (7 - pixel)) & 1);
        uint8_t color_i = (dmg_bg_palette >> (palette_i * 2)) & 0b11;
        color = dmg_colors[color_i];
      }
      framebuffer[lcd_y][pixel_index_x] = color;
    }
  }

  window_internal_line++;
}

void Ppu::drawObjLine() {
  bool obj_enable = (control >> 1) & 1;
  if (!obj_enable) return;

  std::vector<OamEntry> selected;
  selected.reserve(10);

  const bool large_obj = (control >> 2) & 1;
  const uint8_t height = large_obj ? 16 : 8;
  for (size_t oam_index = 0; oam_index < 40; oam_index++) {
    uint8_t y = oam[oam_index * 4 + 0];
    int16_t y_signed = static_cast<int16_t>(y) - 16;
    if (lcd_y >= y_signed && lcd_y < y_signed + height) {
      uint8_t x = oam[oam_index * 4 + 1];
      uint8_t tile_index = oam[oam_index * 4 + 2];
      uint8_t attrs = oam[oam_index * 4 + 3];
      selected.push_back({y, x, tile_index, attrs});

      if (selected.size() >= 10) break;
    }
  }

  // In Non-CGB mode, the smaller the X coordinate, the higher the priority.
  // When X coordinates are identical, the object located first in OAM has
  // higher priority.
  // In CGB mode, only the object’s location in OAM determines its priority. The
  // earlier the object, the higher its priority.
  if (!cgb_mode) {
    std::stable_sort(selected.begin(), selected.end(),
                     [](auto a, auto b) { return a.x < b.x; });
  }

  // Iterate backwards so the highest-priority sprites are drawn on top
  for (auto it = selected.rbegin(); it != selected.rend(); it++) {
    const auto [y, x, tile_index, attrs] = *it;

    if (x == 0 || x >= 168) continue;

    bool bg_win_over_obj = (!cgb_mode || (control & 1)) && ((attrs >> 7) & 1);
    bool y_flip = (attrs >> 6) & 1;
    bool x_flip = (attrs >> 5) & 1;
    uint8_t palette_num = cgb_mode ? (attrs & 0b111) : ((attrs >> 4) & 1);

    int16_t x_signed = static_cast<int16_t>(x) - 8;
    int16_t y_signed = static_cast<int16_t>(y) - 16;
    uint8_t line_index =
        static_cast<uint8_t>(static_cast<int16_t>(lcd_y) - y_signed);
    uint8_t tile_to_draw = it->tile_index;
    if (large_obj) {
      tile_to_draw &= ~1;
      if (line_index >= 8) {
        tile_to_draw |= 1;
        line_index -= 8;
      }

      if (y_flip) tile_to_draw ^= 1;
    }

    uint16_t tile_start = 0x8000 + tile_to_draw * 16;
    if (cgb_mode && ((attrs >> 3) & 1)) tile_start += 0x2000;

    size_t line_n = y_flip ? 7 - line_index : line_index;
    uint8_t least_sig_bits =
        readVramBank0(static_cast<uint16_t>(tile_start + line_n * 2));
    uint8_t most_sig_bits =
        readVramBank0(static_cast<uint16_t>(tile_start + line_n * 2 + 1));

    for (size_t pixel = 0; pixel < 8; pixel++) {
      size_t pixel_index_x =
          static_cast<size_t>(x_signed + static_cast<int>(pixel));
      if (pixel_index_x >= 160) continue;

      // TODO: Respect CGBA BG attr bit 7 "BG-to-OAM Priority"
      if (bg_win_over_obj) {
        uint16_t bg_color_0 =
            cgb_mode ? static_cast<uint16_t>((cgb_bg_palette[1] << 8) |
                                             cgb_bg_palette[0])
                     : dmg_colors[dmg_bg_palette & 0b11];
        if (framebuffer[lcd_y][pixel_index_x] != bg_color_0) continue;
      }

      size_t pixel_num = x_flip ? pixel : 7 - pixel;
      uint8_t palette_i =
          static_cast<uint8_t>(((most_sig_bits >> pixel_num) & 1) << 1) |
          static_cast<uint8_t>((least_sig_bits >> pixel_num) & 1);
      if (palette_i == 0) continue;
      uint16_t color;
      if (cgb_mode) {
        uint8_t color_i = palette_num * 8 + palette_i * 2;
        color = static_cast<uint16_t>((cgb_obj_palette[color_i + 1]) << 8) |
                cgb_obj_palette[color_i];
      } else {
        uint8_t color_i =
            (dmg_obj_palette[palette_num] >> (palette_i * 2)) & 0b11;
        color = dmg_colors[color_i];
      }
      framebuffer[lcd_y][pixel_index_x] = color;
    }
  }
}
