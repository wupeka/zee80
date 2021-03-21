/*
 * bigthings.cpp
 *
 *  Created on: 20 mar 2016
 *      Author: wpk
 */

#include "bigthings.h"
#include "zx48k.h"
#include "zx48krom.h"
#include <Ym2149Ex.h>
#include <YmProfiles.h>
#include <cassert>
#include <ctime>
#include <fstream>
#include <iostream>
#include <strings.h>

using namespace std;

class bigthings : public zx48k {
public:
  bigthings();
  void initialize();
  virtual bool processinput() override;
  bool fullscreen = false;
};

void bigthings::initialize() {
  memcpy(memory_, zx48k_rom, zx48k_rom_len);
  border = 7;
  tape_ = new zxtape(BIGTHINGS_TAP, BIGTHINGS_TAP_len);
  trap_ = true;
  auto_ = true;
  cpu.addtrap(0x056c);
  cpu.addtrap(0x15e1);
  emusdl.settitle("BigThings");
}

bigthings::bigthings() {}

bool bigthings::processinput() {
  emusdl.readinput();
  if (emusdl.key_pressed(SDLK_F2)) {
    if (debounce_ != SDLK_F2) {
      debounce_ = SDLK_F2;
      fullscreen = !fullscreen;
      emusdl.fullscreen(fullscreen);
    }
  } else if (emusdl.key_pressed(SDLK_F4)) {
    cout << "Quitting..." << std::endl;
    return false;
  } else if (emusdl.keys_pressed()) {
    debounce_ = 0;
  }
  return true;
}

bigthings *emu;
int main(int argc, char **argv) {
  SDL_Init(SDL_INIT_EVERYTHING);
  emu = new bigthings();
  emu->initialize();
  emu->run();
}
