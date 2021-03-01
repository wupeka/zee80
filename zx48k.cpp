/*
 * zx48k.cpp
 *
 *  Created on: 20 mar 2016
 *      Author: wpk
 */

#include "zx48k.h"
#include <Ym2149Ex.h>
#include <YmProfiles.h>
//#include <boost/program_options.hpp>
#include <cassert>
#include <ctime>
#include <fstream>
#include <iostream>
#include <strings.h>
#include <chrono>

using namespace std;
//namespace po = boost::program_options;

constexpr int zx48k::MEMORY_SIZE;

namespace {
// Those are 'proper' values for ZX screen, we trim the borders a bit to make it nicer
//constexpr int COLS = 352;
//constexpr int LINES = 296;
//constexpr int TOP_SKIP_LINES = 0;
//constexpr int TOP_BORDER_LINES = 48;
//constexpr int BOTTOM_BORDER_LINES = 56;
//constexpr int LEFT_BORDER_COLS = 48;
//constexpr int RIGHT_BORDER_COLS = 48;
constexpr int COLS = 288;
constexpr int LINES = 224;
constexpr int TOP_SKIP_LINES = 32;
constexpr int TOP_BORDER_LINES = 16;
constexpr int BOTTOM_BORDER_LINES = 16;
constexpr int LEFT_BORDER_COLS = 16;
constexpr int RIGHT_BORDER_COLS = 16;
constexpr uint32_t zxpalette[16] = {
    0x00000000, 0x000000CD, 0x00CD0000, 0x00CD00CD, 0x0000CD00, 0x0000CDCD,
    0x00CDCD00, 0x00CDCDCD, 0x00000000, 0x000000FF, 0x00FF0000, 0x00FF00FF,
    0x0000FF00, 0x0000FFFF, 0x00FFFF00, 0x00FFFFFF};
} // namespace

zx48k::zx48k()
    : emusdl(EmuSDL(COLS, LINES, 2, 2, "zx48k")), cpu(*this),
      ay(new CYm2149Ex(profileSpectrum, 44100)) {
  SDL_AudioSpec want, have;
  SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
  want.freq = 44100;
  want.format = AUDIO_S16;
  want.channels = 2;
  want.samples = INT_AUDIO_BUF_SIZE;
  sdldev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
  if (sdldev == 0) {
    printf("Error opening audio device %s\n", SDL_GetError());
  }
  if (want.format != have.format) {
    printf("Different aoudio format output\n");
  }
  // To fill the buffer we'll unpause it after first frame.
  SDL_PauseAudioDevice(sdldev, 1);
  lastAyWrite = std::chrono::steady_clock::now();

  // Tape load traps
}

void zx48k::initialize() {
  //  memset(memory_, 0xff, MEMORY_SIZE);
  romfile = "testrom.bin";
  std::ifstream fin(romfile, std::ios::in | std::ios::binary);
  if (fin.fail()) {
    throw ifstream::failure("Can't open ROM file");
  }
  fin.seekg(0, std::ios_base::end);
  uint64_t len = fin.tellg();
  fin.seekg(0);
  fin.read((char *)memory_, len);
  fin.close();
  border = 7;
  if (tapfile != "") {
    tape_ = new zxtape(tapfile);
  }
  if (trap_) {
    cpu.addtrap(0x056c);
  }
  if (auto_) {
    cpu.addtrap(0x15e1);
  }
}

static void help() {
  exit(1);
}

void zx48k::parse_opts(int argc, char **argv) {
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--help")) {
      help();
    }
    if (!strcmp(argv[i], "--rom")) {
      i++;
      if (i == argc) {
        help();
      }
      romfile = argv[i];
    } else if (!strcmp(argv[i], "--tap")) {
      i++;
      if (i == argc) {
        help();
      }
      tapfile = argv[i];
    } else if (!strcmp(argv[i], "--trace")) {
      trace_ = true;
    } else if (!strcmp(argv[i], "--trap")) {
      trap_ = true;
    } else if (!strcmp(argv[i], "--auto")) {
      auto_ = true;
    } else if (!strcmp(argv[i], "--fs")) {
      fs_ = true;
    }
  }
}
      
