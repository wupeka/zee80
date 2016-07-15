/*
 * cpc.h
 *
 *  Created on: 31 mar 2016
 *      Author: wpk
 */

#ifndef CPC_H_
#define CPC_H_
#include <set>
#include <string>
#include <map>
#include <vector>
#include "bushandler.h"
#include "emusdl.h"
#include "z80.h"
#include "fdc765.h"


class CPC : public BusHandler, public EmuSDL {
public:
	CPC();
	void parse_opts(int argc, char ** argv);
	void initialize();
	void run();

	virtual uint32_t readmem(uint16_t address);
	virtual void writemem(uint16_t address, uint8_t value);
	virtual uint8_t readio(uint16_t address);
	virtual void writeio(uint16_t address, uint8_t value);


private:
	void scanline();
	z80 cpu;
	std::map<int, std::string> romfiles;

	fdc765 fdc;
	std::string floppy;

	bool trace = false;
	uint8_t ram[8][16387]; // 8 blocks of 16384
	uint8_t lrom[16387];
	uint8_t rammap[4] = { 0, 1, 2, 3 };
	std::map<uint8_t, std::vector<uint8_t> > urom;
	uint8_t uromid;

	uint8_t icounter:6;
	bool in_hsync_int;
	uint8_t penr;
	uint32_t pen[17];
	uint8_t vm:2;

	// PSG
	uint8_t psg_reg = 0;

	uint8_t kbd_line = 0;

	uint8_t crtc_reg = 0;
	uint8_t crtcr[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	int line = 0;
	uint8_t vlc = 0;
	uint8_t vcc = 0;

	bool lr = false;
	bool ur = false;




//	uint8_t rom[8][16384]; // same
	// shortcut memory map
	uint8_t memmap[4];
	bool flashstate=false;

	std::set<uint8_t> keyspressed;
	uint8_t sdlkey2cpc(SDL_Keycode k);


	float util = 0;
	bool turbo = false;
	bool joymode = false;
	bool debounce = false;

	uint64_t lastcycles = 0;
	uint64_t v_diff = 0;
	uint64_t d_diff = 0;
	int64_t acc_delay = 0;
	struct timespec tv_s, tv_e;

	constexpr static uint32_t cpcpalette[32] = {
			0x007f7f7f,
			0x007f7f7f,
			0x0000ff7f,
			0x00ffff7f,
			0x0000007f,
			0x00ff007f,
			0x00007f7f,
			0x00ff7f7f,
			0x00ff007f,
			0x00ffff7f,
			0x00ffff00,
			0x00ffffff,
			0x00ff0000,
			0x00ff00ff,
			0x00ff7f00,
			0x00ff7fff,
			0x0000007f,
			0x0000ff7f,
			0x0000ff00,
			0x0000ffff,
			0x00000000,
			0x000000ff,
			0x00007f00,
			0x00007fff,
			0x007f007f,
			0x007fff7f,
			0x007fff00,
			0x007fffff,
			0x007f0000,
			0x007f00ff,
			0x007f7f00,
			0x007f7fff};
};

#endif /* ZX48K_H_ */
