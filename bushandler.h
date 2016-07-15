/*
 * bushandler.h
 *
 *  Created on: 19 mar 2016
 *      Author: wpk
 */

#ifndef BUSHANDLER_H_
#define BUSHANDLER_H_
#include <stdint.h>

class BusHandler {
public:
	virtual ~BusHandler();
	virtual uint32_t readmem(uint16_t address) const = 0;
	virtual void writemem(uint16_t address, uint8_t value) = 0;
	virtual uint8_t readio(uint16_t address) const = 0;
	virtual void writeio(uint16_t address, uint8_t value) = 0;
};

#endif /* BUSHANDLER_H_ */
