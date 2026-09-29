#pragma once

#include <Arduino.h>

#include "BitmapFont.h"

class GraphicsBW {
public:
  GraphicsBW(uint8_t* buffer,
             uint16_t rawWidth,
             uint16_t visibleWidth,
             uint16_t height,
             uint16_t seamX,
             uint16_t seamGap);

  void clear(bool white = true);
  void setPixel(int16_t x, int16_t y, bool black);
  void drawLine(int16_t x0,
                int16_t y0,
                int16_t x1,
                int16_t y1,
                bool black);
  void drawRect(int16_t x,
                int16_t y,
                uint16_t width,
                uint16_t height,
                bool black);
  void fillRect(int16_t x,
                int16_t y,
                uint16_t width,
                uint16_t height,
                bool black);

  bool drawGlyph(const BitmapFont& font,
                 char c,
                 int16_t x,
                 int16_t y,
                 uint8_t scale = 1,
                 bool black = true);
  bool drawText(const BitmapFont& font,
                const char* text,
                int16_t x,
                int16_t y,
                uint8_t scale = 1,
                bool black = true);
  uint16_t textWidth(const BitmapFont& font,
                     const char* text,
                     uint8_t scale = 1) const;

  uint16_t width() const { return _visibleWidth; }
  uint16_t height() const { return _height; }

  uint8_t* data() { return _buffer; }
  const uint8_t* data() const { return _buffer; }

private:
  uint16_t visibleToRawX(uint16_t x) const;
  void setRawPixel(uint16_t x, uint16_t y, bool black);
  const BitmapGlyph* findGlyph(const BitmapFont& font, char c) const;

  uint8_t* _buffer;
  uint16_t _rawWidth;
  uint16_t _visibleWidth;
  uint16_t _height;
  uint16_t _seamX;
  uint16_t _seamGap;
  uint16_t _stride;
};
