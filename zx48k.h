/*
 * zx48k.h
 *
 *  Created on: 20 mar 2016
 *      Author: wpk
 */

#ifndef ZX48K_H_
#define ZX48K_H_
#include "bushandler.h"
#include "emusdl.h"
#include "z80.h"
#include "zxtape.h"
#include <SDL2/SDL_audio.h>
#include <Ym2149Ex.h>
#include <set>
#include <string>

class zx48k : public BusHandler {
public:
  zx48k();
  void parse_opts(int argc, char **argv);
  void initialize();
  void run();

  virtual uint32_t readmem(uint16_t address, bool dotrace) override;
  virtual void writemem(uint16_t address, uint8_t value, bool dotrace) override;
  virtual uint8_t readio(uint16_t address) override;
  virtual void writeio(uint16_t address, uint8_t value) override;
  virtual bool trap(uint16_t pc) override;

protected:
  EmuSDL emusdl;
  static constexpr int MEMORY_SIZE = 65536;
  z80 cpu;

  CYm2149Ex *ay;
  SDL_AudioDeviceID sdldev;
  uint8_t ayport;
  void processAudio();
  struct timespec lastAyWrite;
  uint64_t lastEarChange;

  void scanline(int y);
  void dump();
  
  SDL_Keycode debounce_;
  virtual bool processinput();
  
  std::string romfile;
  std::string tapfile;
  zxtape *tape_ = NULL;
  uint64_t cpucycles_ = 0;

  uint8_t memory_[MEMORY_SIZE]; // simple linear model
  bool ear = false;
  std::vector<std::pair<bool, uint64_t>> earStates;
  bool mic = false;
  uint8_t border : 3;
  bool flashstate = false;
  // the 'keycode' is :
  // 0b00aaakkk
  // aaa -> address line (0-7)
  // kkk -> key line (0-5)
  // 'shift' == 0b00 000 000
  std::set<uint8_t> keyspressed;
  bool trace_ = false;
  bool trap_ = false;
  bool auto_ = false;
  std::vector<std::vector<uint8_t >> keystopress_;
  uint64_t keypressedtime_ = 0;
  static constexpr uint64_t keypresstime_ = 100000; // ~20ms
  bool turbo_ = false;
  bool fs_ = false;
  bool didtrap_ = false;
};

#endif /* ZX48K_H_ */
