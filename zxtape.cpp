/*
 * zxtape.cpp
 *
 *  Created on: 25 mar 2016
 *      Author: wpk
 */
#include "zxtape.h"
#include <fstream>
#include <iostream>

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
    struct tapblock tb;
    tb.len = len;
    tb.buf = new char[len];
    file.read(tb.buf, len);
    if (file.fail()) {
      file.close();
      throw;
    }
    this->blocks.push_back(tb);
  }
  file.close();
  reset();
}

void zxtape::reset() {
  state = PAUSE;
  blockstate = PREPILOT;
  block = blocks.begin();
  blockoffs = 0;
  pos = 0;
  i_pos = 0;
  earr = false;
}

void zxtape::go() { state = RUNNING; }

bool zxtape::bit() {
  return block->buf[blockoffs + (pos / 8)] & (1 << (7 - (pos % 8)));
}

void zxtape::flip() { earr = !earr; }
bool zxtape::update_ticks(uint32_t diff) {
  if (state != RUNNING) {
    return state != END;
  }
  // A 'pulse' here is either a mark or a space, so 2 pulses makes a complete
  // square wave cycle. Pilot tone: before each block is a sequence of 8063
  // (header) or 3223 (data) pulses, each of length 2168 T-states. Sync pulses:
  // the pilot tone is followed by two sync pulses of 667 and 735 T-states resp.
  // A '0' bit is encoded as 2 pulses of 855 T-states each.
  // A '1' bit is encoded as 2 pulses of 1710 T-states each (ie. twice the
  // length of a '0')
  i_pos += diff;
  switch (blockstate) {
  case PREPILOT:
    if (i_pos > 10000000) {
      blockstate = PILOT;
      i_pos = 0;
    }
    break;
  case PILOT:
    if (i_pos > 2168) {
      i_pos = 0;
      flip();
      if (++pos > 8000) {
        blockstate = SYNC;
        i_pos = 0;
        pos = 0;
      }
    }
    break;
  case SYNC:
    if (pos == 0) {
      if (i_pos > 667) {
        pos = 1;
        i_pos = 0;
        flip();
      }
    } else {
      if (i_pos > 735) {
        blockstate = DATA;
        tock = false;
        pos = 0;
        i_pos = 0;
        flip();
      }
    }
    break;
  case DATA: {
    uint32_t l = bit() ? 1705 : 850; // 1710 : 855;

    if (i_pos > 2 * l) {
      flip();
      tock = false; // tick
      i_pos = 0;
      if (++pos == 8 * block->len) {
        block++;
        blockoffs = 0;
        pos = 0;
        i_pos = 0;
        blockstate = PREPILOT;
        if (block == blocks.end()) {
          state = END;
        }
      }
    } else if (i_pos > l && !tock) {
      flip();
      tock = true;
    }
  }
  }
  return true;
}
bool const zxtape::ear() { return earr; }

zxtape::~zxtape() {
  for (auto &tb : blocks) {
    delete[] tb.buf;
  }
}
