/*
 * emusdl.h
 *
 *  Created on: 23 mar 2016
 *      Author: wpk
 */

#ifndef EMUSDL_H_
#define EMUSDL_H_
#include <SDL2/SDL.h>
#include <algorithm>
#include <memory>
#include <set>

class EmuSDL {
public:
  virtual ~EmuSDL();

protected:
  typedef std::set<SDL_Keycode> keys_t;
  EmuSDL(int w, int h, int hscale = 2, int wscale = 2);
  void redrawscreen();
  void readinput();
  inline const keys_t &get_keys() const { return keys; };
  inline int get_width() const { return w; }
  inline int get_height() const { return h; }
  inline bool key_pressed(const keys_t::value_type &value) const {
    return keys.count(value) != 0;
  }
  uint32_t *pixels;

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
