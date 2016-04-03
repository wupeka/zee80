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
class EmuSDL {
public:
	virtual
	~EmuSDL();
protected:
	EmuSDL(int w, int h, int hscale=2, int wscale=2);
	void redrawscreen();
	void getinput();
	int w;
	int h;
	int hscale;
	int wscale;
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	uint32_t *pixels;
	std::set<SDL_Keycode> keys;

};

#endif /* EMUSDL_H_ */
