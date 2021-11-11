#include "ppu.h"

bool Ppu::tick(int ppu_ticks) {
  // TODO
  return false;
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
