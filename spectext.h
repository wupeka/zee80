#ifndef _SPECTEXT_H
#define _SPECTEXT_H
#include <cstdint>

class SpecText {
public:
  SpecText(const unsigned char *chardefs, uint32_t *pixels, int w, int h);
  void Draw(unsigned char chr, int x, int y, uint32_t bg, uint32_t fg);
  void Write(char *string, int x, int y, int d, uint32_t bg, uint32_t fg);

private:
  const unsigned char *chardefs_;
  uint32_t *pixels_;
  int w_;
  int h_;
};

#endif
