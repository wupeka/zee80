/*
 * emusdl.cpp
 *
 *  Created on: 23 mar 2016
 *      Author: wpk
 */

#include "emusdl.h"
#include <SDL/SDL.h>

EmuSDL::EmuSDL(int w, int h,  SDL_Color* palette, int palsize) : w(w), h(h), scale(2) {
	// TODO Auto-generated constructor stub
	SDL_Init( SDL_INIT_EVERYTHING );
	SDL_WM_SetCaption("EmuSDL", "EmuSDL");
	screen = SDL_SetVideoMode( scale*w, scale*h, 8, SDL_SWSURFACE);
	SDL_SetPalette(screen, SDL_LOGPAL | SDL_PHYSPAL, palette, 0, palsize);

	bscreen = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
	SDL_SetPalette(bscreen, SDL_LOGPAL | SDL_PHYSPAL, palette, 0, palsize);
	SDL_LockSurface(bscreen); // unlocked only for flip
}

EmuSDL::~EmuSDL() {
	// TODO Auto-generated destructor stub
}

void EmuSDL::redrawscreen() {
	if (scale == 1) {
		SDL_UnlockSurface(bscreen);
		SDL_BlitSurface(bscreen, NULL, screen, NULL );
		SDL_LockSurface(bscreen);
	} else {
		SDL_LockSurface(screen);
		// not my proudest moment...
		for (int x=0; x < w; x++)
			for (int y=0; y < h; y++)
				for (int i=0; i < scale; i++)
					for (int j=0; j < scale; j++)
						((char*) screen->pixels)[screen->pitch * (scale*y+j) + scale*x + i] =
								((char*) bscreen->pixels)[bscreen->pitch * y + x];

		SDL_UnlockSurface(screen);
	}
	SDL_Flip(screen);
}

void EmuSDL::getinput() {
	SDL_Event event;
	while(SDL_PollEvent(&event)){
		/* We are only worried about SDL_KEYDOWN and SDL_KEYUP events */
		switch(event.type){
		case SDL_KEYDOWN:
			keys.insert(event.key.keysym.sym);
			break;
		case SDL_KEYUP:
			keys.erase(event.key.keysym.sym);
			break;
		default:
			break;
		}
	}

}
