/*
 * zx48k.cpp
 *
 *  Created on: 20 mar 2016
 *      Author: wpk
 */

#include "zx48k.h"
#include <Ym2149Ex.h>
#include <YmProfiles.h>
#include <boost/program_options.hpp>
#include <cassert>
#include <ctime>
#include <fstream>
#include <iostream>
#include <strings.h>
#include <unistd.h> // TODO remove (usleep)

using namespace std;
namespace po = boost::program_options;

constexpr int zx48k::MEMORY_SIZE;

namespace {
constexpr int COLS = 352;
constexpr int LINES = 296;
constexpr int TOP_BORDER_LINES = 48;
constexpr int BOTTOM_BORDER_LINES = 56;
constexpr int LEFT_BORDER_COLS = 48;
constexpr int RIGHT_BORDER_COLS = 48;
constexpr uint32_t zxpalette[16] = {
    0x00000000, 0x000000CD, 0x00CD0000, 0x00CD00CD, 0x0000CD00, 0x0000CDCD,
    0x00CDCD00, 0x00CDCDCD, 0x00000000, 0x000000FF, 0x00FF0000, 0x00FF00FF,
    0x0000FF00, 0x0000FFFF, 0x00FFFF00, 0x00FFFFFF};
} // namespace

zx48k::zx48k()
    : EmuSDL(COLS, LINES, 2, 2), cpu(*this), romfile("testrom.bin"),
      ay(new CYm2149Ex(profileSpectrum, 44100)),
      ff("audio", std::ios::out | std::ios::binary) {
  SDL_AudioSpec want;
  SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
  want.freq = 44100;
  want.format = AUDIO_S16;
  want.channels = 2;
  want.samples = 4096;
  sdldev = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
  if (sdldev == 0) {
    printf("%s\n", SDL_GetError());
  }
  clock_gettime(CLOCK_MONOTONIC, &lastAyWrite);

  // Tape load traps
}

void zx48k::initialize() {
  //  memset(memory, 0xff, MEMORY_SIZE);
  std::ifstream fin(romfile, std::ios::in | std::ios::binary);
  if (fin.fail()) {
    throw ifstream::failure("Can't open ROM file");
  }
  fin.seekg(0, std::ios_base::end);
  uint64_t len = fin.tellg();
  fin.seekg(0);
  fin.read((char *)memory, len);
  fin.close();
  border = 7;
  if (tapfile != "") {
    tape = new zxtape(tapfile);
  }
  if (trap_) {
    cpu.addtrap(0x056c);
  }
}

void zx48k::parse_opts(int argc, char **argv) {
  po::options_description desc("Allowed options");
  desc.add_options()("help", "display this message help message")
      ("rom", po::value<string>(), "ROM file to use")
      ("tap", po::value<string>(), "Tape to load")
      ("trace", po::bool_switch()->default_value(false), "Enable tracing")
      ("trap", po::bool_switch()->default_value(false), "Enable traploader");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << desc << "\n";
    exit(1);
  }

  if (vm.count("rom")) {
    romfile = vm["rom"].as<string>();
  }
  if (vm.count("tap")) {
    tapfile = vm["tap"].as<string>();
  }
  if (vm["trace"].as<bool>()) {
    trace = true;
  }
  if (vm["trap"].as<bool>()) {
    trap_ = true;
  }
}

uint32_t zx48k::readmem(uint16_t address, bool dotrace) {
  if (trace && dotrace) {
    cout << "READMEM " << std::hex << (int)address << " "
         << (int)(memory[address] & 0xff) << endl;
  }
  return *((uint32_t *)(memory + address));
  return memory[address] + (memory[address + 1] << 8) +
         (memory[address + 2] << 16) + (memory[address + 3] << 24);
}

