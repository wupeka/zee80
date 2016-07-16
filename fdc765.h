/*
 * fdc765.h
 *
 *  Created on: 1 kwi 2016
 *      Author: wpk
 */

#ifndef FDC765_H_
#define FDC765_H_

#include <map>
#include <vector>
#include <queue>
#include <cstdint>

class sector {
public:
	uint8_t sz;
	uint8_t sr1;
	uint8_t sr2;
	std::vector<uint8_t> data;
};

class fdc765 {
public:
	fdc765();
	virtual
	~fdc765();
	void write(uint8_t v);
	uint8_t read();
	uint8_t read_status() const;
	void load(std::string filename);
	void unload();

private:
	void process_cmd();
	void read_track();
	void specify_spddma();
	void sense_drive_state();
	void write_sectors(bool deleted);
	void read_sectors(bool deleted);
	void recalib_seek();
	void sense_int_state();
	void read_id();
	void format();
	void seek();
	void scan(int eq);

	void parse_track(char* buf, int size, bool extended);
	uint8_t msr; // main status register
	uint8_t sr[4]; // status registers
	int tp; // track physical
	int exec_len; // lenght of exec tada in outq
	std::vector<uint8_t> inq; // command
	std::queue<uint8_t> outq; // response (data + response)

	uint8_t tracks;
	uint8_t sides;
	// sectors are indexed by (track << 16) | (side << 8) | (sectorid)
	std::map<uint32_t, sector> sectors;
};

#endif /* FDC765_H_ */
