/*
 * emusdl.h
 *
 *  Created on: 23 mar 2016
 *      Author: wpk
 */

#ifndef EMUSDL_H_
#define EMUSDL_H_
#include <SDL/SDL.h>
#include <set>
class EmuSDL {
public:
	virtual
	~EmuSDL();
protected:
	EmuSDL(int w, int h, SDL_Color* palette, int palsize);
	void redrawscreen();
	void getinput();
	int w;
	int h;
	int scale;
	SDL_Surface *bscreen; // 'back' screen, initialized by child
	SDL_Surface *screen; // 'real' screen, initialized by EmuSDL

	std::set<SDLKey> keys;

};

#endif /* EMUSDL_H_ */
