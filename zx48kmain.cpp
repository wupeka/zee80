#include "zx48k.h"
zx48k *emu;
int main(int argc, char **argv) {
  SDL_Init(SDL_INIT_EVERYTHING);
  emu = new zx48k();
  emu->parse_opts(argc, argv);
  emu->initialize();
  emu->run();
}
