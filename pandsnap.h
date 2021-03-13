#include "z80.h"
#include <string>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <ctime>
//
// header:
// str 'PANDSNAP' - 8b

// date - 8b
// registers - 504b (padded)
// memory - 48k

#define SNAP_HDR_SIZE 8
#define SNAP_MEMSIZE (48*1024)
#define SNAP_BLOCK_SIZE (512 + SNAP_MEMSIZE)
#define SNAP_SLOTS 10
#define HEAD_ "PANDSNAP"
class Pandsnap {
public:
    Pandsnap(std::string filename);
    std::vector<std::string> List();
    void Save(int slot, z80* cpu, uint8_t* mem);
    void Load(int slot, z80* cpu, uint8_t* mem);
    bool Empty(int slot);
private:
    FILE * datafile_;
    void init(std::string filename);
    std::vector<std::time_t> slots_;
};

        