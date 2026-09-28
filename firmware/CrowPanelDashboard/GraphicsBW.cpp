#include "GraphicsBW.h"

#include <cstring>

GraphicsBW::GraphicsBW(uint8_t* buffer, uint16_t width, uint16_t height)
    : _buffer(buffer),
      _width(width),
      _height(height),
      _stride((width + 7U) / 8U) {}

void GraphicsBW::clear(bool white) {
  if (_buffer == nullptr) {
    return;
  }

  memset(_buffer, white ? 0xFF : 0x00, static_cast<size_t>(_stride) * _height);
}

void GraphicsBW::setPixel(uint16_t x, uint16_t y, bool black) {
  if (_buffer == nullptr || x >= _width || y >= _height) {
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
