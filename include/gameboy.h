#ifndef DODO_GAMEBOY_H_
#define DODO_GAMEBOY_H_

#include <memory>
#include <optional>
#include <string>
#include <variant>

#include "bus.h"
#include "cpu.h"
#include "mbc/mbc.h"
#include "mbc/mbc0.h"

class Gameboy {
 public:
  Gameboy() : bus(std::make_shared<Bus>()), cpu(bus) {}

  void step();

  // Attempts to laod a cartridge, returning an error string on failure.
  // This is atomic, so any failure to load the file or create the MBC
  // won't affect the system.
  std::optional<std::string> loadCartridge(std::string filename);

  // Either constructs an MBC of the given type, or returns a string error
  std::variant<std::unique_ptr<Mbc>, std::string> makeMbc(
      uint8_t type, const std::vector<uint8_t> &data);

 private:
  // cpu receives a copy of the bus handle, so initialization order matters here
  std::shared_ptr<Bus> bus;
  Cpu cpu;
};

#endif  // DODO_GAMEBOY_H_
