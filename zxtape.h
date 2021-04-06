/*
 * zxtape.h
 *
 *  Created on: 25 mar 2016
 *      Author: wpk
 */

#ifndef ZXTAPE_H_
#define ZXTAPE_H_
#include "z80.h"
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace zee80 {

class zxtapeblock {
public:
  zxtapeblock(char *data, size_t len);
  ~zxtapeblock();
  void reset();
  bool tick(uint32_t diff);
  bool const ear();
  bool trapload(z80 &cpu);
  size_t len() { return len_; };

private:
  zxtapeblock(const zxtapeblock &);
  enum { LEADIN, PILOT, SYNC, DATA, LEADOUT } blockstate_;
  void flip();
  bool bit();
  size_t len_ = 0;
  char *buf_ = NULL;
  uint32_t pos_ = 0;
  uint32_t i_pos_ = 0;
  bool ear_ = false;
  bool tock_ = false;
};

class zxtape {
public:
  zxtape(std::string filename);
  zxtape(unsigned char *data, unsigned int len);
  void go(bool singleblock = false);
  bool update_ticks(uint32_t diff);
  bool const ear();
  size_t trapload(z80 &cpu);
  void reset(unsigned int block = 0);

private:
  enum { PAUSE, RUNNING, END } state_ = PAUSE;
  std::vector<std::unique_ptr<zxtapeblock>> blocks_;
  unsigned block_ = 0;
  bool singleblock_ = false;
};
}

#endif /* ZXTAPE_H_ */
