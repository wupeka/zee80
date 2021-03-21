/*
 * pandora.cpp
 *
 *  Created on: 20 mar 2016
 *      Author: wpk
 */

#include "gitversion.h"
#include "pandora-resources.h"
#include "pandsnap.h"
#include "spectext.h"
#include "zx48k.h"
#include "zx48krom.h"
#include "platform.h"
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
  void draw_help_screen();
  void redraw_snap_screen();
  void save_snap();
  void load_snap();
  virtual bool processinput() override;
  virtual bool trap(uint16_t pc) override;
  virtual void writemem(uint16_t address, uint8_t v, bool dotrace) override;
  bool fullscreen = false;
  bool intro_loaded_ = false;
  bool help_screen_ = false;
  bool load_screen_ = false;
  bool save_screen_ = false;
  bool help_alt_ = false;
  int help_alt_scroll_ = 0;
  int snap_selected_ = 0;
  uint8_t x_;
  uint8_t y_;
  SDL_Window *mapwindow_;
  SDL_Renderer *maprenderer_;
  SDL_Texture *maptexture_;
  SDL_Texture *dot_;
  Pandsnap *pandsnap_;
  SpecText *spectext_;
  uint32_t finicount_ = 0;
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
      IMG_Load_RW(SDL_RWFromMem(PANDORA_GIF, PANDORA_GIF_len), 1);
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
  memset(memory_, 0x0f, MEMORY_SIZE);
  memcpy(memory_, zx48k_rom, zx48k_rom_len);
  pandsnap_ = new Pandsnap(getSnapshotFileLocation());
  spectext_ = new SpecText(maszyna_he_bin, emusdl.overlay_, emusdl.get_width(),
                           emusdl.get_height());
  border = 7;
  tape_ = new zxtape(PANDORA_TAP, PANDORA_TAP_len);
  trap_ = true;
  auto_ = true;
  cpu.addtrap(0x056c);
  cpu.addtrap(0x15e1);
  cpu.addtrap(0xcb03);
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
    // Game finished
    if (finicount_++ < 1500000) {
      return false;
    }
    finicount_ = 0;
    auto_ = true;
    tape_->reset(7);
    cpu.reset();
    return true;
  } else if (pc == 0xcb03) {
    // Intro loading
    intro_loaded_ = true;
    return false;
  } else {
    return zx48k::trap(pc);
  }
}
pandora::pandora() {}

void pandora::redraw_snap_screen() {
  uint32_t bg = 0x00CDCDCD;
  uint32_t fg = 0x00000000;
  for (int i = 0; i < emusdl.get_width() * emusdl.get_height(); i++) {
    emusdl.overlay_[i] = bg;
  }
  if (load_screen_) {
    spectext_->Write("WYBIERZ SLOT DO WCZYTANIA", 30, 30, 1, bg, fg);
  } else {
    spectext_->Write("WYBIERZ SLOT DO ZAPISU", 43, 30, 1, bg, fg);
  }
  int y = 45;
  auto list = pandsnap_->List();
  for (int i = 0; i < list.size(); i++) {
    uint32_t bgg, fgg;
    if (snap_selected_ == i) {
      bgg = fg;
      fgg = bg;
    } else {
      bgg = bg;
      fgg = fg;
    }
    spectext_->Write(list[i].data(), 59, y, 0, bgg, fgg);
    y += 12;
  }
}

