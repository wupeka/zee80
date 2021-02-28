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
#include <vector>
#include <memory>
#include "z80.h"

class zxtapeblock {
public:
  zxtapeblock(char * data, size_t len);
  ~zxtapeblock();
  void reset();
  bool tick(uint32_t diff);
  bool const ear();
  bool trapload(z80& cpu);
  size_t len() { return len_; };
private:
  zxtapeblock(const zxtapeblock&);
  enum { LEADIN, PILOT, SYNC, DATA, LEADOUT } blockstate_;
  void flip();
  bool bit();
  size_t len_;
  char *buf_;
  uint32_t pos_;
  uint32_t i_pos_;
  bool ear_;
  bool tock_;
};

class zxtape {
public:
  zxtape(std::string filename);
  zxtape(unsigned char *data, unsigned int len);
  void reset();
  void go();
  bool update_ticks(uint32_t diff);
  bool const ear();
  size_t trapload(z80& cpu);
  
private:
  enum { PAUSE, RUNNING, END } state = PAUSE;
  std::vector<std::unique_ptr<zxtapeblock> > blocks;
  std::vector<std::unique_ptr<zxtapeblock> >::iterator block;
};
#endif /* ZXTAPE_H_ */
