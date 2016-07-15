/*
 * zx48k.cpp
 *
 *  Created on: 20 mar 2016
 *      Author: wpk
 */

#include <strings.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <unistd.h> // TODO remove (usleep)
#include <boost/program_options.hpp>

#include "zx48k.h"

using namespace std;
namespace po = boost::program_options;

#define COLS 352
#define LINES 296

constexpr uint32_t zx48k::zxpalette[16];

zx48k::zx48k() :
		EmuSDL(COLS, LINES, hscale=2, wscale=2),
		cpu(*this),
		romfile("testrom.bin")
		{}

void zx48k::initialize() {
	memset(memory, 0xaa, 65536);
	std::ifstream fin(romfile.c_str(), std::ios::in | std::ios::binary);
	if (fin.fail()) {
		throw ifstream::failure("Can't open ROM file");
	}
	fin.seekg(0, std::ios_base::end);
	uint64_t len = fin.tellg();
	fin.seekg(0);
	fin.read((char*) memory,len);
	fin.close();
	border = 7;
	if (tapfile != "") {
		tape = new zxtape(tapfile);
	}
}

void zx48k::parse_opts(int argc, char ** argv) {
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "display this message help message")
		("rom", po::value<string>(), "ROM file to use")
		("tap", po::value<string>(), "Tape to load")
		("trace", po::bool_switch()->default_value(false), "Enable tracing")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << "\n";
		exit(1);
	}

	if (vm.count("rom")) {
		romfile = vm["rom"].as<string>();
	}
	if (vm.count("tap")) {
		tapfile = vm["tap"].as<string>();
	}
	if (vm["trace"].as<bool>()) {
		trace = true;
	}

}

uint32_t zx48k::readmem(uint16_t address) {
	if (trace) {
		cout << "READMEM " << std::hex << (int) address << " " << (int) (memory[address] & 0xff) << "\n";
	}
	return * ((uint32_t*) (memory + address));
	return memory[address] + (memory[address+1] << 8) + (memory[address+2] << 16) + (memory[address + 3] << 24);
}

void zx48k::writemem(uint16_t address, uint8_t v) {
	if (address >= 0x4000) {
		memory[address] = v;
		if (trace) {
			cout << "WRITEMEM " << std::hex << (int) address << " " << (int) v << "\n";
		}
	}
}

static uint8_t sdlkey2spectrum(SDL_Keycode k) {
	switch (k) {
	case SDLK_LSHIFT:
		return 0 << 3 | 0;
	case SDLK_z:
		return 0 << 3 | 1;
	case SDLK_x:
		return 0 << 3 | 2;
	case SDLK_c:
		return 0 << 3 | 3;
	case SDLK_v:
		return 0 << 3 | 4;
	case SDLK_a:
		return 1 << 3 | 0;
	case SDLK_s:
		return 1 << 3 | 1;
	case SDLK_d:
		return 1 << 3 | 2;
	case SDLK_f:
		return 1 << 3 | 3;
	case SDLK_g:
		return 1 << 3 | 4;
	case SDLK_q:
		return 2 << 3 | 0;
	case SDLK_w:
		return 2 << 3 | 1;
	case SDLK_e:
		return 2 << 3 | 2;
	case SDLK_r:
		return 2 << 3 | 3;
	case SDLK_t:
		return 2 << 3 | 4;
	case SDLK_1:
		return 3 << 3 | 0;
	case SDLK_2:
		return 3 << 3 | 1;
	case SDLK_3:
		return 3 << 3 | 2;
	case SDLK_4:
		return 3 << 3 | 3;
	case SDLK_5:
		return 3 << 3 | 4;
	case SDLK_0:
		return 4 << 3 | 0;
	case SDLK_9:
		return 4 << 3 | 1;
	case SDLK_8:
		return 4 << 3 | 2;
	case SDLK_7:
		return 4 << 3 | 3;
	case SDLK_6:
		return 4 << 3 | 4;
	case SDLK_p:
		return 5 << 3 | 0;
	case SDLK_o:
		return 5 << 3 | 1;
	case SDLK_i:
		return 5 << 3 | 2;
	case SDLK_u:
		return 5 << 3 | 3;
	case SDLK_y:
		return 5 << 3 | 4;
	case SDLK_RETURN:
		return 6 << 3 | 0;
	case SDLK_l:
		return 6 << 3 | 1;
	case SDLK_k:
		return 6 << 3 | 2;
	case SDLK_j:
		return 6 << 3 | 3;
	case SDLK_h:
		return 6 << 3 | 4;
	case SDLK_SPACE:
		return 7 << 3 | 0;
	case SDLK_RSHIFT:
		return 7 << 3 | 1;
	case SDLK_m:
		return 7 << 3 | 2;
	case SDLK_n:
		return 7 << 3 | 3;
	case SDLK_b:
		return 7 << 3 | 4;
	default:
		return 0xff;
	}
}

void zx48k::writeio(uint16_t address, uint8_t v) {
	if ((address & 0x1) == 0) { // ULA, might be & 0xff == 0xfe
		ear = v & 0x10;
		mic = v & 0x08;
		border = v & 0x07;
	} else if ((address & 0x8002) == 0x8000) { //ay
		cout << "WRITEAY" << "\n";
	} else {
//		cout << "WRITEIO " << std::hex << (int) address << " " << (int) v << "\n";
	}
}

