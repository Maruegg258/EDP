#pragma once

#include <Arduino.h>

#include "BitmapFont.h"

// Minimal project-owned 5x7 bitmap font used for bring-up/regression tests.
// Phase 2B-1 keeps only the glyphs needed for HELLO; character expansion is
// deliberately deferred to the next checkpoint.
namespace Font5x7 {

static constexpr uint8_t GLYPH_WIDTH = 5;
static constexpr uint8_t GLYPH_HEIGHT = 7;
static constexpr uint8_t GLYPH_SPACING = 1;
static constexpr uint8_t GLYPH_ADVANCE = GLYPH_WIDTH + GLYPH_SPACING;

// BitmapFont stores each row MSB-first. The visible 5 pixels therefore occupy
// bits 7..3 of each byte.
static constexpr uint8_t H[GLYPH_HEIGHT] = {
  0b10001000, 0b10001000, 0b10001000, 0b11111000,
  0b10001000, 0b10001000, 0b10001000
};
static constexpr uint8_t E[GLYPH_HEIGHT] = {
  0b11111000, 0b10000000, 0b10000000, 0b11110000,
  0b10000000, 0b10000000, 0b11111000
};
static constexpr uint8_t L[GLYPH_HEIGHT] = {
  0b10000000, 0b10000000, 0b10000000, 0b10000000,
  0b10000000, 0b10000000, 0b11111000
};
static constexpr uint8_t O[GLYPH_HEIGHT] = {
  0b01110000, 0b10001000, 0b10001000, 0b10001000,
  0b10001000, 0b10001000, 0b01110000
};

static constexpr BitmapGlyph GLYPHS[] = {
  {'H', GLYPH_WIDTH, GLYPH_HEIGHT, GLYPH_ADVANCE, H},
  {'E', GLYPH_WIDTH, GLYPH_HEIGHT, GLYPH_ADVANCE, E},
  {'L', GLYPH_WIDTH, GLYPH_HEIGHT, GLYPH_ADVANCE, L},
  {'O', GLYPH_WIDTH, GLYPH_HEIGHT, GLYPH_ADVANCE, O},
};

static constexpr BitmapFont FONT = {
  GLYPHS,
  static_cast<uint16_t>(sizeof(GLYPHS) / sizeof(GLYPHS[0])),
  GLYPH_HEIGHT
};

}  // namespace Font5x7
