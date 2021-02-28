/*
 * pandora.cpp
 *
 *  Created on: 20 mar 2016
 *      Author: wpk
 */

#include "pandora.h"
#include "zx48k.h"
#include "zx48krom.h"
#include <SDL2/SDL_image.h>
#include <Ym2149Ex.h>
#include <YmProfiles.h>
#include <cassert>
#include <ctime>
#include <fstream>
#include <iostream>
#include <strings.h>

using namespace std;

class pandora : public zx48k {
public:
  pandora();
  void initialize();
  void showmap();
  void upmap();
  virtual bool processinput() override;
  virtual void writemem(uint16_t address, uint8_t v, bool dotrace) override;
  bool fullscreen = false;
  uint8_t x_;
  uint8_t y_;
  SDL_Window *mapwindow_;
  SDL_Renderer *maprenderer_;
  SDL_Texture *maptexture_;
  SDL_Texture *dot_;
};

void pandora::showmap() {
  if (mapwindow_) {
    SDL_DestroyWindow(mapwindow_);
    mapwindow_ = NULL;
    return;
  }
  mapwindow_ =
      SDL_CreateWindow("Mapa", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       1000, 1000, SDL_WINDOW_RESIZABLE);
  maprenderer_ = SDL_CreateRenderer(mapwindow_, -1, 0);
  SDL_RenderSetLogicalSize(maprenderer_, 1000, 1000);
  SDL_Surface *surf =
      IMG_Load_RW(SDL_RWFromMem(PANDORA_PNG, PANDORA_PNG_len), 1);
  maptexture_ = SDL_CreateTextureFromSurface(maprenderer_, surf);
  dot_ = SDL_CreateTexture(maprenderer_, SDL_PIXELFORMAT_ARGB8888,
                           SDL_TEXTUREACCESS_STREAMING, 25, 25);
  uint32_t b[25 * 25];
  for (int i = 0; i < 25 * 25; i++) {
    b[i] = (32 << 24) + (255 << 16); // red + alpha;
  }
  SDL_UpdateTexture(dot_, NULL, b, 25 * sizeof(uint32_t));
}

void pandora::upmap() {
  if (!mapwindow_) {
    return;
  }
  int rx = 25 * (x_ - 1);
  int ry = 25 * (40 - y_);
  SDL_Rect dest = {.x = rx, .y = ry, .w = 25, .h = 25};
  SDL_RenderClear(maprenderer_);
  SDL_RenderCopy(maprenderer_, maptexture_, NULL, NULL);
  SDL_RenderCopy(maprenderer_, dot_, NULL, &dest);
  SDL_RenderPresent(maprenderer_);
}

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

void pandora::writemem(uint16_t address, uint8_t v, bool dotrace) {
  bool change = false;
  if (address == 0x6a9e) {
    x_ = v;
    change = true;
  } else if (address == 0x6a9f) {
    y_ = v;
    change = true;
  }
  if (change) {
    cout << "New pos " << (int)x_ << " " << (int)y_ << "\n";
    upmap();
  }
  zx48k::writemem(address, v, dotrace);
}

pandora::pandora() {}

bool pandora::processinput() {
  emusdl.readinput();
  if (emusdl.key_pressed(SDLK_F2)) {
    if (debounce_ != SDLK_F2) {
      debounce_ = SDLK_F2;
      fullscreen = !fullscreen;
      emusdl.fullscreen(fullscreen);
    }
  } else if (emusdl.key_pressed(SDLK_F5)) {
    if (debounce_ != SDLK_F5) {
      debounce_ = SDLK_F5;
      showmap();
    }
  } else if (emusdl.key_pressed(SDLK_F4)) {
    cout << "Quitting..." << std::endl;
    emusdl.fullscreen(false);
    return false;
  } else { // if (emusdl.keys_pressed()) {
    debounce_ = 0;
  }
  upmap();
  return true;
}

pandora *emu;
int main(int argc, char **argv) {
  SDL_Init(SDL_INIT_EVERYTHING);
  emu = new pandora();
  emu->initialize();
  emu->run();
}
