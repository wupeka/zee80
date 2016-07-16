/*
 * z80test.h
 *
 *  Created on: 29 mar 2016
 *      Author: wpk
 */

#ifndef Z80TEST_H_
#define Z80TEST_H_

#include "bushandler.h"

class z80test : public BusHandler {
public:
	z80test();
	virtual
	~z80test();
	char memory[65536];

	virtual uint32_t readmem(uint16_t address) override;
	virtual void writemem(uint16_t address, uint8_t value) override;
	virtual uint8_t readio(uint16_t address) override;
	virtual void writeio(uint16_t address, uint8_t value) override;
};

#endif /* Z80TEST_H_ */
