/*
 * emusdl.h
 *
 *  Created on: 23 mar 2016
 *      Author: wpk
 */

#ifndef EMUSDL_H_
#define EMUSDL_H_
#ifdef __MINGW32__
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <algorithm>
#include <memory>
#include <set>

class EmuSDL {
public:
  virtual ~EmuSDL();

public:
  typedef std::set<SDL_Keycode> keys_t;
  EmuSDL(int w, int h, int hscale = 2, int wscale = 2, const char *name="EmuSDL");
  void settitle(const char* name);
  void redrawscreen();
  void readinput();
  void waitevent();
  void fullscreen(bool fs);
  inline const keys_t &get_keys() const { return keys; };
  void clearkey(SDL_Keycode k) { keys.erase(k); };
  inline int get_width() const { return w; }
  inline int get_height() const { return h; }
  inline bool key_pressed(const keys_t::value_type &value) const {
    return keys.count(value) != 0;
  }
  inline bool keys_pressed() const { return !keys.empty(); }
  uint32_t *pixels;
  uint32_t *overlay_;
  bool draw_overlay_ = false;

private:
  const int w;
  const int h;
  const int hscale;
  const int wscale;
  SDL_Window *window;
  SDL_Renderer *renderer;
  SDL_Texture *texture;
  keys_t keys;
};

#endif /* EMUSDL_H_ */