static const std::vector<std::string> licenses = {
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "      ZEE80 EMULATOR BY wpk        ",
    "",
    "",
    "",
    "",
    "",
    "",
    "THIS SOFTWARE IS PROVIDED \"AS-IS\"",
    "WITHOUT ANY EXPRESS OR IMPLIED WAR-",
    "RANTY. IN NO EVENT WILL THE AUTHORS",
    "BE HELD LIABLE FOR ANY DAMAGES ARI-",
    "SING FROM THE USE OF THIS SOFTWARE.",
    "",
    "###################################",
    "",
    "THIS SOFTWARE CONTAINS ZX SPECTRUM ",
    "48K ROM. AMSTRAD HAVE KINDLY GIVEN ",
    "THEIR PERMISSION FOR THE REDISTRI- ",
    "BUTION OF THEIR COPYRIGHTED MATER- ",
    "IAL BUT RETAIN THAT COPYRIGHT.     ",
    "",
    "###################################",
    "",
    "THIS SOFTWARE IS USING SDL2.0 LIB- ",
    "RARY, LICENSED UNDER ZLIB LICENSE, ",
    "SEE WWW.LIBSDL.ORG",
    "",
    "###################################",
    "",
    "THIS SOFTWARE IS USING STSOUND 2.0 ",
    "LIBRARY WITH THE FOLLOWING LICENSE:",
    "",
    "REDISTRIBUTION AND USE IN SOURCE   ",
    "AND BINARY FORMS, WITH OR WITHOUT  ",
    "MODIFICATION, ARE PERMITTED PROVI- ",
    "DED THAT THE FOLLOWING CONDITIONS  ",
    "ARE MET:",
    " * REDISTRIBUTIONS OF SOURCE CODE  ",
    "MUST RETAIN THE ABOVE COPYRIGHT    ",
    "NOTICE, THIS LIST OF CONDITIONS AND",
    " THE FOLLOWING DISCLAIMER.",
    " * REDISTRIBUTIONS IN BINARY FORM  ",
    "MUST REPRODUCE THE ABOVE COPYRIGHT ",
    "NOTICE, THIS LIST OF CONDITIONS AND",
    " THE FOLLOWING DISCLAIMER IN THE   ",
    "DOCUMENTATION AND/OR OTHER MATER-  ",
    "IALS PROVIDED WITH THE DISTRIBUTION",
    "THIS SOFTWARE IS PROVIDED BY THE   ",
    "COPYRIGHT HOLDERS AND CONTRIBUTORS ",
    "\"AS IS\" AND ANY EXPRESS OR IM-   ",
    "PLIED WARRANTIES, INCLUDING, BUT   ",
    "NOT LIMITED TO, THE IMPLIED WARRAN-",
    "TIES OF MERCHANTABILITY AND FITNESS",
    "FOR A PARTICULAR PURPOSE ARE DIS-  ",
    "CLAIMED. IN NO EVENT SHALL THE CO- ",
    "PYRIGHT OWNER OR CONTRIBUTORS BE   ",
    "LIABLE FOR ANY DIRECT, INDIRECT,   ",
    "INCIDENTAL, SPECIAL, EXEMPLARY, OR ",
    "CONSEQUENTIAL DAMAGES (INCLUDING,  ",
    "BUT NOT LIMITED TO, PROCUREMENT OF ",
    "SUBSTITUTE GOODS OR SERVICES; LOSS ",
    "OF USE, DATA, OR PROFITS; OR       ",
    "BUSINESS INTERRUPTION) HOWEVER     ",
    "CAUSED AND ON ANY THEORY OF LIABI- ",
    "LITY, WHETHER IN CONTRACT, STRICT  ",
    "LIABILITY, OR TORT (INCLUDING NEG- ",
    "LIGENCE OR OTHERWISE) ARISING IN   ",
    "ANY WAY OUT OF THE USE OF THIS     ",
    "SOFTWARE, EVEN IF ADVISED OF THE   ",
    "POSSIBILITY OF SUCH DAMAGE.        "};

void pandora::draw_help_screen() {
  uint32_t bg = 0x00CDCDCD;
  uint32_t fg = 0x00000000;
  // "                             "
  // "           abcdef0           "
  char helpstring[] = "           " GIT_REVISION "           ";
  //  char helpstring[] = " " GIT_REVISION " " BUILD_TIMESTAMP;
  char *h = helpstring;
  while (*h) {
    *h = toupper(*h);
    ++h;
  }
  int x = 15;
  for (int i = 0; i < emusdl.get_width() * emusdl.get_height(); i++) {
    emusdl.overlay_[i] = bg;
  }
  if (help_alt_) {
    int has = help_alt_scroll_ - 100;
    if (has < 0) {
      has = 0;
    };
    has /= 3;
    int lineToDraw = has / 9;
    if (lineToDraw > licenses.size()) {
      help_alt_scroll_ = 0;
      lineToDraw = 0;
    }
    int lineOffs = has % 9;
    int y = 15 - lineOffs;
    while (y < 200 && lineToDraw < licenses.size()) {
      spectext_->Write(licenses[lineToDraw++].data(), 5, y, 0, bg, fg);
      y += 9;
    }
  } else {
    // "                             "
    spectext_->Write("       PUSZKA PANDORY        ", x, 20, 1, bg, fg);
    int y = 45;
    spectext_->Write("    F1 - EKRAN POMOCY        ", x, y, 1, bg, fg);
    y += 12;
    spectext_->Write("    F2 - PElNY EKRAN         ", x, y, 1, bg, fg);
    y += 12;
    spectext_->Write("    F3 - WYlaCZ TURBOLOAD    ", x, y, 1, bg, fg);
    y += 12;
    spectext_->Write("    F4 - WYJDx Z GRY         ", x, y, 1, bg, fg);
    y += 12;
    spectext_->Write("    F5 - WYsWIETL MAPe       ", x, y, 1, bg, fg);
    y += 12;
    spectext_->Write("    F7 - ZAPISZ GRe          ", x, y, 1, bg, fg);
    y += 12;
    spectext_->Write("    F8 - ZAlADUJ GRe         ", x, y, 1, bg, fg);
    y += 24;
    spectext_->Write("    PORUSZANIE:  N E S W     ", x, y, 1, bg, fg);
    y += 12;
    spectext_->Write("KOMENDY W BEZOKOLICZNIKU, NP:", x, y, 1, bg, fg);
    y += 12;
    spectext_->Write(" PlYNac, WZIac, sCIac, WEJsc ", x, y, 1, bg, fg);
    y += 24;
    spectext_->Write("    EMULATOR ZEE80: wpk      ", x, y, 1, bg, fg);
    y += 12;
    spectext_->Write(helpstring, x, y, 1, bg, fg);
  }
}

