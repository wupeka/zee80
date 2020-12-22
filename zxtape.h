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

class zxtapeblock {
public:
  zxtapeblock(char * data, size_t len);
  ~zxtapeblock();
  void reset();
  bool tick(uint32_t diff);
  bool const ear();
private:
  zxtapeblock(const zxtapeblock&);
  enum { SILENCE, PILOT, SYNC, DATA } blockstate_;
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
  void reset();
  void go();
  bool update_ticks(uint32_t diff);
  bool const ear();

private:
  enum { PAUSE, RUNNING, END } state = PAUSE;
  std::vector<std::unique_ptr<zxtapeblock> > blocks;
  std::vector<std::unique_ptr<zxtapeblock> >::iterator block;
};
#endif /* ZXTAPE_H_ */
