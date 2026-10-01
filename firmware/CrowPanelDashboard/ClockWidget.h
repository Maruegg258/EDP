#pragma once

#include <Arduino.h>

#include "GraphicsBW.h"
#include "WidgetStates.h"

class ClockWidget {
public:
  ClockWidget(GraphicsBW& graphics, int16_t centerX, int16_t y);

  bool render(const ClockWidgetState& state);

private:
  GraphicsBW& _graphics;
  int16_t _centerX;
  int16_t _y;
};
