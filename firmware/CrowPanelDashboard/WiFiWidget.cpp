#include "WiFiWidget.h"

WiFiWidget::WiFiWidget(GraphicsBW& graphics,
                       int16_t x,
                       int16_t y)
    : _graphics(graphics),
      _x(x),
      _y(y) {
}

bool WiFiWidget::render(const WiFiWidgetState& state) {
  return _graphics.drawBitmap(
      *state.icon,
      _x,
      _y,
      true
  );
}
