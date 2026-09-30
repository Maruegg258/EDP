#include "WeatherWidget.h"

#include "Font5x7.h"

WeatherWidget::WeatherWidget(GraphicsBW& graphics,
                             int16_t x,
                             int16_t y)
    : _graphics(graphics),
      _x(x),
      _y(y) {
}

bool WeatherWidget::render(const Bitmap1bpp& icon,
                           const char* label,
                           const char* temperature) {
  return _graphics.drawBitmap(icon, _x, _y + 2, true) &&
         _graphics.drawText(
             Font5x7::FONT,
             label,
             _x + 46,
             _y,
             1,
             true) &&
         _graphics.drawText(
             Font5x7::FONT,
             temperature,
             _x + 46,
             _y + 18,
             2,
             true);
}
