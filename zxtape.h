/*
 * zxtape.h
 *
 *  Created on: 25 mar 2016
 *      Author: wpk
 */

#ifndef ZXTAPE_H_
#define ZXTAPE_H_
#include <fstream>
#include <string>
class zxtape {
public:
	zxtape(std::string filename);
	~zxtape();
	void reset();
	void go();
	void update_ticks(uint32_t diff);
	bool const ear();
private:
	bool bit();
	void flip();
	void newblock();
	std::ifstream file;
	char* buf;
	uint32_t len;
	enum { PAUSE, RUNNING, END } state = PAUSE;
	enum { PILOT, SYNC, DATA } blockstate = PILOT;
	uint32_t blockno = 0;
	uint32_t blocklen = 0;
	uint32_t blockoffs = 0;
	uint32_t pos = 0;
	uint32_t i_pos = 0;
	bool earr;
	bool tock;
};
#endif /* ZXTAPE_H_ */