uint32_t zx48k::readmem(uint16_t address, bool dotrace) {
  if ((trace_ && dotrace)) {
    cout << "READMEM " << std::hex << (int)address << " "
         << (int)(memory_[address] & 0xff) << endl;
  }
  return *((uint32_t *)(memory_ + address));
  return memory_[address] + (memory_[address + 1] << 8) +
         (memory_[address + 2] << 16) + (memory_[address + 3] << 24);
}

void zx48k::writemem(uint16_t address, uint8_t v, bool dotrace) {
  if (address >= 0x4000) {
    memory_[address] = v;
    if (trace_ && dotrace) {
      cout << "WRITEMEM " << std::hex << (int)address << " " << (int)v << endl;
    }
  }
}

static uint8_t sdlkey2spectrum(SDL_Keycode k) {
  switch (k) {
  case SDLK_LSHIFT:
    return 0 << 3 | 0;
  case SDLK_z:
    return 0 << 3 | 1;
  case SDLK_x:
    return 0 << 3 | 2;
  case SDLK_c:
    return 0 << 3 | 3;
  case SDLK_v:
    return 0 << 3 | 4;
  case SDLK_a:
    return 1 << 3 | 0;
  case SDLK_s:
    return 1 << 3 | 1;
  case SDLK_d:
    return 1 << 3 | 2;
  case SDLK_f:
    return 1 << 3 | 3;
  case SDLK_g:
    return 1 << 3 | 4;
  case SDLK_q:
    return 2 << 3 | 0;
  case SDLK_w:
    return 2 << 3 | 1;
  case SDLK_e:
    return 2 << 3 | 2;
  case SDLK_r:
    return 2 << 3 | 3;
  case SDLK_t:
    return 2 << 3 | 4;
  case SDLK_1:
    return 3 << 3 | 0;
  case SDLK_2:
    return 3 << 3 | 1;
  case SDLK_3:
    return 3 << 3 | 2;
  case SDLK_4:
    return 3 << 3 | 3;
  case SDLK_5:
    return 3 << 3 | 4;
  case SDLK_0:
    return 4 << 3 | 0;
  case SDLK_9:
    return 4 << 3 | 1;
  case SDLK_8:
    return 4 << 3 | 2;
  case SDLK_7:
    return 4 << 3 | 3;
  case SDLK_6:
    return 4 << 3 | 4;
  case SDLK_p:
    return 5 << 3 | 0;
  case SDLK_o:
    return 5 << 3 | 1;
  case SDLK_i:
    return 5 << 3 | 2;
  case SDLK_u:
    return 5 << 3 | 3;
  case SDLK_y:
    return 5 << 3 | 4;
  case SDLK_RETURN:
    return 6 << 3 | 0;
  case SDLK_l:
    return 6 << 3 | 1;
  case SDLK_k:
    return 6 << 3 | 2;
  case SDLK_j:
    return 6 << 3 | 3;
  case SDLK_h:
    return 6 << 3 | 4;
  case SDLK_SPACE:
    return 7 << 3 | 0;
  case SDLK_RALT:
    return 7 << 3 | 1;
  case SDLK_m:
    return 7 << 3 | 2;
  case SDLK_n:
    return 7 << 3 | 3;
  case SDLK_b:
    return 7 << 3 | 4;
    
//  case SDLK_BACKSPACE: // shift + 0
//    return (0 << 3 | 0) | (;

  default:
    return 0xff;
  }
}
#define NSEC_PER_SEC 1000000000
bool zx48k::processAudio() {
  std::chrono::steady_clock::time_point tv_e = std::chrono::steady_clock::now();
  uint64_t proc_time = std::chrono::nanoseconds(tv_e - lastAyWrite).count();
  uint64_t samples = (proc_time * 44100) / NSEC_PER_SEC;
  if (samples > 0) {
    if (samples > INT_AUDIO_BUF_SIZE - abuf_pos_) {
      samples = INT_AUDIO_BUF_SIZE - abuf_pos_;
    }
    uint64_t proc_ns = (samples * NSEC_PER_SEC) / 44100;
    lastAyWrite += std::chrono::nanoseconds(proc_ns);
    ay->updateStereo(&abuf_[2*abuf_pos_], samples);
    
    abuf_pos_ += samples;
    if (abuf_pos_ == INT_AUDIO_BUF_SIZE) {
      int i;
      double scale = 1;
      int move = aearbufpos_;
      if (aearbufpos_ < INT_AUDIO_BUF_SIZE) {
        move = aearbufpos_;
        scale = 1.0 * aearbufpos_ / INT_AUDIO_BUF_SIZE;
      }
      for (i = 0; i < INT_AUDIO_BUF_SIZE; i++) {
          
        // We fill in the blanks with the last value
//        if ((i < aearbufpos_ ? aearbuf_[i] : aearbuf_[aearbufpos_-1])) {
        if (aearbuf_[(int)(scale * i)]) {
          abuf_[2*i] += 0x3fff;
          abuf_[2*i+1] += 0x3fff;
        }
      }
//      if (aearbufpos_ < INT_AUDIO_BUF_SIZE) {
//        cout << aearbufpos_ << "\n";
//      }
      memmove(&aearbuf_[0], &aearbuf_[move], EARBUFOFFSET*INT_AUDIO_BUF_SIZE+EARBUFRESERVE - move);
      aearbufpos_ = 0;
      i = SDL_QueueAudio(sdldev, abuf_, 2 * sizeof(ymsample) * INT_AUDIO_BUF_SIZE);
      if (i != 0) {
        cout << "queue audio error ";
        cout << SDL_GetError();
      }
      abuf_pos_ = 0;
    }
    return true;
  }

  return false;
}

