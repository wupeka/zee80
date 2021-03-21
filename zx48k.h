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
#ifdef __MINGW32__
#include <SDL_audio.h>
#else
#include <SDL2/SDL_audio.h>
#endif

#include <Ym2149Ex.h>
#include <chrono>
#include <set>
#include <string>
#define INT_AUDIO_BUF_SIZE 2048
#define EARBUFOFFSET 2
#define EARBUFRESERVE 0
#define LPF_BETA 0.25
#define DCF_BETA 0.001
//#define EARCYCLES 79
//#define EARCYCLES 80
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
  virtual uint64_t contention(uint64_t address, uint64_t ts) override;

protected:
  uint64_t earcycles = 78;
  int earcycles_mini = 3;
  bool do_frame();
  EmuSDL emusdl;
  static constexpr int MEMORY_SIZE = 65536;
  z80 cpu;
  ymsample abuf_[2 * INT_AUDIO_BUF_SIZE];
  char aearbuf_[EARBUFOFFSET * INT_AUDIO_BUF_SIZE + EARBUFRESERVE];
  int aear_lpf_ = 0;
  int aear_dcf_ = 0;
  int aearbufpos_ = 0;
  int abuf_pos_ = 0;

  CYm2149Ex *ay;
  SDL_AudioDeviceID sdldev;
  uint8_t ayport;
  bool processAudio();
  std::chrono::steady_clock::time_point lastAyWrite;
  uint64_t lastEarWrite;

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
  bool quickload_ = true;
  bool auto_ = false;
  int autoload_delay_ = 0;
  std::vector<std::vector<uint8_t>> keystopress_;
  uint64_t keypressedtime_ = 0;
  static constexpr uint64_t keypresstime_ = 100000; // ~20ms
  bool turbo_ = false;
  bool fs_ = false;
  bool didtrap_ = false;
  int frame_ = 0;
  uint64_t v_diff_ = 0;
  uint64_t d_diff_ = 0;
  uint64_t a_diff_ = 0;
  int64_t acc_delay_ = 0;
  int audio_started_ = 0;
  std::chrono::steady_clock::time_point tv_s_, tv_e_;

  uint64_t pausecycles_;
};

#endif /* ZX48K_H_ */
