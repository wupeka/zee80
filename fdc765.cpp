/*
 * fdc765.cpp
 *
 *  Created on: 1 kwi 2016
 *      Author: wpk
 */

#include "fdc765.h"
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
using namespace std;

namespace {
const std::map<uint8_t, uint8_t> cmdlen{
    {0x02, 8}, {0x03, 2}, {0x04, 1}, {0x05, 8}, {0x06, 8},
    {0x07, 1}, {0x08, 0}, {0x09, 8}, {0x0a, 1}, {0x0c, 8},
    {0x0d, 5}, {0x0f, 2}, {0x11, 8}, {0x19, 8}, {0x1d, 8}};
}

#define MSR_BUSY0 (1 << 0)
#define MSR_BUSY1 (1 << 1)
#define MSR_BUSY2 (1 << 2)
#define MSR_BUSY3 (1 << 3)
#define MSR_CB (1 << 4)
#define MSR_EXM (1 << 5)
#define MSR_DIO (1 << 6)
#define MSR_RQM (1 << 7)

#define SR0_US0 (1 << 0)
#define SR0_US1 (1 << 1)
#define SR0_HD (1 << 2)
#define SR0_NR (1 << 3)
#define SR0_EC (1 << 4)
#define SR0_SE (1 << 5)
#define SR0_IC0 (1 << 6)
#define SR0_IC1 (1 << 7)

#define SR1_MA (1 << 0)
#define SR1_NW (1 << 1)
#define SR1_ND (1 << 2)
#define SR1_OR (1 << 4)
#define SR1_DE (1 << 5)
#define SR1_EN (1 << 5)

#define SR2_MD (1 << 0)
#define SR2_BC (1 << 1)
#define SR2_SN (1 << 2)
#define SR2_SH (1 << 3)
#define SR2_WC (1 << 4)
#define SR2_DD (1 << 5)
#define SR2_CM (1 << 6)

#define SR3_US0 (1 << 0)
#define SR3_US1 (1 << 1)
#define SR3_HD (1 << 2)
#define SR3_TS (1 << 3)
#define SR3_T0 (1 << 4)
#define SR3_RY (1 << 5)
#define SR3_WP (1 << 6)
#define SR3_FT (1 << 7)

fdc765::fdc765() {
  unload();
  //	load("sorcery.dsk");
  //	load("6128SP_1.DSK");
  //	load("pyjamara.dsk");
  //	load("bombjack.dsk");
}
void fdc765::unload() {
  msr = 0b10000000;
  tp = 0;
  sr[0] = SR0_NR;
  sr[1] = 0;
  sr[2] = 0;
  sr[3] = 0;
  sectors.clear();
}
void fdc765::load(std::string filename) {
  //	00 - 21	"EXTENDED CPC DSK File\r\nDisk-Info\r\n"	34
  //	22 - 2f	name of creator (utility/emulator)	14
  //	30	number of tracks	1
  //	31	number of sides	1
  //	32 - 33	unused	2
  //	34 - xx	track size table	bunumber of tracks*number of sides
  std::ifstream fin(filename, std::ios::in | std::ios::binary);
  if (fin.fail()) {
    throw ifstream::failure("Can't open disk file '" + filename + "'");
  }
  fin.seekg(0, std::ios_base::end);
  uint64_t len = fin.tellg();
  fin.seekg(0);
  char *buf = new char[len];
  fin.read((char *)buf, len);
  fin.close();
  bool extended;
  if (!memcmp("EXTENDED CPC DSK File\r\nDisk-Info\r\n", buf, 34)) {
    extended = true;
    //	} else if (!memcmp("MV - CPCEMU Disk-File\r\nDisk-Info\r\n", buf, 34)) {
  } else if (!memcmp("MV - CPCEMU", buf, 11)) {
    extended = false;
  } else {
    throw ifstream::failure("Invalid disk format");
  }

  tracks = buf[0x30];
  sides = buf[0x31];
  int tsize = buf[0x32] + (buf[0x33] << 8);
  char *base = buf + 0x100;
  for (int i = 0; i < tracks * sides; i++) {
    if (extended) {
      tsize = buf[0x34 + i] << 8;
    }
    if (tsize) { // TODO store unformatted tracks?
      parse_track(base, tsize, extended);
      base += tsize;
    }
  }
  delete[] buf;
  sr[0] &= ~(SR0_NR);
  sr[3] = SR3_RY;
  if (sides < 2) {
    sr[3] |= SR3_TS;
  }
}

