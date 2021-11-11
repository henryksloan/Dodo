#include "mbc/mbc0.h"

#include <algorithm>

Mbc0::Mbc0(const std::vector<uint8_t> &data) : rom(), ram() {
  // TODO: Ensure the file is big enough; maybe use a factory method
  std::copy(data.begin(), data.begin() + 0x8000, rom.begin());
}