uint8_t zx48k::readio(uint16_t address) {
	uint8_t out = 0;
	if ((address & 0xff) == 0xfe) { // keyboard routines
		out = 0xbf;
		uint8_t lines = ~(address >> 8);
		for (auto sdlkey: keys) {
			uint8_t key = sdlkey2spectrum(sdlkey);
			if (key != 0xff) {
				uint8_t kline = 1 << (key >> 3);
				uint8_t kaddr = 1 << (key & 0b111);
				if (kline & lines) {
					out &= ~(kaddr); // drop the line pressed
				}
			}
		}
		if (tape && tape->ear()) {
			out |= 1<<6;
		}

	}
	return out;
}

void zx48k::scanline(int y) {
	if (y >= 296) { // 16 empty scanlines at the end
			return;
	} else if ((y < 48) || (y >= 240) ) {
		// TODO unloopize it
		for (int x=0; x<352; x++) {
			uint32_t pixel = COLS * y + x;
			pixels[pixel] = zxpalette[border];
		}
	} else {
		for (int x = 0; x < 48; x++ ) {
			uint32_t pixel = COLS * y + x;
			pixels[pixel] = zxpalette[border];
		}
		for (int xx = 0; xx < 32; xx++) {
			int yy = y-48;
			// 0 1 0 y7 y6 y2 y1 y0 y5 y4 y3 x4 x3 x2 x1 x0
			uint16_t addr = (1 << 14) | xx |
					(((yy >> 3) & 0x7) << 5) |
					((yy&7) << 8) |
					((yy>>6) << 11);
			uint8_t attrs = memory[0x5800 + xx + 32*(yy/8)];
			uint8_t ink = attrs & 0x7;
			uint8_t paper = (attrs >> 3) & 0x7;
			if (attrs & 0b01000000) { // 'bright'
				ink |= 0x8;
				paper |= 0x8;
			}

			if ((attrs & 0b10000000) && flashstate) {
				ink ^= paper;
				paper ^= ink;
				ink ^= paper;
			}

			for (int i = 0; i < 8; i++) {
				uint32_t pixel = COLS * y + 48 + 8*xx + i;
				bool lit = memory[addr] & (1 << (7-i));
				pixels[pixel] = zxpalette[lit ? ink : paper];
			}
		}
		// TODO unloopize it
		for (int x = 304; x < 352; x++) {
			uint32_t pixel = COLS * y + x;
			pixels[pixel] = zxpalette[border];
		}
	}

}

void zx48k::run() {
	uint64_t lastcycles = 0;
	int line = 0;
	int frame = 0;
	uint64_t v_diff = 0;
	uint64_t d_diff = 0;
	int64_t acc_delay = 0;

	struct timespec tv_s, tv_e;
	clock_gettime(CLOCK_MONOTONIC, &tv_s);
	for (;;) {
		if (trace) {
			std::cout << cpu.get_trace();
		}
		uint64_t cycles = cpu.tick();
		uint64_t diff = cycles - lastcycles;
		lastcycles = cycles;
		if (tape) {
			tape->update_ticks(diff);
		}

		v_diff += diff;
		d_diff += diff;

		// draw lines that should be drawn now.
		while (v_diff > 224) {
			scanline(line++);
			if (line >= 312) {
				line = 0;
				// process input
				getinput();
				if (keys.count(SDLK_F5)) {
					trace = true;
				}
				if (keys.count(SDLK_F6)) {
					trace = false;
				}
				if (keys.count(SDLK_F7) && tape) {
					tape->reset();
					tape->go();
				}
				if (keys.count(SDLK_F8)) {
					if (!debounce) {
						turbo = !turbo;
						debounce = true;
						cout << string("Turbo mode ") << (turbo ? "on\n" : "off\n");
					}
				} else {
					debounce = false;
				}
				if (keys.count(SDLK_F4)) {
					SDL_Quit();
					exit(0);
				}
				cpu.interrupt();
				if (!turbo) {
					redrawscreen();
				}
				if (++frame == 16) {
					flashstate = !flashstate;
					frame = 0;
					if (turbo) {
						redrawscreen();
					}
				}
			}
			v_diff-=224;
		}


		// correct timing:
		// 'd_diff' cycles should take us d_diff * 285ns, if we're too late - too bad
		// if we're too soon - wait
		// do it only if d_diff > 10000 to save time on unnecessary clock_gettimes
		// TODO move it into emusdl
		if (d_diff > 10000) {
			clock_gettime(CLOCK_MONOTONIC, &tv_e);
			uint64_t real_time = (tv_e.tv_sec - tv_s.tv_sec) * 1000000000 +
					tv_e.tv_nsec - tv_s.tv_nsec;
			uint64_t handl_time = d_diff * 285; // 3.5MHz == 1/285ns
			d_diff = 0;

			acc_delay += handl_time - real_time;

			if (acc_delay < 0 || turbo) {
				acc_delay = 0;
			}

			if (acc_delay > 1000000) {
				uint64_t c = acc_delay/1000000;
				acc_delay -= c*1000000;
				SDL_Delay(c);
			}
			clock_gettime(CLOCK_MONOTONIC, &tv_s);
		}
	}
}

int main(int argc, char ** argv) {
	zx48k * emu = new zx48k();
	emu->parse_opts(argc, argv);
	emu->initialize();
	emu->run();
}