void fdc765::parse_track(char *buf, int size, bool extended) {
  //	00 - 0b	"Track-Info\r\n"	12
  //	0c - 0f	unused	4
  //	10	track number	1
  //	11	side number	1
  //	12 - 13	unused	2
  //	14	sector size	1
  //	15	number of sectors	1
  //	16	GAP#3 length	1
  //	17	filler byte	1
  //	18 - xx	Sector Information List	xx
  uint32_t ssize = 0x80 << buf[0x14];
  uint8_t nsectors = buf[0x15];
  char *sibase = buf + 0x18;
  char *base = buf + 0x100;
  cout << "Parse track " << (int)ssize << " " << (int)nsectors << " EX "
       << extended << endl;
  for (int i = 0; i < nsectors; i++) {
    cout << "Adding sector " << i << endl;
    //		00	track (equivalent to C parameter in NEC765 commands)
    // 1 		01	side (equivalent to H parameter in NEC765 commands)	1
    // 02 sector ID (equivalent to R parameter in NEC765 commands)	1
    // 03	sector
    // size (equivalent to N parameter in NEC765 commands)	1 		04
    // FDC status register 1 (equivalent to NEC765 ST1 status register)	1
    // 05 FDC status register 2 (equivalent to NEC765 ST2 status register)
    // 1 		06 - 07	actual data length in bytes	2
    sector s;
    uint8_t track = sibase[0];
    uint8_t side = sibase[1];
    uint8_t sectorid = sibase[2];
    uint32_t id = (track << 16) | (side << 8) | sectorid;
    s.sz = sibase[3];
    s.sr1 = sibase[4];
    s.sr2 = sibase[5];
    uint32_t asize = extended ? (sibase[6] + (sibase[7] << 8)) : ssize;
    cout << "T " << (int)track << " SI " << (int)side << " SECID "
         << (int)sectorid << " SID " << id << " SZ " << s.sz << " ASZ " << asize
         << endl;
    s.data = vector<uint8_t>(base, base + asize);
    base += asize;
    sectors[id] = s;
    sibase += 8;
  }
}
fdc765::~fdc765() {
  // TODO Auto-generated destructor stub
}

void fdc765::write(uint8_t v) {
  inq.push_back(v);
  uint8_t sc = inq[0] & 0x1f;
  auto p = cmdlen.find(sc);
  cout << "FDC WRITE BYTE " << (int)v << "\n";
  if (p == cmdlen.end()) {
    inq.clear();
    cout << "FDC cmd not found\n";
    return;
  }
  if (p->second + 1 == inq.size()) {
    cout << "FDC calling\n";
    process_cmd();
    inq.clear();
  }
}

uint8_t fdc765::read_status() const { return msr; }

uint8_t fdc765::read() {
  uint8_t out = 0;
  if (msr & 0b01000000) { // fdc -> cpu
    out = outq.front();
    outq.pop();
    if (outq.empty()) {
      msr &= ~(MSR_DIO | MSR_CB | MSR_EXM);
    }
    if (--exec_len == 0) {
      msr &= ~(MSR_EXM);
    }
  }
  cout << "FDC READ BYTE " << (int)out << "\n";
  return out;
}

// Command     Parameters              Exm Result               Description
// 02+MF+SK    HU TR HD ?? SZ NM GP SL <R> S0 S1 S2 TR HD NM SZ read track
// 03          XX YY                    -                       specify spd/dma
// 04          HU                       -  S3                   sense drive
// state 05+MT+MF    HU TR HD SC SZ LS GP SL <W> S0 S1 S2 TR HD LS SZ write
// sector(s) 06+MT+MF+SK HU TR HD SC SZ LS GP SL <R> S0 S1 S2 TR HD LS SZ read
// sector(s) 07          HU                       - recalib.seek TP=0 08 - -  S0
// TP                sense int.state 09+MT+MF    HU TR HD SC SZ LS GP SL <W> S0
// S1 S2 TR HD LS SZ wr deleted sec(s) 0A+MF       HU                       - S0
// S1 S2 TR HD LS SZ read ID 0C+MT+MF+SK HU TR HD SC SZ LS GP SL <R> S0 S1 S2 TR
// HD LS SZ rd deleted sec(s) 0D+MF       HU SZ NM GP FB          <W> S0 S1 S2
// TR HD LS SZ format track 0F          HU TP                    - seek track n
// 11+MT+MF+SK HU TR HD SC SZ LS GP SL <W> S0 S1 S2 TR HD LS SZ scan equal
// 19+MT+MF+SK HU TR HD SC SZ LS GP SL <W> S0 S1 S2 TR HD LS SZ scan low or
// equal 1D+MT+MF+SK HU TR HD SC SZ LS GP SL <W> S0 S1 S2 TR HD LS SZ scan high
// or eq.

void fdc765::process_cmd() {
  uint8_t sc = inq[0] & 0x1f;
  msr |= MSR_CB;
  switch (sc) {
  case 0x02:
    read_track();
    break;
  case 0x03:
    specify_spddma();
    break;
  case 0x04:
    sense_drive_state();
    break;
  case 0x05:
    write_sectors(false);
    break;
  case 0x06:
    read_sectors(false);
    break;
  case 0x07:
    recalib_seek();
    break;
  case 0x08:
    sense_int_state();
    break;
  case 0x09:
    write_sectors(true);
    break;
  case 0x0a:
    read_id();
    break;
  case 0x0c:
    read_sectors(true);
    break;
  case 0x0d:
    format();
    break;
  case 0x0f:
    seek();
    break;
  case 0x11:
    scan(0);
    break;
  case 0x19:
    scan(-1);
    break;
  case 0x1d:
    scan(1);
    break;
  }
}

