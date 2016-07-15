/*
 * cpc.cpp
 *
 *  Created on: 31 mar 2016
 *      Author: wpk
 */

#include "cpc.h"
#include <strings.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <unistd.h>
#include <boost/program_options.hpp>


using namespace std;
namespace po = boost::program_options;


constexpr uint32_t CPC::cpcpalette[32];

#define LINES 305
#define COLS 760
#define STARTLINE (0)

// BitRePosition
#define BRP(v, b, n) ((((v) >> (b)) & 0x1) << n)
CPC::CPC() :
		EmuSDL(COLS, LINES, hscale=2, wscale=1),
		cpu(*this) {};

void CPC::initialize() {
	for (auto i = 0; i < 8; i++) {
		memset(ram[i], 0xaa, sizeof(ram[i]));
	}
	for (auto romfile: romfiles) {
		std::ifstream fin;
		fin.open (romfile.second.c_str(), std::ios::in | std::ios::binary);
		if (fin.fail()) {
			throw ifstream::failure("Can't open ROM file" + romfile.second);
		}
		fin.seekg(0, std::ios_base::end);
		uint64_t len = fin.tellg();
		fin.seekg(0);
		if (romfile.first == 256) {
			fin.read((char*) lrom, len);
		} else {
			urom[romfile.first] = std::vector<uint8_t>(16387);
			fin.read((char*) urom[romfile.first].data(), len);
		}
		fin.close();
	}
	if (floppy != "") {
		fdc.load(floppy);
	}
	clock_gettime(CLOCK_MONOTONIC, &tv_s);
}

void CPC::parse_opts(int argc, char ** argv) {
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "display this message help message")
		("rom", po::value<string>(), "ROM file to use")
		("floppy", po::value<string>(), "disk in drive")
		("trace", po::bool_switch()->default_value(false), "Enable tracing")
	;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << "\n";
		exit(1);
	}

	if (vm.count("floppy")) {
		floppy = vm["floppy"].as<string>();
	}
//	if (vm.count("rom")) {
//		romfiles = vm["rom"].as<string>();
//	}
	romfiles[0] = "BASIC_1.1.ROM";
	romfiles[7] = "AMSDOS_0.5.ROM";
	romfiles[256] = "CPC6128.ROM";

	if (vm["trace"].as<bool>()) {
		trace = true;
	}


}

uint32_t CPC::readmem(uint16_t address) {
	uint8_t block = address >> 14;
	uint16_t addr = address & 0x3fff;
	if ((block == 0) && !lr) {
		if (trace) cout << "READMEM lr " << hex << addr << "\n";
		return * ((uint32_t*) (lrom + addr));
	} else if ((block == 3) && !ur) {
		if (trace) cout << "READMEM ur " << hex << addr << "\n";
		return * ((uint32_t*) (urom[uromid].data() + addr));
	} else {
		if (trace) cout << "READMEM mem " << hex << addr << " block " << (int)block << " realblock " << (int)rammap[block] << "\n";
		return * ((uint32_t*) (ram[rammap[block]] + addr));
	}
}

void CPC::writemem(uint16_t address, uint8_t v) {
	// writes go straight to RAM, no matter what
	uint8_t block = address >> 14;
	address &= 0x3fff;
	ram[rammap[block]][address] = v;
}

