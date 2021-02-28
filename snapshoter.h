#include "z80.h"
#include <string>
#include <stdint>
//
// header:
// str 'ZEE80SNP' - 8b
// uint32_t no_slots

// 'slot' + uint32_t length (sans 6b header)
// 'name' (4b) + length (4b)  name       - 512b padded
// 'cpur' (4b) + length (4b) - 512b (padded)
// 'memm' (4b) + length (4b)


class Snapshoter {
public:
    Snapshoter(std::string filename, int slots, z80* cpu, char* mem, size_t mem_len);
    std::vector<std::string> List();
    void Create(std::string name, int slot);
    void Load(int slot);
private:
    struct snapshot {
        size_t foffset;
        std::string name;
        
}

        