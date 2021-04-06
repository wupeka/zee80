#include "zx48k.h"
zee80::zx48k *emu;
int main(int argc, char **argv) {
  SDL_Init(SDL_INIT_EVERYTHING);
  emu = new zee80::zx48k();
  emu->parse_opts(argc, argv);
  emu->initialize();
  emu->run();
}
