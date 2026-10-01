#include "StatusWidget.h"

#include "Font5x7.h"
#include "UiText.h"

StatusWidget::StatusWidget(GraphicsBW& graphics,
                           int16_t x,
                           int16_t y,
                           uint16_t width,
                           uint16_t height)
    : _graphics(graphics),
      _x(x),
      _y(y),
      _width(width),
      _height(height) {
}

bool StatusWidget::render(const StatusWidgetState& state) {
  _graphics.fillRect(_x, _y, _width, _height, true);

  return UiText::drawCentered(
      _graphics,
      Font5x7::FONT,
      state.text,
      static_cast<int16_t>(_x + _width / 2),
      static_cast<int16_t>(_y + 8),
      2,
      false
  );
}