void fdc765::read_track() {}

void fdc765::specify_spddma() { cout << "FDC SPECIFYSPDDMA\n"; }

void fdc765::sense_drive_state() {
  cout << "FDC SENSEDRIVE\n";
  msr |= 1 << 6; // FDC -> CPU;
  outq.push(sr[3]);
}

void fdc765::write_sectors(bool deleted) { cout << "FDC WRITESECTORS\n"; }

void fdc765::read_sectors(bool deleted) {
  cout << "FDC READ_SECTORS\n";
  // HU TR HD SC SZ LS GP SL
  uint8_t hu = inq[1];
  uint8_t tr = inq[2];
  uint8_t hd = inq[3];
  uint8_t sc = inq[4]; // start sector
  uint8_t sz = inq[5];
  uint32_t ssize = (sz == 0) ? inq[8] : (0x80 << sz);
  uint8_t ls = inq[6]; // last sector
  uint8_t gp = inq[7];
  uint8_t sl = inq[8];

  uint8_t sector = sc;
  exec_len = 0;
  while (sector <= ls) {
    uint32_t id = (tr << 16) | (hd << 8) | sector;
    auto p = sectors.lower_bound(id);
    cout << "FDC found " << p->first << " id " << id << "\n";
    // there's nothing after the sector we're looking for
    sr[1] &= SR1_ND;
    sr[1] &= ~(SR1_MA); // not missed
    sr[0] &= ~(SR0_IC0 | SR0_IC1);
    if (p == sectors.end() || (p->first & 0xffff00) != (id & 0xffff00)) {
      sr[1] |= SR1_ND; // NO DATA
      sr[1] |= SR1_MA; // missed
      sr[0] |= (SR0_IC0);
      // TODO sr[0]
      break;
    }
    // there is something - no data, but not missed
    else if (p->first != id) {
      sr[1] |= SR1_ND;
      sr[0] |= (SR0_IC0);
      ls++;
      break;
    }
    cout << "FDC doing our job sz " << (0x80 << sz) << " datasz "
         << p->second.data.size() << endl;
    sr[1] = p->second.sr1;
    sr[2] = p->second.sr2;
    sz = p->second.sz;
    for (auto x : p->second.data) {
      outq.push(x);
      exec_len++;
    }
    sector++;
  }
  outq.push(sr[0]);
  outq.push(sr[1]);
  outq.push(sr[2]);
  outq.push(tr);
  outq.push(hd);
  outq.push(ls);
  outq.push(sz);
  msr |= MSR_DIO; // FDC -> CPU;
  msr |= MSR_CB;  // busy
  if (exec_len > 0) {
    msr |= MSR_EXM; // exec phase
  }

  //	S0 S1 S2 TR HD NM SZ
  //	66 0   1  0  2  2  2   a ff
  //	   HU TR HD ?? SZ  NM GP SL
}

void fdc765::recalib_seek() {
  cout << "FDC RECALIB_SEEK\n";
  tp = 0;
  sr[0] |= SR0_SE; // seek end
  sr[3] |= SR3_T0;
  sr[0] &= ~(SR0_IC0 | SR0_IC1); // icode OK
}

void fdc765::sense_int_state() {
  cout << "FDC SENSEINT\n";
  msr |= 1 << 6; // FDC -> CPU;
  outq.push(sr[0]);
  outq.push(tp);
  sr[0] &= ~(3 << 6); //
  sr[0] |= (2 << 6);  // unknown or senseint without int
}

void fdc765::read_id() {
  cout << "FDC READID tp " << (int)tp << "\n";
  uint32_t id = (tp << 16);
  auto p = sectors.lower_bound(id);
  cout << "FDC " << p->first << "\n";
  if ((p->first >> 16) == tp) {
    // S0 S1 S2 TR HD LS SZ
    sr[0] &= ~(SR0_IC0 | SR0_IC1); //
    outq.push(sr[0]);
    outq.push(sr[1]);
    outq.push(sr[2]);
    outq.push(tp);
    outq.push((p->first >> 8) & 0xff);
    outq.push((p->first) & 0xff);
    outq.push(p->second.sz);
  }
  msr |= MSR_DIO; // FDC -> CPU;
}

void fdc765::format() {}

void fdc765::seek() {
  cout << "FDC SEEK_TRACK " << (int)inq[2] << "\n";
  tp = inq[2];
  if (tp == 0) {
    sr[3] |= SR3_T0;
  }
  sr[0] |= SR0_SE;               // seek end
  sr[0] &= ~(SR0_IC0 | SR0_IC1); // icode OK
}

void fdc765::scan(int eq) { cout << "FDC SCAN\n"; }
