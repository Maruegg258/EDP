#pragma once

#include <Arduino.h>

// Minimal project-owned 5x7 bitmap font used for Phase 1D bring-up.
// Only the glyphs needed for HELLO are implemented.
namespace Font5x7 {

static constexpr uint8_t GLYPH_WIDTH = 5;
static constexpr uint8_t GLYPH_HEIGHT = 7;
static constexpr uint8_t GLYPH_SPACING = 1;

static constexpr uint8_t H[GLYPH_HEIGHT] = {
  0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001
};
static constexpr uint8_t E[GLYPH_HEIGHT] = {
  0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111
};
static constexpr uint8_t L[GLYPH_HEIGHT] = {
  0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111
};
static constexpr uint8_t O[GLYPH_HEIGHT] = {
  0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110
};

inline const uint8_t* glyph(char c) {
  switch (c) {
    case 'H': return H;
    case 'E': return E;
    case 'L': return L;
    case 'O': return O;
    default: return nullptr;
  }
}

}  // namespace Font5x7
