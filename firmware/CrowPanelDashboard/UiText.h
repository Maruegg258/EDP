#pragma once

#include <Arduino.h>

#include "BitmapFont.h"
#include "GraphicsBW.h"

namespace UiText {

inline bool drawCentered(GraphicsBW& graphics,
                         const BitmapFont& font,
                         const char* text,
                         int16_t centerX,
                         int16_t y,
                         uint8_t scale,
                         bool black = true) {
  const uint16_t width = graphics.textWidth(font, text, scale);
  if (width == 0 || width > graphics.width()) {
    return false;
  }

  const int16_t x = static_cast<int16_t>(centerX - width / 2);
  return graphics.drawText(font, text, x, y, scale, black);
}

}  // namespace UiText