void zx48k::writeio(uint16_t address, uint8_t v) {
  if ((address & 0x1) == 0) { // ULA, might be & 0xff == 0xfe
    bool n_ear = v & 0x10;
    bool n_mic = v & 0x08;
    if (ear != n_ear) {
//      uint64_t proc_time = cpucycles_ - lastEarChange;
//      cout << "es change " << ear << " " << proc_time << " " << earStates.size() << endl;
//      earStates.push_back(std::make_pair(ear, (proc_time * 285)));
//      lastEarChange = cpucycles_;
    }
    ear = n_ear;
    mic = n_mic;
    border = v & 0x07;
  } else if (address == 0xfffd) {
    // AY select port
    ayport = v;
  } else if (address == 0xbffd) {
    processAudio();
    ay->writeRegister(ayport, v);
  } else {
    cout << "WRITEIO to unknown port " << std::hex << (int)address << " "
         << (int)v << "\n";
  }
}

uint8_t zx48k::readio(uint16_t address) {
  uint8_t out = 0;
  if ((address & 0xff) == 0xfe) { // keyboard routines
    out = 0xbf;
    uint8_t lines = ~(address >> 8);
    if (!keystopress_.empty()) {
      if (keypressedtime_ != 0 && keypressedtime_ + keypresstime_ < cpucycles_) {
        cout << keypressedtime_ << " " << keypresstime_ << " " << cpucycles_ << "\n";
        keystopress_.erase(keystopress_.begin());
        keypressedtime_ = 0;
        // continue, wait for the next round
      } else {
        if (keypressedtime_ == 0) {
          keypressedtime_ = cpucycles_;
        }
        auto keys = keystopress_.front();
        for (uint8_t key: keys) {
          std::cout << (int)key << "\n";
          uint8_t kline = 1 << (key >> 3);
          uint8_t kaddr = 1 << (key & 0b111);
          if (kline & lines) {
            out &= ~(kaddr);
          }
        }
      }
    }
    
    for (auto sdlkey : emusdl.get_keys()) {
      uint8_t key = sdlkey2spectrum(sdlkey);
      if (key != 0xff) {
        uint8_t kline = 1 << (key >> 3);
        uint8_t kaddr = 1 << (key & 0b111);
        if (kline & lines) {
          out &= ~(kaddr); // drop the line pressed
        }
      }
    }
    if (tape_) {
      bool n_ear;
      if (tape_->ear()) {
        out |= 1 << 6;
        n_ear = true;
      } else {
        n_ear = false;
      }
      if (ear != n_ear) {
        ear = n_ear;
//        uint64_t proc_time = cpucycles_ - lastEarChange;
//       earStates.push_back(std::make_pair(n_ear, (proc_time * 285)));
//        lastEarChange = cpucycles_;
        
      }
    }

  } else if (address == 0xfffd) {
    out = ay->readRegister(ayport);
  } else {
    cout << "READIO from unknown port " << std::hex << (int)address << "\n";
  }
  return out;
}

