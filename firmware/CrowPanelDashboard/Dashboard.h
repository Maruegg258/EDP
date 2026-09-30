#pragma once

#include <Arduino.h>

#include "Bitmap1bpp.h"
#include "BitmapFont.h"
#include "DashboardState.h"
#include "GraphicsBW.h"

class Dashboard {
public:
  explicit Dashboard(GraphicsBW& graphics);

  bool render(const DashboardState& state);

private:
  bool drawCenteredText(const BitmapFont& font,
                        const char* text,
                        int16_t centerX,
                        int16_t y,
                        uint8_t scale,
                        bool black = true);

  bool drawCryptoCard(int16_t x,
                      const Bitmap1bpp& icon,
                      const char* symbol,
                      const char* price);

  GraphicsBW& _graphics;
};
