#include "spectext.h"

SpecText::SpecText(const unsigned char *chardefs, uint32_t *pixels, int w,
                   int h)
    : chardefs_(chardefs), pixels_(pixels), w_(w), h_(h) {}

void SpecText::Draw(unsigned char chr, int x, int y, uint32_t bg, uint32_t fg) {
  for (int yy = 0; yy < 8; ++yy) {
    for (int xx = 0; xx < 8; ++xx) {
      int pos = (y + yy) * w_ + (x + xx);
      bool pix = (chardefs_[(chr - 32) * 8 + yy] >> (7 - xx)) & 1;
      pixels_[pos] = pix ? fg : bg;
    }
  }
}

void SpecText::Write(const char *str, int x, int y, int d, uint32_t bg,
                     uint32_t fg) {
  while (str[0] != '\0') {
    Draw(str[0], x, y, bg, fg);
    str++;
    x += 8 + d;
  }
}