void zx48k::scanline(int y) {
  if (y < TOP_SKIP_LINES || y >= LINES + TOP_SKIP_LINES) { // 16 empty scanlines at the end
    return;
  }
  y -= TOP_SKIP_LINES;

  if ((y < TOP_BORDER_LINES) || (y >= LINES - BOTTOM_BORDER_LINES)) {
    // TODO unloopize it
    for (int x = 0; x < COLS; x++) {
      uint32_t pixel = COLS * y + x;
      emusdl.pixels[pixel] = zxpalette[border];
    }
    return;
  }

  for (int x = 0; x < LEFT_BORDER_COLS; x++) {
    uint32_t pixel = COLS * y + x;
    emusdl.pixels[pixel] = zxpalette[border];
  }

  for (int xx = 0; xx < 32; xx++) { // 32 color boxes
    int yy = y - TOP_BORDER_LINES;
    // 0 1 0 y7 y6 y2 y1 y0 y5 y4 y3 x4 x3 x2 x1 x0
    uint16_t addr = (1 << 14) | xx | (((yy >> 3) & 0x7) << 5) |
                    ((yy & 7) << 8) | ((yy >> 6) << 11);
    uint8_t attrs = memory_[0x5800 + xx + 32 * (yy / 8)];
    uint8_t ink = attrs & 0x7;
    uint8_t paper = (attrs >> 3) & 0x7;
    if (attrs & 0b01000000) { // 'bright'
      ink |= 0x8;
      paper |= 0x8;
    }

    if ((attrs & 0b10000000) && flashstate) {
      ink ^= paper;
      paper ^= ink;
      ink ^= paper;
    }

    for (int i = 0; i < 8; i++) { // 8 pixels in each box.
      uint32_t pixel = COLS * y + LEFT_BORDER_COLS + 8 * xx + i;
      bool lit = memory_[addr] & (1 << (7 - i));
      emusdl.pixels[pixel] = zxpalette[lit ? ink : paper];
    }
  }

  // TODO unloopize it
  for (int x = COLS - RIGHT_BORDER_COLS; x < COLS; x++) {
    uint32_t pixel = COLS * y + x;
    emusdl.pixels[pixel] = zxpalette[border];
  }
}

void zx48k::dump() {
  {
    std::ofstream fout("memdump", std::ios::out | std::ios::binary);
    fout.write((char *)memory_, MEMORY_SIZE);
    fout.close();
  }
  {
    std::ofstream fout("regdump", std::ios::out | std::ios::binary);
    struct z80_regs r = cpu.get_regs();
    fout.write((char *)&r, sizeof(struct z80_regs));
    fout.close();
  }
}

bool zx48k::trap(uint16_t pc) {
  if (pc == 0x056c) { // start tape load
    if (tape_) {
      size_t len = tape_->trapload(cpu);
      if (len > 0) {
        pausecycles_ = len * 20;
        didtrap_ = true;
      } else {
        didtrap_ = false;
      }
      return didtrap_;
    }	
    didtrap_ = false;
    return false;
  } else if (pc == 0x15e1 && auto_) {
    auto_ = false;
    keystopress_ = std::vector<std::vector<uint8_t > > { { (6<<3|3)}, {}, {(7<<3|1)}, {(7<<3|1), (5 << 3 | 0)}, {}, {(7<<3|1)}, {(7<<3|1), (5 << 3 | 0)}, {}, {(6 << 3 | 0)} };
    return false;
  }
  return false;
}

