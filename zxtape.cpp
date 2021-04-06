/*
 * zxtape.cpp
 *
 *  Created on: 25 mar 2016
 *      Author: wpk
 */
#include "zxtape.h"
#include "z80.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory.h>
#include <memory>


namespace zee80 {
using namespace std;

zxtape::zxtape(string filename) {
  ifstream file;
  file.open(filename.c_str(), std::ios::in | std::ios::binary);
  if (file.fail()) {
    throw ifstream::failure("Can't open TAP file");
  }
  while (!file.eof()) {
    uint16_t len;
    file.read((char *)&len, 2);
    if (file.fail()) {
      break;
    }
    char buf[len];
    file.read(buf, len);
    if (file.fail()) {
      file.close();
      throw;
    }
    this->blocks_.push_back(std::make_unique<zxtapeblock>((char *)buf, len));
  }
  file.close();
  reset();
}

zxtape::zxtape(unsigned char *data, unsigned int len) {
  unsigned int pos = 0;
  while (pos < len) {
    uint16_t blen = *(uint16_t *)(&data[pos]);
    pos += 2;
    assert(pos + blen <= len);
    this->blocks_.push_back(
        std::make_unique<zxtapeblock>((char *)&data[pos], blen));
    pos += blen;
  }
  reset();
}

size_t zxtape::trapload(z80 &cpu) {
  cout << "Traploading " << block_ << "\n";
  if (state_ != PAUSE) {
    return 0;
  }
  if (block_ == blocks_.size()) {
    return 0;
  }
  if (blocks_[block_]->trapload(cpu)) {
    size_t blen = blocks_[block_]->len();
    block_++;
    return blen;
  } else {
    state_ = RUNNING;
    return 0;
  }
}

bool zxtapeblock::trapload(z80 &cpu) {
  struct z80_regs r = cpu.get_regs();
  bool verify = !(r.fp & cpu.fC);
  uint8_t parity = buf_[0];
  if (len_ != r.de + 2 || r.de == 0 || verify || parity != r.ap) {
    return false;
  }
  r.pc = 0x05e2;
  r.a = 0;
  r.ap = 0x01;
  r.fp = 0x45;
  r.bc = 0xb001;

  for (unsigned i = 1; i < len_ - 1; i++) {
    parity ^= buf_[i];
    cpu.bh.writemem(r.ix + i - 1, buf_[i]);
  }
  parity ^= buf_[len_ - 1];
  r.hl = (parity << 8) | buf_[len_ - 1];
  r.ix += r.de;
  r.de = 0;
  r.a = parity;

  // 0000000010307875 PC=05e2 A=00 F=42 F=01000010 I=3f IFF=00 IM=1 BC=b001
  // DE=0000 HL=fffe AF'=0145 BC'=1721 DE'=369b HL'=bebe IX=5cf3 IY=5c3a SP=ff4a
  // (SP)=3f057107 OPC=c9cde705 0000000030869693 PC=05e2 A=00 F=93 F=10010011
  // I=3f IFF=00 IM=1 BC=b07e DE=0000 HL=00fe AF'=7e6d BC'=1721 DE'=369b
  // HL'=bebe IX=5cf3 IY=5c3a SP=ff4a (SP)=3f057107 OPC=c9cde705

  cpu.set_regs(r);
  // it modifies flags, need to do it after set_regs
  cpu.i_sub8(r.a, 1, false);
  return true;
}

zxtapeblock::zxtapeblock(char *data, size_t len) {
  buf_ = new char[len];
  len_ = len;
  memcpy(buf_, data, len);
  reset();
}

void zxtapeblock::reset() {
  blockstate_ = LEADIN;
  pos_ = 0;
  i_pos_ = 0;
  ear_ = false;
}

void zxtape::reset(unsigned int block) {
  state_ = PAUSE;
  block_ = block;
  blocks_[block_]->reset();
}

void zxtape::go(bool singleblock) {
  singleblock_ = singleblock;
  state_ = RUNNING;
}

bool zxtapeblock::bit() { return buf_[(pos_ / 8)] & (1 << (7 - (pos_ % 8))); }

void zxtapeblock::flip() { ear_ = !ear_; }

bool zxtape::update_ticks(uint32_t diff) {
  if (block_ == blocks_.size()) {
    state_ = END;
  }
  if (state_ != RUNNING) {
    return state_ != END;
  }
  if (blocks_[block_]->tick(diff)) {
    block_++;
    if (block_ == blocks_.size()) {
      state_ = END;
      return false;
    } else {
      blocks_[block_]->reset();
      if (singleblock_) {
        state_ = PAUSE;
      }
    }
  }
  return true;
}
bool zxtapeblock::tick(uint32_t diff) {
  // A 'pulse' here is either a mark or a space, so 2 pulses makes a complete
  // square wave cycle. Pilot tone: before each block is a sequence of 8063
  // (header) or 3223 (data) pulses, each of length 2168 T-states. Sync pulses:
  // the pilot tone is followed by two sync pulses of 667 and 735 T-states resp.
  // A '0' bit is encoded as 2 pulses of 855 T-states each.
  // A '1' bit is encoded as 2 pulses of 1710 T-states each (ie. twice the
  // length of a '0')
  i_pos_ += diff;
  switch (blockstate_) {
  case LEADIN:
    if (i_pos_ > 4000000) { // ~1.15s
      blockstate_ = PILOT;
      i_pos_ = 0;
    }
    break;

  case PILOT:
    if (i_pos_ > 2168) {
      i_pos_ = 0;
      flip();
      if (++pos_ > 8000) {
        blockstate_ = SYNC;
        i_pos_ = 0;
        pos_ = 0;
      }
    }
    break;

  case SYNC:
    if (pos_ == 0) {
      if (i_pos_ > 667) {
        pos_ = 1;
        i_pos_ = 0;
        flip();
      }
    } else {
      if (i_pos_ > 735) {
        blockstate_ = DATA;
        tock_ = false;
        pos_ = 0;
        i_pos_ = 0;
        flip();
      }
    }
    break;

  case DATA: {
    uint32_t l = bit() ? 1710 : 855; // 1710 : 855;

    if (i_pos_ > 2 * l) {
      flip();
      tock_ = false; // tick
      i_pos_ = 0;
      if (++pos_ == 8 * len_) {
        blockstate_ = LEADOUT;
        pos_ = 0;
        i_pos_ = 0;
      }
    } else if (i_pos_ > l && !tock_) {
      flip();
      tock_ = true;
    }
  }
  case LEADOUT:
    if (i_pos_ > 2000000) { // ~0.5s
      return true;
    }
    break;
  }

  return false;
}

bool const zxtape::ear() {
  return (block_ < blocks_.size()) ? blocks_[block_]->ear() : false;
}

bool const zxtapeblock::ear() { return ear_; }

zxtapeblock::~zxtapeblock() { delete[] buf_; }

}
