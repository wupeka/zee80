#include "pandsnap.h"
#include <ctime>

Pandsnap::Pandsnap(std::string filename) {
    datafile_ = fopen(filename.c_str(), "r+");
    if (datafile_ == NULL) {
        throw;
    }
    fseek(datafile_, 0, SEEK_END);
    int length = ftell(datafile_);
    fseek(datafile_, 0, SEEK_SET);
    if (length < 1) {
        init(filename);
    } else if (length != SNAP_HDR_SIZE + SNAP_SLOTS * SNAP_BLOCK_SIZE) {
        throw;
    }
    char head[8];
    fread(&head, 8, 1, datafile_);
    if (memcmp(head, HEAD_, 8)) {
        throw;
    }
    for (int i=0; i< SNAP_SLOTS; i++) {
        fseek(datafile_, SNAP_HDR_SIZE + i*SNAP_BLOCK_SIZE, SEEK_SET);
        uint64_t tm;
        fread(&tm, 8, 1, datafile_);
        slots_.push_back(tm);
    }
}

std::vector<std::string> Pandsnap::List() {
    std::vector<std::string> rv;
    for (auto slot: slots_) {
        if (slot != 0) {
            char buf[512];
            std::tm* tm = std::localtime(&slot);
            std::strftime(buf, 512, "%y-%m-%d %H:%M", tm);
            rv.push_back(buf);
        } else {
            rv.push_back("<WOLNY>");
        }
    }
    return rv;
}

void Pandsnap::init(std::string filename) {
    fwrite(HEAD_, 8, 1, datafile_);
    char zeros[SNAP_BLOCK_SIZE] = {0};
    for (int i=0; i< SNAP_SLOTS; i++) {
        fwrite(zeros, SNAP_BLOCK_SIZE, 1, datafile_);
        slots_.push_back(0);
    }
    fseek(datafile_, 0, SEEK_SET);
}

void Pandsnap::Save(int slot, z80* cpu, uint8_t* mem) {
    fseek(datafile_, SNAP_HDR_SIZE + slot * SNAP_BLOCK_SIZE, SEEK_SET);
    std::time_t tm = std::time(nullptr);
    fwrite(&tm, 8, 1, datafile_);
    struct z80_regs R = cpu->get_regs();
    fwrite(&R, sizeof(R), 1, datafile_);
    fseek(datafile_, SNAP_HDR_SIZE + slot * SNAP_BLOCK_SIZE + 512, SEEK_SET);
    fwrite((char*) mem, SNAP_MEMSIZE, 1, datafile_);
    fflush(datafile_);
}

void Pandsnap::Load(int slot, z80* cpu, uint8_t* mem) {
    fseek(datafile_, SNAP_HDR_SIZE + slot * SNAP_BLOCK_SIZE, SEEK_SET);
    std::time_t tm;
    fread(&tm, 8, 1, datafile_);
    struct z80_regs R;
    fread(&R, sizeof(R), 1, datafile_);
    cpu->set_regs(R);
    fseek(datafile_, SNAP_HDR_SIZE + slot * SNAP_BLOCK_SIZE + 512, SEEK_SET);
    fread(mem, SNAP_MEMSIZE, 1, datafile_);
    fflush(datafile_);
}
    