uint8_t CPC::sdlkey2cpc(SDL_Keycode k) {
	switch (k) {
	case SDLK_UP:
		if (!joymode) {
			return 0 << 3 | 0;
		} else {
			return 9 << 3 | 0;
		}
	case SDLK_RIGHT:
		if (!joymode) {
			return 0 << 3 | 1;
		} else {
			return 9 << 3 | 3;
		}
	case SDLK_DOWN:
		if (!joymode) {
			return 0 << 3 | 2;
		} else {
			return 9 << 3 | 1;
		}
	case SDLK_KP_9:
		return 0 << 3 | 3;
	case SDLK_KP_6:
		return 0 << 3 | 4;
	case SDLK_KP_3:
		return 0 << 3 | 5;
	case SDLK_KP_ENTER:
		return 0 << 3 | 6;
	case SDLK_KP_PERIOD:
		return 0 << 3 | 7;

	case SDLK_LEFT:
		if (!joymode)
			return 1 << 3 | 0;
		else {
			return 9 << 3 | 2;
		}
	case SDLK_RCTRL:
		return 1 << 3 | 1;
	case SDLK_KP_7:
		return 1 << 3 | 2;
	case SDLK_KP_8:
		return 1 << 3 | 3;
	case SDLK_KP_5:
		return 1 << 3 | 4;
	case SDLK_KP_1:
		return 1 << 3 | 5;
	case SDLK_KP_2:
		return 1 << 3 | 6;
	case SDLK_KP_0:
		return 1 << 3 | 7;

	case SDLK_RALT:
		return 2 << 3 | 0;
	case SDLK_LEFTBRACKET:
		return 2 << 3 | 1;
	case SDLK_RETURN:
		return 2 << 3 | 2;
	case SDLK_RIGHTBRACKET:
		return 2 << 3 | 3;
	case SDLK_KP_4:
		return 2 << 3 | 4;
	case SDLK_LSHIFT:
		return 2 << 3 | 5;
	case SDLK_BACKSLASH:
		return 2 << 3 | 6;
	case SDLK_LCTRL:
		return 2 << 3 | 7;

	case SDLK_BACKQUOTE:
		return 3 << 3 | 0;
	case SDLK_MINUS:
		return 3 << 3 | 1;
	case SDLK_EQUALS: // @
		return 3 << 3 | 2;
	case SDLK_p:
		return 3 << 3 | 3;
	case SDLK_SEMICOLON:
		return 3 << 3 | 4;
	case SDLK_QUOTE: // :
		return 3 << 3 | 5;
	case SDLK_SLASH:
		return 3 << 3 | 6;
	case SDLK_PERIOD:
		return 3 << 3 | 7;

	case SDLK_0:
		return 4 << 3 | 0;
	case SDLK_9:
		return 4 << 3 | 1;
	case SDLK_o:
		return 4 << 3 | 2;
	case SDLK_i:
		return 4 << 3 | 3;
	case SDLK_l:
		return 4 << 3 | 4;
	case SDLK_k:
		return 4 << 3 | 5;
	case SDLK_m:
		return 4 << 3 | 6;
	case SDLK_COMMA:
		return 4 << 3 | 7;

	case SDLK_8:
		return 5 << 3 | 0;
	case SDLK_7:
		return 5 << 3 | 1;
	case SDLK_u:
		return 5 << 3 | 2;
	case SDLK_y:
		return 5 << 3 | 3;
	case SDLK_h:
		return 5 << 3 | 4;
	case SDLK_j:
		return 5 << 3 | 5;
	case SDLK_n:
		return 5 << 3 | 6;
	case SDLK_SPACE:
		return 5 << 3 | 7;

	case SDLK_6:
		return 6 << 3 | 0;
	case SDLK_5:
		return 6 << 3 | 1;
	case SDLK_r:
		return 6 << 3 | 2;
	case SDLK_t:
		return 6 << 3 | 3;
	case SDLK_g:
		return 6 << 3 | 4;
	case SDLK_f:
		return 6 << 3 | 5;
	case SDLK_b:
		return 6 << 3 | 6;
	case SDLK_v:
		return 6 << 3 | 7;

	case SDLK_4:
		return 7 << 3 | 0;
	case SDLK_3:
		return 7 << 3 | 1;
	case SDLK_e:
		return 7 << 3 | 2;
	case SDLK_w:
		return 7 << 3 | 3;
	case SDLK_s:
		return 7 << 3 | 4;
	case SDLK_d:
		return 7 << 3 | 5;
	case SDLK_c:
		return 7 << 3 | 6;
	case SDLK_x:
		return 7 << 3 | 7;

	case SDLK_1:
		return 8 << 3 | 0;
	case SDLK_2:
		return 8 << 3 | 1;
	case SDLK_ESCAPE:
		return 8 << 3 | 2;
	case SDLK_q:
		return 8 << 3 | 3;
	case SDLK_TAB:
		return 8 << 3 | 4;
	case SDLK_a:
		return 8 << 3 | 5;
	case SDLK_CAPSLOCK:
		return 8 << 3 | 6;
	case SDLK_z:
		return 8 << 3 | 7;

	// TODO joystick!

	case SDLK_RSHIFT:
		return 9 << 3 | 4;

	case SDLK_BACKSPACE:
		return 9 << 3 | 7;


	default:
		return 0xff;
	}
}

