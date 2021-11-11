#include "gameboy.h"

#include <fstream>
#include <sstream>

void Gameboy::step() {
  int cpu_tcycles = cpu.step() * 4;
  bus->tick(cpu_tcycles);
}

std::optional<std::string> Gameboy::loadCartridge(std::string filename) {
  std::ifstream file(filename, std::ios::binary);
  file.unsetf(std::ios::skipws);
  if (file.fail()) {
    return "Failed to open file: " + filename;
  }

  std::streampos file_size;
  file.seekg(0, std::ios::end);
  file_size = file.tellg();
  file.seekg(0, std::ios::beg);

  std::vector<uint8_t> data;
  data.reserve(file_size);

  data.insert(data.begin(), std::istream_iterator<uint8_t>(file),
              std::istream_iterator<uint8_t>());

  if (data.size() < 0x150) {
    return "File too small";
  }

  // Try to make an MBC of the correct type, otherwise return a string error
  uint8_t mbc_type = data[0x147];
  auto mbc_result = makeMbc(mbc_type, data);
  if (std::holds_alternative<std::unique_ptr<Mbc>>(mbc_result)) {
    bus->loadMbc(std::move(std::get<0>(mbc_result)));
  } else {
    return std::get<1>(mbc_result);
  }

  bus->reset();
  cpu.reset();

  return {};
}

std::variant<std::unique_ptr<Mbc>, std::string> Gameboy::makeMbc(
    uint8_t type, const std::vector<uint8_t> &data) {
  switch (type) {
    case 0x00:
      return std::make_unique<Mbc0>(data);
      break;
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x05:
    case 0x06:
    case 0x08:
    case 0x09:
    case 0x0B:
    case 0x0C:
    case 0x0D:
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
    case 0x20:
    case 0x22:
    case 0xFC:
    case 0xFD:
    case 0xFE:
    case 0xFF: {
      std::stringstream sstream;
      sstream << "Unimplemented MBC type: ";
      sstream << std::hex << static_cast<int>(type);
      return sstream.str();
    }
    default: {
      std::stringstream sstream;
      sstream << "Invalid MBC type: ";
      sstream << std::hex << static_cast<int>(type);
      return sstream.str();
    }
  }
}
