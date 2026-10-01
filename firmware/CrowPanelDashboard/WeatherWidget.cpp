#include "WeatherWidget.h"

#include "Font5x7.h"

WeatherWidget::WeatherWidget(GraphicsBW& graphics,
                             int16_t x,
                             int16_t y)
    : _graphics(graphics),
      _x(x),
      _y(y) {
}

bool WeatherWidget::render(const WeatherWidgetState& state) {
  return _graphics.drawBitmap(*state.icon, _x, _y + 2, true) &&
         _graphics.drawText(
             Font5x7::FONT,
             state.label,
             _x + 46,
             _y,
             1,
             true) &&
         _graphics.drawText(
             Font5x7::FONT,
             state.temperature,
             _x + 46,
             _y + 18,
             2,
             true);
}