void pandora::load_snap() {
  std::cout << "Loading snap " << snap_selected_ << "\n";
  pandsnap_->Load(snap_selected_, &cpu, memory_ + 16384);
  ay->reset();
  tape_->reset(7);
  emusdl.clearkey(SDLK_RETURN);
}

void pandora::save_snap() {
  std::cout << "Saving snap " << snap_selected_ << "\n";
  pandsnap_->Save(snap_selected_, &cpu, memory_ + 16384);
}

bool pandora::processinput() {
  if (help_alt_ && help_screen_) {
    help_alt_scroll_ += 1;
    draw_help_screen();
  }
  emusdl.readinput();
  if (emusdl.key_pressed(SDLK_F4)) {
    cout << "Quitting..." << std::endl;
    emusdl.fullscreen(false);
    return false;
  }
  if (!emusdl.get_keys().empty() && debounce_ != 0) {
    return true;
  }

  if (emusdl.get_keys().empty()) {
    debounce_ = 0;
    return true;
  }

  debounce_ = *emusdl.get_keys().begin();

  if (help_screen_) {
    if (emusdl.key_pressed(SDLK_F1) && !help_alt_) {
      help_alt_ = true;
      help_alt_scroll_ = 0;
      draw_help_screen();
      return true;
    } else {
      help_alt_ = false;
      help_screen_ = false;
      emusdl.draw_overlay_ = false;
      return true;
    }
  }

  if (load_screen_ || save_screen_) {
    if (emusdl.key_pressed(SDLK_DOWN)) {
      if (snap_selected_ < SNAP_SLOTS - 1) {
        ++snap_selected_;
      }
      redraw_snap_screen();
    } else if (emusdl.key_pressed(SDLK_UP)) {
      if (snap_selected_ > 0) {
        --snap_selected_;
      }
      redraw_snap_screen();
    } else if (emusdl.key_pressed(SDLK_RETURN)) {
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
    help_screen_ = true;
    draw_help_screen();
    emusdl.draw_overlay_ = true;
  } else if (emusdl.key_pressed(SDLK_F2)) {
    fullscreen = !fullscreen;
    emusdl.fullscreen(fullscreen);
  } else if (emusdl.key_pressed(SDLK_F3)) {
    quickload_ = !quickload_;
  } else if (emusdl.key_pressed(SDLK_F5)) {
    showmap();
  } /* else if (emusdl.key_pressed(SDLK_F6)) {
    trace_ = true;
  } */
  else if (emusdl.key_pressed(SDLK_F7) && intro_loaded_) {
    save_screen_ = true;
    emusdl.draw_overlay_ = true;
    redraw_snap_screen();
  } else if (emusdl.key_pressed(SDLK_F8) && intro_loaded_) {
    load_screen_ = true;
    emusdl.draw_overlay_ = true;
    redraw_snap_screen();
  } /* else if (emusdl.key_pressed(SDLK_F9)) {
     int x, y;
     cin >> x;
     cin >> y;
     zx48k::writemem(0x6a9e, x, false);
     zx48k::writemem(0x6a9f, y, false);
   } */
  else if (emusdl.key_pressed(SDLK_BACKSPACE)) {
    keystopress_ =
        std::vector<std::vector<uint8_t>>{{(0 << 3 | 0)},
                                          {(0 << 3 | 0)},
                                          {(0 << 3 | 0)},
                                          {(0 << 3 | 0), (4 << 3 | 0)},
                                          {(0 << 3 | 0), (4 << 3 | 0)},
                                          {(0 << 3 | 0), (4 << 3 | 0)},
                                          {(0 << 3 | 0), (4 << 3 | 0)}};
  }
  upmap();
  return true;
}

pandora *emu;
int main(int argc, char **argv) {
  SDL_Init(SDL_INIT_EVERYTHING);
  atexit(SDL_Quit);
  emu = new pandora();
  emu->initialize();
  emu->run();
  return 0;
}
