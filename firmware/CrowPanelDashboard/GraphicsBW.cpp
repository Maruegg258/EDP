#include "GraphicsBW.h"

#include <cstring>

GraphicsBW::GraphicsBW(uint8_t* buffer,
                       uint16_t rawWidth,
                       uint16_t visibleWidth,
                       uint16_t height,
                       uint16_t seamX,
                       uint16_t seamGap)
    : _buffer(buffer),
      _rawWidth(rawWidth),
      _visibleWidth(visibleWidth),
      _height(height),
      _seamX(seamX),
      _seamGap(seamGap),
      _stride((rawWidth + 7U) / 8U) {}

void GraphicsBW::clear(bool white) {
  if (_buffer == nullptr) {
    return;
  }

  memset(_buffer, white ? 0xFF : 0x00, static_cast<size_t>(_stride) * _height);
}

uint16_t GraphicsBW::visibleToRawX(uint16_t x) const {
  return (x < _seamX)
      ? x
      : static_cast<uint16_t>(x + _seamGap);
}

void GraphicsBW::setRawPixel(uint16_t x, uint16_t y, bool black) {
  if (_buffer == nullptr || x >= _rawWidth || y >= _height) {
    return;
  }

  const size_t index = static_cast<size_t>(y) * _stride + (x / 8U);
  const uint8_t mask = static_cast<uint8_t>(0x80U >> (x % 8U));

  if (black) {
    _buffer[index] &= static_cast<uint8_t>(~mask);
  } else {
    _buffer[index] |= mask;
  }
}

void GraphicsBW::setPixel(int16_t x, int16_t y, bool black) {
  if (x < 0 || y < 0 ||
      static_cast<uint16_t>(x) >= _visibleWidth ||
      static_cast<uint16_t>(y) >= _height) {
    return;
  }

  setRawPixel(
      visibleToRawX(static_cast<uint16_t>(x)),
      static_cast<uint16_t>(y),
      black
  );
}

void GraphicsBW::drawLine(int16_t x0,
                          int16_t y0,
                          int16_t x1,
                          int16_t y1,
                          bool black) {
  int32_t x = x0;
  int32_t y = y0;
  const int32_t targetX = x1;
  const int32_t targetY = y1;
  const int32_t dx = (targetX >= x) ? (targetX - x) : (x - targetX);
  const int32_t sx = (x < targetX) ? 1 : -1;
  const int32_t absDy = (targetY >= y) ? (targetY - y) : (y - targetY);
  const int32_t dy = -absDy;
  const int32_t sy = (y < targetY) ? 1 : -1;
  int32_t error = dx + dy;

  while (true) {
    setPixel(static_cast<int16_t>(x), static_cast<int16_t>(y), black);

    if (x == targetX && y == targetY) {
      break;
    }

    const int32_t error2 = error * 2;
    if (error2 >= dy) {
      error += dy;
      x += sx;
    }
    if (error2 <= dx) {
      error += dx;
      y += sy;
    }
  }
}

void GraphicsBW::drawRect(int16_t x,
                          int16_t y,
                          uint16_t width,
                          uint16_t height,
                          bool black) {
  if (width == 0 || height == 0) {
    return;
  }

  const int16_t right = static_cast<int16_t>(x + width - 1U);
  const int16_t bottom = static_cast<int16_t>(y + height - 1U);

  drawLine(x, y, right, y, black);
  drawLine(x, bottom, right, bottom, black);
  drawLine(x, y, x, bottom, black);
  drawLine(right, y, right, bottom, black);
}

void GraphicsBW::fillRect(int16_t x,
                          int16_t y,
                          uint16_t width,
                          uint16_t height,
                          bool black) {
  for (uint16_t yy = 0; yy < height; ++yy) {
    for (uint16_t xx = 0; xx < width; ++xx) {
      setPixel(
          static_cast<int16_t>(x + xx),
          static_cast<int16_t>(y + yy),
          black
      );
    }
  }
}

const BitmapGlyph* GraphicsBW::findGlyph(const BitmapFont& font, char c) const {
  if (font.glyphs == nullptr) {
    return nullptr;
  }

  for (uint16_t i = 0; i < font.glyphCount; ++i) {
    if (font.glyphs[i].code == c) {
      return &font.glyphs[i];
    }
  }

  return nullptr;
}

bool GraphicsBW::drawGlyph(const BitmapFont& font,
                           char c,
                           int16_t x,
                           int16_t y,
                           uint8_t scale,
                           bool black) {
  const BitmapGlyph* glyph = findGlyph(font, c);
  if (glyph == nullptr || glyph->bitmap == nullptr ||
      glyph->width == 0 || glyph->height == 0 || scale == 0) {
    return false;
  }

  const uint16_t rowBytes =
      static_cast<uint16_t>((glyph->width + 7U) / 8U);

  for (uint8_t row = 0; row < glyph->height; ++row) {
    for (uint8_t col = 0; col < glyph->width; ++col) {
      const size_t byteIndex =
          static_cast<size_t>(row) * rowBytes + (col / 8U);
      const uint8_t mask = static_cast<uint8_t>(0x80U >> (col % 8U));

      if ((glyph->bitmap[byteIndex] & mask) != 0U) {
        fillRect(
            static_cast<int16_t>(x + col * scale),
            static_cast<int16_t>(y + row * scale),
            scale,
            scale,
            black
        );
      }
    }
  }

  return true;
}

bool GraphicsBW::drawText(const BitmapFont& font,
                          const char* text,
                          int16_t x,
                          int16_t y,
                          uint8_t scale,
                          bool black) {
  if (text == nullptr || scale == 0) {
    return false;
  }

  int16_t cursorX = x;
  while (*text != '\0') {
    const BitmapGlyph* glyph = findGlyph(font, *text);
    if (glyph == nullptr || !drawGlyph(font, *text, cursorX, y, scale, black)) {
      return false;
    }

    cursorX = static_cast<int16_t>(
        cursorX + static_cast<uint16_t>(glyph->xAdvance) * scale
    );
    ++text;
  }

  return true;
}

uint16_t GraphicsBW::textWidth(const BitmapFont& font,
                               const char* text,
                               uint8_t scale) const {
  if (text == nullptr || *text == '\0' || scale == 0) {
    return 0;
  }

  uint32_t width = 0;
  const BitmapGlyph* glyph = nullptr;

  while (*text != '\0') {
    glyph = findGlyph(font, *text);
    if (glyph == nullptr) {
      return 0;
    }

    ++text;
    if (*text == '\0') {
      width += static_cast<uint32_t>(glyph->width) * scale;
    } else {
      width += static_cast<uint32_t>(glyph->xAdvance) * scale;
    }
  }

  return (width <= 0xFFFFU) ? static_cast<uint16_t>(width) : 0;
}
