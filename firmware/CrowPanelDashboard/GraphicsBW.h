#pragma once

#include <Arduino.h>

class GraphicsBW {
public:
  GraphicsBW(uint8_t* buffer, uint16_t width, uint16_t height);

  void clear(bool white = true);
  void setPixel(uint16_t x, uint16_t y, bool black);

  uint8_t* data() { return _buffer; }
  const uint8_t* data() const { return _buffer; }

private:
  uint8_t* _buffer;
  uint16_t _width;
  uint16_t _height;
  uint16_t _stride;
};
