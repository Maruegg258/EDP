#pragma once

#include <Arduino.h>

// Row-major 1-bit bitmap font data.
// Each glyph row is packed MSB-first and padded to a whole byte.
struct BitmapGlyph {
  char code;
  uint8_t width;
  uint8_t height;
  uint8_t xAdvance;
  const uint8_t* bitmap;
};

struct BitmapFont {
  const BitmapGlyph* glyphs;
  uint16_t glyphCount;
  uint8_t lineHeight;
};
