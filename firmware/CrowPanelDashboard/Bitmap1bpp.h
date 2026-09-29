#pragma once

#include <Arduino.h>

// Row-major 1-bit bitmap data.
// Rows are MSB-first and padded to a whole byte.
// A set bit is foreground; a clear bit is transparent.
struct Bitmap1bpp {
  uint16_t width;
  uint16_t height;
  const uint8_t* data;
};
