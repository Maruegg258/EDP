#include "ClockWidget.h"

#include "Font9x13.h"
#include "UiText.h"

ClockWidget::ClockWidget(GraphicsBW& graphics,
                         int16_t centerX,
                         int16_t y)
    : _graphics(graphics),
      _centerX(centerX),
      _y(y) {
}

bool ClockWidget::render(const ClockWidgetState& state) {
  return UiText::drawCentered(
      _graphics,
      Font9x13::FONT,
      state.time,
      _centerX,
      _y,
      3,
      true
  );
}
