/*
 * main.cpp
 *
 *  Created on: 20 mar 2016
 *      Author: wpk
 */

#include <SDL/SDL.h>
#include <SDL/SDL_video.h>
#include "zx48k.h"
#include <iostream>
//The images

int main(int argc, char** argv) {
	zx48k * emulator = new zx48k(argv[1]);
	SDL_Surface* s = NULL;
	SDL_Surface* screen = NULL;
	SDL_Color colors[256];
	colors[0] = {0x00, 0x00, 0x00};
	colors[1] = {0x00, 0x00, 0xCD};
	colors[2] = {0xCD, 0x00, 0x00};
	colors[3] = {0xCD, 0x00, 0xCD};
	colors[4] = {0x00, 0xCD, 0x00};
	colors[5] = {0x00, 0xCD, 0xCD};
	colors[6] = {0xCD, 0xCD, 0x00};
	colors[7] = {0xCD, 0xCD, 0xCD};
	colors[8] = {0x00, 0x00, 0x00};
	colors[9] = {0x00, 0x00, 0xFF};
	colors[10] = {0xFF, 0x00, 0x00};
	colors[11] = {0xFF, 0x00, 0xFF};
	colors[12] = {0x00, 0xFF, 0x00};
	colors[13] = {0x00, 0xFF, 0xFF};
	colors[14] = {0xFF, 0xFF, 0x00};
	colors[15] = {0xFF, 0xFF, 0xFF};


	SDL_Init( SDL_INIT_EVERYTHING );

	screen = SDL_SetVideoMode( 256, 192, 8, SDL_SWSURFACE );
	SDL_SetPalette(screen, SDL_LOGPAL | SDL_PHYSPAL, colors, 0, 256);


	s = SDL_CreateRGBSurface(SDL_SWSURFACE, 256, 192, 8, 0, 0, 0, 0);
	SDL_SetPalette(s, SDL_LOGPAL | SDL_PHYSPAL, colors, 0, 256);

	uint64_t lastclock = 0;
	for (;;) {
		uint64_t clock = emulator->tick();
		if (clock > lastclock + 70000) { // every 50Hz
			std::cout << "Tick!" << std::endl;
			lastclock = clock;
			SDL_LockSurface(s);
			for (int x = 0; x < 32; x++) {
				for (int y = 0; y < 192; y++) {
					// 0 1 0 y7 y6 y2 y1 y0 y5 y4 y3 x4 x3 x2 x1 x0
					uint16_t addr = (1 << 14) | x |
							(((y >> 3) & 0x7) << 5) |
							((y&7) << 8) |
							((y>>6) << 11);

					for (int i = 0; i < 8; i++) {
						uint8_t attrs = emulator->memory[0x5800 + x + (y/8)];
						uint8_t ink = attrs & 0xf;
						uint8_t paper = (attrs >> 3) &0xf; // TODO flash
						if (attrs & 0b01000000) {
							ink |= 0x8;
							paper |= 0x8;
						}
						if (emulator->memory[addr] & (1 << (7-i))) {
							((char*) s->pixels)[s->pitch * y + 8*x + i] = ink;
						} else {
							((char*) s->pixels)[s->pitch * y + 8*x + i] = paper;
						}
					}
				}
			}
			SDL_UnlockSurface(s);
			SDL_BlitSurface(s, NULL, screen, NULL );
			SDL_Flip(screen);
			emulator->interrupt();
		}
	}
	SDL_FreeSurface(s);

	//Quit SDL
	SDL_Quit();

}

