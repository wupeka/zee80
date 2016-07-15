/*
 * emusdl.cpp
 *
 *  Created on: 23 mar 2016
 *      Author: wpk
 */

#include "emusdl.h"
#include <SDL2/SDL.h>

EmuSDL::EmuSDL(int w, int h, int hscale, int wscale) : w(w), h(h), wscale(wscale),hscale(hscale) {
	// TODO Auto-generated constructor stub
	window = SDL_CreateWindow("EmuSDL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, wscale*w, hscale*h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
	renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");  // make the scaled rendering look smoother.
	SDL_RenderSetLogicalSize(renderer, wscale*w, hscale*h);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
				    SDL_TEXTUREACCESS_STREAMING,
				    w, h);
	pixels = (uint32_t*) malloc(w*h*sizeof(uint32_t));
}

EmuSDL::~EmuSDL() {
	// TODO Auto-generated destructor stub
}

void EmuSDL::redrawscreen() {
	SDL_UpdateTexture(texture, NULL, pixels, w * sizeof(uint32_t));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
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
