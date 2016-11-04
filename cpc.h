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

	virtual uint32_t readmem(uint16_t address) override;
	virtual void writemem(uint16_t address, uint8_t value) override;
	virtual uint8_t readio(uint16_t address) override;
	virtual void writeio(uint16_t address, uint8_t value) override;
	void LoadFloppy(std::string floppy);

	float getLoad() const {
		return load;
	}
	void reset();
private:
	void scanline();
	uint8_t sdlkey2cpc(SDL_Keycode k) const;

	static constexpr int BLOCKS = 8;
	static constexpr int BLOCK_SIZE = 16387;
	static constexpr int LROM_SIZE = 16387;

	z80 cpu;

	std::map<int, std::string> romfiles;
	mutable fdc765 fdc; // TODO: fix the ugly
	float load;

	std::string floppy;

	bool trace = false;
	uint8_t ram[BLOCKS][BLOCK_SIZE]; // 8 blocks of 16384
	uint8_t lrom[LROM_SIZE];
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

	float util = 0;
	bool turbo = false;
	bool joymode = false;
	bool debounce = false;
	uint64_t lastcycles = 0;
	uint64_t v_diff = 0;
	uint64_t d_diff = 0;
	int64_t acc_delay = 0;
	struct timespec tv_s, tv_e;
};

#endif /* ZX48K_H_ */
