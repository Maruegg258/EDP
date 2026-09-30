#include "WiFiWidget.h"

#include "Font5x7.h"
#include "Font9x13.h"

WiFiWidget::WiFiWidget(GraphicsBW& graphics,
                       int16_t x,
                       int16_t y)
    : _graphics(graphics),
      _x(x),
      _y(y) {
}

bool WiFiWidget::render(const Bitmap1bpp& icon, const char* rssi) {
  return _graphics.drawText(
             Font5x7::FONT,
             "WIFI",
             _x,
             _y,
             1,
             true) &&
         _graphics.drawText(
             Font9x13::FONT,
             rssi,
             _x,
             _y + 16,
             1,
             true) &&
         _graphics.drawBitmap(
             icon,
             _x + 76,
             _y + 4,
             true);
}
