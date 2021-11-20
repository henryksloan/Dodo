#ifndef DODO_MBC_H_
#define DODO_MBC_H_

#include <cstdint>
#include <string>
#include <string_view>

// An abstract "Memory Bus Controller" - dispatches accesses to cartridge memory
class Mbc {
 public:
  virtual ~Mbc() {}

  uint8_t read(uint16_t addr) {
    if (addr < 0x4000) {
      return readRomLo(addr);
    } else if (addr >= 0x4000 && addr < 0x8000) {
      return readRomHi(addr - 0x4000);
    } else if (addr >= 0xA000 && addr < 0xC000) {
      return readRam(addr - 0xA000);
    }
    return 0;
  }

  void write(uint16_t addr, uint8_t data) {
    if (addr < 0x4000) {
      writeRomLo(addr, data);
    } else if (addr >= 0x4000 && addr < 0x8000) {
      writeRomHi(addr - 0x4000, data);
    } else if (addr >= 0xA000 && addr < 0xC000) {
      writeRam(addr - 0xA000, data);
    }
  }

 protected:
  static std::string saveFileName(std::string_view filename) {
    return std::string(filename.substr(0, filename.find_last_of('.'))) + ".sav";
  }

 private:
  virtual uint8_t readRomLo(uint16_t addr) = 0;
  virtual uint8_t readRomHi(uint16_t addr) = 0;
  virtual uint8_t readRam(uint16_t addr) = 0;

  virtual void writeRomLo(uint16_t addr, uint8_t data) = 0;
  virtual void writeRomHi(uint16_t addr, uint8_t data) = 0;
  virtual void writeRam(uint16_t addr, uint8_t data) = 0;
};

#endif  // DODO_MBC_H_
