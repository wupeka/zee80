/*
 * emusdl.h
 *
 *  Created on: 23 mar 2016
 *      Author: wpk
 */

#ifndef EMUSDL_H_
#define EMUSDL_H_
#include <SDL2/SDL.h>
#include <set>
#include <algorithm>
#include <memory>

class EmuSDL {
public:
	virtual
	~EmuSDL();
protected:
	typedef std::set<SDL_Keycode> keys_t;
	EmuSDL(size_t w, size_t h, size_t hscale=2, size_t wscale=2);
	void redrawscreen();
	void readinput();
	inline const keys_t& get_keys() const { return keys; };
	inline void set_pixel(size_t x, size_t y, uint32_t value) { *(pixels.get() + y * w + x) = value; }
	inline void set_line(size_t y, uint32_t value) { std::fill_n(pixels.get() + y * w, w, value); }
	void draw_line(size_t x0, size_t x1, size_t y, uint32_t value);
	inline size_t get_width() const { return w; }
	inline size_t get_height() const { return h; }
	inline bool key_pressed(const keys_t::value_type& value) const { return keys.count(value) != 0; }

private:
	const size_t w;
	const size_t h;
	const size_t hscale;
	const size_t wscale;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	std::unique_ptr<uint32_t[]> pixels;
	keys_t keys;
};

#endif /* EMUSDL_H_ */