bool zx48k::processinput() {
  emusdl.readinput();
  if (emusdl.key_pressed(SDLK_F5)) {
          trace_ = true;
  } else if (emusdl.key_pressed(SDLK_F6)) {
    trace_ = false;
  } else if (emusdl.key_pressed(SDLK_F9)) {
    if (debounce_ != SDLK_F9) {
      debounce_ = SDLK_F9;
      dump();
    }
  } else if (emusdl.key_pressed(SDLK_F7) && tape_) {
    if (debounce_ != SDLK_F7) {
      debounce_ = SDLK_F7;
      tape_->reset();
      tape_->go();
    }
  } else if (emusdl.key_pressed(SDLK_F8)) {
    if (debounce_ != SDLK_F8) {
      turbo_ = !turbo_;
      debounce_ = SDLK_F8;
      cout << string("Turbo mode ") << (turbo_ ? "on" : "off")
           << std::endl;
    }
  } else if (emusdl.key_pressed(SDLK_F4)) {
    cout << "Quitting..." << std::endl;
    return false;
  } else if (emusdl.keys_pressed()) {
    debounce_ = 0;
  }
  return true;
}

void zx48k::run() {
  int line = 0;
  int frame = 0;
  uint64_t v_diff = 0;
  uint64_t d_diff = 0;
  uint64_t a_diff = 0;

  int64_t acc_delay = 0;
  int audioStarted = 0;
  std::chrono::steady_clock::time_point tv_s, tv_e;
  tv_s = std::chrono::steady_clock::now();
  for (;;) {
    if (trace_) {
      std::cout << cpu.get_trace();
    }
    uint64_t cycles;
    uint64_t diff;
    if (pausecycles_ > 0) {
      diff = 12;
      if ((pausecycles_ % 500) == 0) {
         border = (pausecycles_ % 1000) ? 1 : 6;
      }
      pausecycles_--;
    } else {
      cycles = cpu.tick();
      diff = cycles - cpucycles_;
      cpucycles_ = cycles;
    }
    if (tape_) {
      tape_->update_ticks(diff);
    }

    v_diff += diff;
    d_diff += diff;
    a_diff += diff;

    // draw lines that should be drawn now.
    while (v_diff > 224) {
      scanline(line++);
      if (line >= 312) {
        line = 0;
        if (!processinput()) {
          return;
        }
        if (audioStarted < 20 && ++audioStarted == 20) {
          SDL_PauseAudioDevice(sdldev, 0);
        }

        if (pausecycles_ == 0)  {
          cpu.interrupt(0xff);
        }
        if (!turbo_) {
          emusdl.redrawscreen();
        }
        if (++frame % 16 == 0) {
          flashstate = !flashstate;
          if ((frame % 100 == 0) && turbo_) {
            emusdl.redrawscreen();
          }
        }
      }
      v_diff -= 224;
    }

    while (a_diff > EARCYCLES) {
      if (aearbufpos_ == EARBUFOFFSET*INT_AUDIO_BUF_SIZE + EARBUFRESERVE) {
        cout << "Too much !\n";
        break;
      }
      aearbuf_[aearbufpos_++] = ear;
      a_diff -= EARCYCLES;
      processAudio();
    }

    // correct timing:
    // 'd_diff' cycles should take us d_diff * 285ns, if we're too late - too
    // bad if we're too soon - wait do it only if d_diff > 10000 to save time on
    // unnecessary clock_gettimes
    // TODO move it into emusdl
    if (d_diff > 30000) {
      tv_e = std::chrono::steady_clock::now();
      uint64_t real_time = std::chrono::nanoseconds(tv_e - tv_s).count();
      uint64_t handl_time = d_diff * 285; // 3.5MHz == 1/285ns
      d_diff = 0;

      acc_delay += handl_time - real_time;

      if (acc_delay < 0 || turbo_) {
        acc_delay = 0;
      }

      if (acc_delay > 1000000) {
        uint64_t c = acc_delay / 1000000;
        acc_delay -= c * 1000000;
        SDL_Delay(c);
      }
      tv_s = std::chrono::steady_clock::now();
    }
  }
}

