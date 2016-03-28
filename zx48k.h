/*
 * zx48k.h
 *
 *  Created on: 20 mar 2016
 *      Author: wpk
 */

#ifndef ZX48K_H_
#define ZX48K_H_
#include <set>
#include <string>
#include "bushandler.h"
#include "emusdl.h"
#include "z80.h"
#include "zxtape.h"

class zx48k : public BusHandler, public EmuSDL {
public:
	zx48k();
	void parse_opts(int argc, char ** argv);
	void initialize();
	void run();

	virtual uint32_t readmem(uint16_t address);
	virtual void writemem(uint16_t address, uint8_t value);
	virtual uint8_t readio(uint16_t address);
	virtual void writeio(uint16_t address, uint8_t value);


private:
	void scanline(int y);
	z80 cpu;
	std::string romfile;
	std::string tapfile;
	zxtape* tape = NULL;

	bool trace = false;
	uint8_t memory[65536]; // simple linear model
	bool ear=0;
	bool mic=0;
	uint8_t border:3;
	bool flashstate=false;
	// the 'keycode' is :
	// 0b00aaakkk
	// aaa -> address line (0-7)
	// kkk -> key line (0-5)
	// 'shift' == 0b00 000 000
	std::set<uint8_t> keyspressed;

	bool turbo = false;
	bool debounce = false;

	constexpr static SDL_Color zxpalette[16] = {
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0xCD},
	{0xCD, 0x00, 0x00},
	{0xCD, 0x00, 0xCD},
	{0x00, 0xCD, 0x00},
	{0x00, 0xCD, 0xCD},
	{0xCD, 0xCD, 0x00},
	{0xCD, 0xCD, 0xCD},
	{0x00, 0x00, 0x00},
	{0x00, 0x00, 0xFF},
	{0xFF, 0x00, 0x00},
	{0xFF, 0x00, 0xFF},
	{0x00, 0xFF, 0x00},
	{0x00, 0xFF, 0xFF},
	{0xFF, 0xFF, 0x00},
	{0xFF, 0xFF, 0xFF}};
};

#endif /* ZX48K_H_ */
