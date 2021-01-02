/*
 * pandora.cpp
 *
 *  Created on: 20 mar 2016
 *      Author: wpk
 */

#include "zx48k.h"
#include <Ym2149Ex.h>
#include <YmProfiles.h>
#include <boost/program_options.hpp>
#include <cassert>
#include <ctime>
#include <fstream>
#include <iostream>
#include <strings.h>
#include "zx48krom.h"
#include "pandora.h"

using namespace std;
namespace po = boost::program_options;

class pandora : public zx48k {
public:
  pandora();
  void initialize();
  virtual bool processinput() override;
  bool fullscreen = false;
};

void pandora::initialize() {
  memcpy(memory_, zx48k_rom, zx48k_rom_len);
  border = 7;
  tape_ = new zxtape(PANDORA_TAP, PANDORA_TAP_len);
  trap_ = true;
  auto_ = true;
  cpu.addtrap(0x056c);
  cpu.addtrap(0x15e1);
  emusdl.settitle("Puszka Pandory");
}

pandora::pandora()
{
}

bool pandora::processinput() {
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

pandora *emu;
int main(int argc, char **argv) {
  SDL_Init(SDL_INIT_EVERYTHING);
  emu = new pandora();
  emu->initialize();
  emu->run();
}