void zx48k::writemem(uint16_t address, uint8_t v, bool dotrace) {
  assert(address < MEMORY_SIZE);
  if (address >= 0x4000) {
    memory[address] = v;
    if (trace && dotrace) {
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
  case SDLK_RSHIFT:
    return 7 << 3 | 1;
  case SDLK_m:
    return 7 << 3 | 2;
  case SDLK_n:
    return 7 << 3 | 3;
  case SDLK_b:
    return 7 << 3 | 4;
  default:
    return 0xff;
  }
}
#define NSEC_PER_SEC 1000000000
void zx48k::processAudio() {
  struct timespec tv_e;
  clock_gettime(CLOCK_MONOTONIC, &tv_e);
  uint64_t proc_time = (tv_e.tv_sec - lastAyWrite.tv_sec) * NSEC_PER_SEC +
                       tv_e.tv_nsec - lastAyWrite.tv_nsec;
  if (proc_time > 100000) {
    uint64_t samples = (proc_time * 44100) / NSEC_PER_SEC;
    uint64_t proc_ns = (samples * NSEC_PER_SEC) / 44100;
    lastAyWrite.tv_nsec += proc_ns;
    while (lastAyWrite.tv_nsec >= NSEC_PER_SEC) {
      lastAyWrite.tv_nsec -= NSEC_PER_SEC;
      lastAyWrite.tv_sec++;
    }
    ymsample buf[2 * samples];
    ay->updateStereo(buf, samples);
    uint64_t samps = 0;
    while (!earStates.empty()) {
      auto x = earStates.front();
      earStates.erase(earStates.begin());
      uint64_t samps_to_go = (x.second * 44100) / NSEC_PER_SEC;
      if (x.first) {
        int i;
        for (i = 0; i < samps_to_go && i + samps < samples; i++) {
          buf[2 * (samps + i)] += 0x3fff;
          buf[2 * (samps + i) + 1] += 0x3fff;
        }
        if (i < samps_to_go) {
          x.second -= ((samps_to_go - i) * NSEC_PER_SEC) / 44100;
          earStates.insert(earStates.begin(), x);
          break;
        }
      }
      samps += samps_to_go;
    }
    //    if (samps > 0) {
    //      std::cout << samps << " " << samples << std::endl;
    //    }
    ff.write((char *)buf, 2 * sizeof(ymsample) * samples);
    int i = SDL_QueueAudio(sdldev, buf, 2 * sizeof(ymsample) * samples);
    if (i != 0) {
      cout << SDL_GetError();
    }
  }
}

void zx48k::writeio(uint16_t address, uint8_t v) {
  if ((address & 0x1) == 0) { // ULA, might be & 0xff == 0xfe
    if (ear != (bool)(v & 0x10)) {
      ear = v & 0x10;
      uint64_t proc_time = lastcycles - lastEarChange;
      earStates.push_back(std::make_pair(ear, (proc_time * 285)));
      lastEarChange = lastcycles;
    }
    mic = v & 0x08;
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
    for (auto sdlkey : get_keys()) {
      uint8_t key = sdlkey2spectrum(sdlkey);
      if (key != 0xff) {
        uint8_t kline = 1 << (key >> 3);
        uint8_t kaddr = 1 << (key & 0b111);
        if (kline & lines) {
          out &= ~(kaddr); // drop the line pressed
        }
      }
    }
    if (tape) {
      bool n_ear;
      if (tape->ear()) {
        out |= 1 << 6;
        n_ear = true;
      } else {
        n_ear = false;
      }
      if (ear != n_ear) {
        ear = n_ear;
        uint64_t proc_time = lastcycles - lastEarChange;
        earStates.push_back(std::make_pair(n_ear, (proc_time * 285)));
        lastEarChange = lastcycles;
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
  if (y >= LINES) { // 16 empty scanlines at the end
    return;
  } else if ((y < 48) || (y >= 240)) {
    // TODO unloopize it
    for (int x = 0; x < 352; x++) {
      uint32_t pixel = COLS * y + x;
      pixels[pixel] = zxpalette[border];
    }
  } else {
    for (int x = 0; x < 48; x++) {
      uint32_t pixel = COLS * y + x;
      pixels[pixel] = zxpalette[border];
    }
    for (int xx = 0; xx < 32; xx++) {
      int yy = y - TOP_BORDER_LINES;
      // 0 1 0 y7 y6 y2 y1 y0 y5 y4 y3 x4 x3 x2 x1 x0
      uint16_t addr = (1 << 14) | xx | (((yy >> 3) & 0x7) << 5) |
                      ((yy & 7) << 8) | ((yy >> 6) << 11);
      uint8_t attrs = memory[0x5800 + xx + 32 * (yy / 8)];
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

      for (int i = 0; i < 8; i++) {
        uint32_t pixel = COLS * y + 48 + 8 * xx + i;
        bool lit = memory[addr] & (1 << (7 - i));
        pixels[pixel] = zxpalette[lit ? ink : paper];
      }
    }
    // TODO unloopize it
    for (int x = 304; x < 352; x++) {
      uint32_t pixel = COLS * y + x;
      pixels[pixel] = zxpalette[border];
    }
  }
}

void zx48k::dump() {
  {
    std::ofstream fout("memdump", std::ios::out | std::ios::binary);
    fout.write((char *)memory, MEMORY_SIZE);
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
  if (tape) {
    return tape->trapload(cpu);
  }
  return false;
}

void zx48k::run() {
  int line = 0;
  int frame = 0;
  uint64_t v_diff = 0;
  uint64_t d_diff = 0;
  uint64_t a_diff = 0;
  int64_t acc_delay = 0;
  bool audioStarted = false;
  struct timespec tv_s, tv_e;
  clock_gettime(CLOCK_MONOTONIC, &tv_s);
  SDL_Keycode debounce;
  for (;;) {
    if (trace) {
      std::cout << cpu.get_trace();
    }
    uint64_t cycles = cpu.tick();
    uint64_t diff = cycles - lastcycles;
    lastcycles = cycles;
    if (tape) {
      tape->update_ticks(diff);
    }

    v_diff += diff;
    d_diff += diff;
    a_diff += diff;

    // draw lines that should be drawn now.
    while (v_diff > 224) {
      scanline(line++);
      processAudio();
      if (!audioStarted) {
        SDL_PauseAudioDevice(sdldev, 0);
        audioStarted = true;
      }
      if (line >= 312) {
        line = 0;
        // process input
        readinput();
        if (key_pressed(SDLK_F5)) {
          trace = true;
        } else if (key_pressed(SDLK_F6)) {
          trace = false;
        } else if (key_pressed(SDLK_F9)) {
          if (debounce != SDLK_F9) {
            debounce = SDLK_F9;
            dump();
          }
        } else if (key_pressed(SDLK_F7) && tape) {
          if (debounce != SDLK_F7) {
            debounce = SDLK_F7;
            tape->reset();
            tape->go();
          }
        } else if (key_pressed(SDLK_F8)) {
          if (debounce != SDLK_F8) {
            turbo_ = !turbo_;
            debounce = SDLK_F8;
            cout << string("Turbo mode ") << (turbo_ ? "on" : "off")
                 << std::endl;
          }
        } else if (key_pressed(SDLK_F4)) {
          cout << "Quitting..." << std::endl;
          return;
        } else if (keys_pressed()) {
          debounce = 0;
        }
        cpu.interrupt(0xff);
        if (!turbo_) {
          redrawscreen();
        }
        if (++frame % 16 == 0) {
          flashstate = !flashstate;
          if ((frame % 100 == 0) && turbo_) {
            redrawscreen();
          }
        }
      }
      v_diff -= 224;
    }

    // correct timing:
    // 'd_diff' cycles should take us d_diff * 285ns, if we're too late - too
    // bad if we're too soon - wait do it only if d_diff > 10000 to save time on
    // unnecessary clock_gettimes
    // TODO move it into emusdl
    if (d_diff > 1000) {
      clock_gettime(CLOCK_MONOTONIC, &tv_e);
      uint64_t real_time = (tv_e.tv_sec - tv_s.tv_sec) * 1000000000 +
                           tv_e.tv_nsec - tv_s.tv_nsec;
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
      clock_gettime(CLOCK_MONOTONIC, &tv_s);
    }
  }
}

zx48k *emu;
int main(int argc, char **argv) {
  SDL_Init(SDL_INIT_EVERYTHING);
  emu = new zx48k();
  emu->parse_opts(argc, argv);
  emu->initialize();
  emu->run();
}
