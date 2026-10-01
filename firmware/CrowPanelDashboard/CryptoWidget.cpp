#include "CryptoWidget.h"

#include "Font5x7.h"
#include "Font9x13.h"
#include "UiText.h"

namespace {

constexpr uint16_t CARD_WIDTH = 230;
constexpr uint16_t CARD_HEIGHT = 126;

}  // namespace

CryptoWidget::CryptoWidget(GraphicsBW& graphics,
                           int16_t x,
                           int16_t y,
                           const Bitmap1bpp& icon,
                           const char* symbol)
    : _graphics(graphics),
      _x(x),
      _y(y),
      _icon(icon),
      _symbol(symbol) {
}

bool CryptoWidget::render(const CryptoWidgetState& state) {
  const int16_t centerX =
      static_cast<int16_t>(_x + CARD_WIDTH / 2);

  _graphics.drawRect(
      _x,
      _y,
      CARD_WIDTH,
      CARD_HEIGHT,
      true
  );

  if (!_graphics.drawBitmap(
          _icon,
          centerX - 16,
          _y + 12,
          true)) {
    return false;
  }

  if (!UiText::drawCentered(
          _graphics,
          Font5x7::FONT,
          _symbol,
          centerX,
          _y + 50,
          2,
          true)) {
    return false;
  }

  if (!UiText::drawCentered(
          _graphics,
          Font9x13::FONT,
          state.price,
          centerX,
          _y + 72,
          2,
          true)) {
    return false;
  }

  return UiText::drawCentered(
      _graphics,
      Font5x7::FONT,
      "PERP",
      centerX,
      _y + 108,
      1,
      true
  );
}