void CPC::writeio(uint16_t address, uint8_t v) {
	if (trace) cout << "WRITEIO " << hex <<  address << " " << (int) v << endl;
	switch (address >> 8) {
	case 0x7f: { // PAL/GA
		uint8_t rv = v & 0x3f;
		switch (v >> 6) {
		case 0:
			if (rv & 0x20)
				rv = 0x20;
			penr = rv;
			break;
		case 1:
			pen[penr] = cpcpalette[rv];
			break;
		case 2: { // rmr
			if (rv & 0x10) {
				icounter = 0;
				in_hsync_int = false;
			}
			ur = rv & 0x8;
			lr = rv & 0x4;
			vm = rv & 0x3;
			break;
		}
		case 3: { // mmr
			switch (rv & 0x7) {
			case 0:
				rammap[0] = 0;
				rammap[1] = 1;
				rammap[2] = 2;
				rammap[3] = 3;
				break;
			case 1:
				rammap[0] = 0;
				rammap[1] = 1;
				rammap[2] = 2;
				rammap[3] = 7;
				break;
			case 2:
				rammap[0] = 4;
				rammap[1] = 5;
				rammap[2] = 6;
				rammap[3] = 7;
				break;
			case 3:
				rammap[0] = 0;
				rammap[1] = 3;
				rammap[2] = 2;
				rammap[3] = 7;
				break;
			default: // oldest bit set
				rammap[0] = 0;
				rammap[1] = 4 + (rv & 0x3);
				rammap[2] = 2;
				rammap[3] = 3;
				break;
			}
			}
		}
		break;
	}
	case 0xbc: {
		crtc_reg = v;
		break;
	}
	case 0xbd: {
		if (crtc_reg < 16) {
			crtcr[crtc_reg] = v;
		}
		break;
	}
	case 0xdf: {
		if (urom.find(v) != urom.end()) {
			uromid = v;
		} else { // map to BASIC
			uromid = 0;
		}
		break;
	}
	case 0xf4: { // PPI A - PSG
		// TODO check if port is set to output!
		psg_reg = v;
		break;

	}
	case 0xf5: { // PPI B
		break;

	}
	case 0xf6: { // PPI C
		kbd_line = v & 0xf;
		break;
	}
	case 0xf7: { // PPI control
		break;
	}
	case 0xfb: { // fdc
		if (address == 0xfb7f) {
			fdc.write(v);
		}
		break;
	}
	}

}

uint8_t CPC::readio(uint16_t address) {
	uint8_t out = 0;
	switch (address >> 8) {
	case 0xf4: { // PIO port A - PSG
		if (psg_reg == 0xe) { // ext A
			out = 0xff;
			for (auto sdlkey: keys) {
				uint8_t key = sdlkey2cpc(sdlkey);
				if ((key >> 3) == kbd_line) {
					uint8_t kaddr = 1 << (key & 0b111);
					out &= ~(kaddr); // drop the line pressed
				}

			}
		}
		break;
	}

	case 0xf5: { // PIO port B
		out = 0b00011110;
//		if (vcc >= crtcr[7] && vcc < crtcr[7] + (crtcr[3] >> 4)) {
		if (line < STARTLINE + (crtcr[3] >> 4)) {
			out |= 1;
		}
		break;
	}
	case 0xfb: { // FDC
		if (address == 0xfb7e) { // fdc status register
			out = fdc.read_status();
		} else if (address == 0xfb7f) {
			out = fdc.read();
		}
		break;
	}
	}
	if (trace) cout << "READIO " << hex << address << " " << (int) out << "\n";
	return out;
}

