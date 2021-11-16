#ifndef DODO_GAMEBOY_H_
#define DODO_GAMEBOY_H_

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "bus.h"
#include "cpu.h"
#include "mbc/mbc.h"

class Gameboy {
 public:
  Gameboy() : bus(std::make_shared<Bus>()), cpu(bus) {}

  // Returns number of CPU T-Cycles
  int step();

  // Attempts to laod a cartridge, returning an error string on failure.
  // This is atomic, so any failure to load the file or create the MBC
  // won't affect the system.
  std::optional<std::string> loadCartridge(std::string filename);

  // Either constructs an MBC of the given type, or returns a string error
  static std::variant<std::unique_ptr<Mbc>, std::string> makeMbc(
      uint8_t type, size_t ram_size, const std::vector<uint8_t> &data);

  // Bit 3  Down  or Start    (0=Pressed)
  // Bit 2  Up    or Select   (0=Pressed)
  // Bit 1  Left  or B        (0=Pressed)
  // Bit 0  Right or A        (0=Pressed)
  void setButtonsPressed(uint8_t action_buttons_pressed,
                         uint8_t dir_buttons_pressed) {
    bus->setButtonsPressed(action_buttons_pressed, dir_buttons_pressed);
  }

  std::array<std::array<uint16_t, 160>, 144> frameTest() {
    return bus->frameTest();
  }

 private:
  // cpu receives a copy of the bus handle, so initialization order matters here
  const std::shared_ptr<Bus> bus;
  Cpu cpu;
};

#endif  // DODO_GAMEBOY_H_
