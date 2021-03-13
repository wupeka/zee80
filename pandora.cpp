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
#include "pandsnap.h"

using namespace std;

class pandora : public zx48k {
public:
  pandora();
  void initialize();
  void showmap();
  void upmap();
  void redraw_snap_screen();
  void save_snap();
  void load_snap();
  virtual bool processinput() override;
  virtual bool trap(uint16_t pc) override;
  virtual void writemem(uint16_t address, uint8_t v, bool dotrace) override;
  bool fullscreen = false;
  bool help_screen_ = false;
  bool load_screen_ = false;
  bool save_screen_ = false;
  int snap_selected_ = 0;
  uint8_t x_;
  uint8_t y_;
  SDL_Window *mapwindow_;
  SDL_Renderer *maprenderer_;
  SDL_Texture *maptexture_;
  SDL_Texture *dot_;
  Pandsnap *pandsnap_;
};

void pandora::showmap() {
  if (mapwindow_) {
    SDL_DestroyWindow(mapwindow_);
    mapwindow_ = NULL;
    return;
  }
  mapwindow_ =
      SDL_CreateWindow("Mapa", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                       600, 600, SDL_WINDOW_RESIZABLE);
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
  pandsnap_ = new Pandsnap("snapshot.dat");
  border = 7;
  tape_ = new zxtape(PANDORA_TAP, PANDORA_TAP_len);
  trap_ = true;
  auto_ = true;
  cpu.addtrap(0x056c);
  cpu.addtrap(0x15e1);
  cpu.addtrap(0x8dc5);
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
    upmap();
    std::cout << "X: " << (int)x_ << " Y: " << (int)y_ << std::endl;
  }
  zx48k::writemem(address, v, dotrace);
}

bool pandora::trap(uint16_t pc) {
  if (pc == 0x8dc5) {
    std::cout << "FINISHED" << std::endl;
    auto_ = true;
    cpu.reset();    
    return true;
  } else {
    return zx48k::trap(pc);
  }
}
pandora::pandora() {}

void pandora::redraw_snap_screen() {
}

void pandora::load_snap() {
      pandsnap_->Load(snap_selected_, &cpu, memory_+16384);
      ay->reset();
      tape_->reset(7);
}

void pandora::save_snap() {
      pandsnap_->Save(snap_selected_, &cpu, memory_+16384);
}

bool pandora::processinput() {
  emusdl.readinput();
  if (help_screen_) {
    if (!emusdl.get_keys().empty()) {
       if (debounce_ != 0 && emusdl.key_pressed(SDLK_F1)) {
         return true;
       }
       help_screen_ = false;
       emusdl.draw_overlay_ = false;
    } else {
      debounce_ = 0;
    }
    return true;
  }
  
  if (load_screen_ || save_screen_) {
    if (!emusdl.get_keys().empty() && debounce_ != 0) {
      return true;
    } else {
      debounce_ = 0;
    }
    if (emusdl.key_pressed(SDLK_DOWN)) {
      debounce_ = SDLK_DOWN;
      if (snap_selected_ < SNAP_SLOTS) {
        ++snap_selected_;
      }
      redraw_snap_screen();
    } else if (emusdl.key_pressed(SDLK_UP)) {
      debounce_ = SDLK_UP;
      if (snap_selected_ > 0) {
        --snap_selected_;
      }
      redraw_snap_screen();
    } else if (emusdl.key_pressed(SDLK_RETURN)) {
      debounce_ = SDLK_RETURN;
      if (load_screen_) {
        if (pandsnap_->Empty(snap_selected_)) {
          return true;
        }
        load_snap();
      } else {
        save_snap();
      }
      load_screen_ = save_screen_ = false;
      emusdl.draw_overlay_ = false;
    } else if (!emusdl.get_keys().empty()) {
      load_screen_ = save_screen_ = false;
      emusdl.draw_overlay_ = false;
    }
    return true;
  } 
  
  if (emusdl.key_pressed(SDLK_F1)) {
    if (debounce_ != SDLK_F1) {
      debounce_ = SDLK_F1;
      help_screen_ = true;
      emusdl.draw_overlay_ = true;
      // fill the screen!
    }
  } else if (emusdl.key_pressed(SDLK_F2)) {
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
  } else if (emusdl.key_pressed(SDLK_F6)) {
    if (debounce_ != SDLK_F6) {
      debounce_ = SDLK_F6;
      trace_ = true;
    }
  } else if (emusdl.key_pressed(SDLK_F7)) {
    if (debounce_ != SDLK_F7) {
      debounce_ = SDLK_F7;
      save_screen_ = true;
      emusdl.draw_overlay_ = true;
      redraw_snap_screen();
    }
  } else if (emusdl.key_pressed(SDLK_F8)) {
    if (debounce_ != SDLK_F8) {
      debounce_ = SDLK_F8;
      load_screen_ = true;
      emusdl.draw_overlay_ = true;
      redraw_snap_screen();
    }
  }  else if (emusdl.key_pressed(SDLK_F9)) {
    if (debounce_ != SDLK_F9) {
      debounce_ = SDLK_F9;
      int x,y;
      cin >> x;
      cin >> y;
      zx48k::writemem(0x6a9e, x, false);
      zx48k::writemem(0x6a9f, y, false);
    }
  } else if (emusdl.key_pressed(SDLK_F4)) {
    cout << "Quitting..." << std::endl;
    emusdl.fullscreen(false);
    return false;
  } else if (emusdl.key_pressed(SDLK_BACKSPACE)) {
    keystopress_ = std::vector<std::vector<uint8_t > > { {(0<<3|0)},  {(0<<3|0)}, {(0<<3|0)}, {(0<<3|0), (4 << 3 | 0)}, {(0<<3|0), (4 << 3 | 0)}, {(0<<3|0), (4 << 3 | 0)}, {(0<<3|0), (4 << 3 | 0)} };
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
