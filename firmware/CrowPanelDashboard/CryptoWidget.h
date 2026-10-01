#pragma once

#include <Arduino.h>

#include "Bitmap1bpp.h"
#include "GraphicsBW.h"
#include "WidgetStates.h"

class CryptoWidget {
public:
  CryptoWidget(GraphicsBW& graphics,
               int16_t x,
               int16_t y,
               const Bitmap1bpp& icon,
               const char* symbol);

  bool render(const CryptoWidgetState& state);

private:
  GraphicsBW& _graphics;
  int16_t _x;
  int16_t _y;
  const Bitmap1bpp& _icon;
  const char* _symbol;
};
