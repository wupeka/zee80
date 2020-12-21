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
struct tapblock {
  size_t len;
  char *buf;
};

class zxtape {
public:
  zxtape(std::string filename);
  ~zxtape();
  void reset();
  void go();
  bool update_ticks(uint32_t diff);
  bool const ear();

private:
  bool bit();
  void flip();
  std::ifstream file;
  enum { PAUSE, RUNNING, END } state = PAUSE;
  enum { PREPILOT, PILOT, SYNC, DATA } blockstate = PILOT;
  std::vector<tapblock> blocks;
  std::vector<tapblock>::iterator block;
  uint32_t blockoffs = 0;
  uint32_t pos = 0;
  uint32_t i_pos = 0;
  bool earr;
  bool tock;
};
#endif /* ZXTAPE_H_ */
