#pragma once

#include "GraphicsBW.h"
#include "MarketPageState.h"

class MarketPage {
public:
  explicit MarketPage(GraphicsBW& graphics);

  bool render(const MarketPageState& state);

private:
  bool renderRow(const char* label,
                 const char* price,
                 int16_t y);

  GraphicsBW& _graphics;
};