void CPC::scanline() {
	// vcc - vertical char counter
	// hcc - horizontal char counter, not used
	// vlc - vertical line counter
	uint32_t crtc_offset = (((crtcr[12] & 0x3) << 8) + (crtcr[13])) * 2;
	uint8_t crtc_block = (crtcr[12] >> 4) & 0x3;

	// only valid for 8 lines
	// character row + row offset
	uint32_t base = (vlc << 11) + ((crtc_offset + (vcc * 2 * crtcr[1])) & 0x7ff);
	const int sbase = COLS * line;
	int spx = 0;

	if (vcc >= crtcr[6] && line >= 0) {
		for (int i=0; i < COLS; i++) {
			pixels[sbase+i] = pen[16];
		}
	} else if (vcc < crtcr[6] && line >=0) { // display line
		// We have 'COLS' pixels at the screen, we want the content to be in the middle
		// The content is 8 * 2*crtcr[1] pixels wide
		for (; spx < (COLS-(16*crtcr[1]))/2; spx++) {
			pixels[sbase + spx] = pen[16];
		}

		for (int x = 0; x < 2 * crtcr[1]; x++) {
			uint8_t p = ram[crtc_block][(base+x) & 0x3fff];
			// mode 0 - 16 colors, 160 pixels
			// mode 1 - 4 colors -  320 pixels
			// mode 2 - 1 color, 640 pixels
			switch (vm) {
			case 0: {
				for (int i = 1; i >=0; i--) {
					const uint32_t c = pen[BRP(p, i, 3) | BRP(p, 4+i, 2) | BRP(p, 2+i, 1) | BRP(p, 6+i, 0)];
					pixels[sbase + spx++] = c;
					pixels[sbase + spx++] = c;
					pixels[sbase + spx++] = c;
					pixels[sbase + spx++] = c;
				}
				break;
			}
			case 1: {
				for (int i = 3; i >= 0; i--) {
					const uint32_t c = pen[BRP(p, i, 1) | BRP(p, 4+i, 0)];
					pixels[sbase + spx++] = c;
					pixels[sbase + spx++] = c;
				}
				break;
			}
			case 2: {
				for (int i = 7; i >=0; i--) {
					const uint32_t c = pen[BRP(p, i, 0)];
					pixels[sbase + spx++] = c;
				}
				break;
			}
			}
		}
		for (;spx < COLS; spx++) {
			pixels[sbase + spx] = pen[16];
		}
	}

	if (vlc++ == crtcr[9]) {
		vcc++;
		vlc = 0;
	}

	if (trace && line < STARTLINE + (crtcr[3] >> 4)) {
		for (int i = 10; i < 15; i++) {
			pixels[sbase + i] = 0x0050ff50;
		}
	}

	if (vcc == crtcr[7] && vlc == 0) { // VSYNC
		line = STARTLINE;
	} else {
		if (line < LINES-1)
			line++;
	}

	if (vcc == crtcr[4]) { //
		vcc = 0;
		vlc = 0;
	}
	if (trace) {
		cout << "LINE " << line << " vcc " << (int)vcc << " vlc " << (int) vlc << " crtc4 " << (int)crtcr[4] << " crtcr7 " << (int) crtcr[7] << endl;
	}
	bool interrupted = false;
	if (in_hsync_int) {
		if (cpu.interrupt()) { // if acked reset && set bit5 to 0
			if (trace) {
				for (int i = 0; i < 100; i++ ) {
					pixels[sbase+i] = 0x0015c15c;
				}
			}


			in_hsync_int = false;
			icounter &= ~(1<<5);
			interrupted = true;
		}
	}
	if (line == STARTLINE+2) { // second hsync after vsync
		if (icounter >= 32 and not interrupted) {
			cpu.interrupt();
			if (trace) {
				for (int i = 0; i < 100; i++ ) {
					pixels[sbase+i] = 0x00c00fee;
				}

			}
		}
		icounter = 0;
	} else if (++icounter == 52) {
		if (not interrupted) {
			if (trace) {
				for (int i = 0; i < 100; i++ ) {
					pixels[sbase+i] =  0x00fade31;
				}
			}
			if (!cpu.interrupt()) {
				in_hsync_int = true;
			}
		}
		icounter = 0;
	}

}

void CPC::run() {
	if (trace) {
		std::cout << cpu.get_trace();
	}
	uint64_t cycles = cpu.tick();
	uint64_t diff = cycles - lastcycles;
	lastcycles = cycles;

	v_diff += diff;
	d_diff += diff;

	// draw lines that should be drawn now.

	bool redrawn = false;
	while (v_diff > 256) { // 64 microseconds == 256 tics @ 4MHz (1 hsync)
		scanline();
		if (line == 0) {
			if (redrawn)
				continue;
			// process input
			getinput();
			if (keys.count(SDLK_F5)) {
				trace = true;
			}
			if (keys.count(SDLK_F6)) {
				trace = false;
			}

			if (keys.count(SDLK_F8)) {
				if (!debounce) {
					turbo = !turbo;
					debounce = true;
					cout << string("Turbo mode ") << (turbo ? "on\n" : "off\n");
				}
			} else if (keys.count(SDLK_F9)) {
				if (!debounce) {
					joymode = !joymode;
					debounce = true;
					cout << string("Joy mode ") << (joymode ? "on\n" : "off\n");
				}
			} else if (keys.count(SDLK_F10)) {
				if (!debounce) {
					string f;
					getline(cin, f);
					fdc.unload();
					fdc.load(f);
				}
			} else {
				debounce = false;
			}

			if (keys.count(SDLK_F4)) {
				SDL_Quit();
				exit(0);
			}
			redrawscreen();
		} else { redrawn = false; };
		v_diff-=256;
	}


	// correct timing:
	// 'd_diff' cycles should take us d_diff * 250ns, if we're too late - too bad
	// if we're too soon - wait
	// do it only if d_diff > 10000 to save time on unnecessary clock_gettimes
	// TODO move it into emusdl
	if (d_diff > 100000) {
		clock_gettime(CLOCK_MONOTONIC, &tv_e);
		uint64_t real_time = (tv_e.tv_sec - tv_s.tv_sec) * 1000000000 +
				tv_e.tv_nsec - tv_s.tv_nsec;
		uint64_t handl_time = d_diff * 250; // 4MHz == 1/250ns
		d_diff = 0;

		acc_delay += handl_time - real_time;
		util = (handl_time - real_time)/real_time;
		if (acc_delay < -250000000) {
			cout << "Underrun of " << std::dec << acc_delay << " d_diff " << d_diff << " real_time " << real_time << " handl_time " << handl_time << endl;
		}
		if (acc_delay < -250000000 || turbo) {
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

