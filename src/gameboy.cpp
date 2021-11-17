#include "gameboy.h"

#include <fstream>
#include <sstream>

#include "mbc/mbc0.h"
#include "mbc/mbc1.h"
#include "mbc/mbc3.h"
#include "mbc/mbc5.h"

bool Gameboy::step() {
  int cpu_tcycles = cpu.step() * 4;
  return bus->tick(cpu_tcycles);
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
  data.reserve(static_cast<size_t>(file_size));

  data.insert(data.begin(), std::istream_iterator<uint8_t>(file),
              std::istream_iterator<uint8_t>());

  if (data.size() < 0x150) {
    return "File too small";
  }

  // Try to make an MBC of the correct type, otherwise return a string error
  uint8_t mbc_type = data[0x147];
  const size_t ram_sizes[6] = {0, 0, 0x2000, 0x8000, 0x20000, 0x10000};
  size_t ram_size = ram_sizes[data[0x149]];
  auto mbc_result = makeMbc(mbc_type, ram_size, data);
  if (std::holds_alternative<std::unique_ptr<Mbc>>(mbc_result)) {
    bus->loadMbc(std::move(std::get<0>(mbc_result)));
  } else {
    return std::get<1>(mbc_result);
  }

  bool cgb_flag = (data[0x143] >> 7) & 1;

  bus->reset(cgb_flag);
  cpu.reset(cgb_flag);

  return {};
}

std::variant<std::unique_ptr<Mbc>, std::string> Gameboy::makeMbc(
    uint8_t type, size_t ram_size, const std::vector<uint8_t> &data) {
  switch (type) {
    case 0x00:
      return std::make_unique<Mbc0>(data, ram_size);
      break;
    case 0x01:
    case 0x02:
    case 0x03:
      return std::make_unique<Mbc1>(data, ram_size);
      break;
    case 0x0F:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
      return std::make_unique<Mbc3>(data, ram_size);
      break;
    case 0x19:
    case 0x1A:
    case 0x1B:
    case 0x1C:
    case 0x1D:
    case 0x1E:
      return std::make_unique<Mbc5>(data, ram_size);
      break;
    case 0x05:
    case 0x06:
    case 0x08:
    case 0x09:
    case 0x0B:
    case 0x0C:
    case 0x0D:
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
