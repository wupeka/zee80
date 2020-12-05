/*
 * z80test.cpp
 *
 *  Created on: 29 mar 2016
 *      Author: wpk
 */

#include "z80test.h"
#include "z80.h"
#include <iostream>
#include <memory>
using namespace std;
z80test::z80test() {
  string testname;
  cin >> testname;
  z80 *cpu = new z80(*this);
  uint64_t ticks = cpu->readstate(cin);
  while (true) {
    int addr;
    cin >> hex >> addr;
    if (addr == -1)
      break;
    while (true) {
      int m;
      cin >> hex >> m;
      if (m == -1) {
        break;
      }
      memory[addr++] = m;
    }
  }

  uint64_t t = 0;
  std::cerr << cpu->get_trace();
  while (t < ticks) {
    t = cpu->tick();
    std::cerr << cpu->get_trace();
  };
  cpu->writestate(cout);
}

uint32_t z80test::readmem(uint16_t address) {
  return *((uint32_t *)(memory + address));
}
void z80test::writemem(uint16_t address, uint8_t value) {
  memory[address] = value;
}

uint8_t z80test::readio(uint16_t address) { return address >> 8; }
void z80test::writeio(uint16_t address, uint8_t value) {}

z80test::~z80test() {
  // TODO Auto-generated destructor stub
}

int main() { z80test x; }
