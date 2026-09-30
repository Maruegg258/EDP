#include "Dashboard.h"

#include "Font5x7.h"
#include "Font9x13.h"
#include "Icons.h"

namespace {

constexpr uint16_t DASHBOARD_BORDER_WIDTH = 776;
constexpr uint16_t DASHBOARD_BORDER_HEIGHT = 256;

constexpr uint16_t CRYPTO_CARD_WIDTH = 230;
constexpr uint16_t CRYPTO_CARD_HEIGHT = 126;
constexpr int16_t CRYPTO_CARD_Y = 82;

}  // namespace

Dashboard::Dashboard(GraphicsBW& graphics)
    : _graphics(graphics) {
}

bool Dashboard::drawCenteredText(const BitmapFont& font,
                                 const char* text,
                                 int16_t centerX,
                                 int16_t y,
                                 uint8_t scale,
                                 bool black) {
  const uint16_t width = _graphics.textWidth(font, text, scale);
  if (width == 0 || width > _graphics.width()) {
    return false;
  }

  const int16_t x = static_cast<int16_t>(centerX - width / 2);
  return _graphics.drawText(font, text, x, y, scale, black);
}

bool Dashboard::drawCryptoCard(int16_t x,
                               const Bitmap1bpp& icon,
                               const char* symbol,
                               const char* price) {
  const int16_t centerX =
      static_cast<int16_t>(x + CRYPTO_CARD_WIDTH / 2);

  _graphics.drawRect(
      x,
      CRYPTO_CARD_Y,
      CRYPTO_CARD_WIDTH,
      CRYPTO_CARD_HEIGHT,
      true
  );

  if (!_graphics.drawBitmap(
          icon,
          centerX - 16,
          CRYPTO_CARD_Y + 12,
          true)) {
    return false;
  }

  if (!drawCenteredText(
          Font5x7::FONT,
          symbol,
          centerX,
          CRYPTO_CARD_Y + 50,
          2,
          true)) {
    return false;
  }

  if (!drawCenteredText(
          Font9x13::FONT,
          price,
          centerX,
          CRYPTO_CARD_Y + 72,
          2,
          true)) {
    return false;
  }

  return drawCenteredText(
      Font5x7::FONT,
      "PERP",
      centerX,
      CRYPTO_CARD_Y + 108,
      1,
      true
  );
}

bool Dashboard::render(const DashboardState& state) {
  _graphics.clear(true);

  _graphics.drawRect(
      8,
      8,
      DASHBOARD_BORDER_WIDTH,
      DASHBOARD_BORDER_HEIGHT,
      true
  );
  _graphics.drawLine(20, 68, 772, 68, true);

  if (!_graphics.drawBitmap(*state.weatherIcon, 26, 20, true) ||
      !_graphics.drawText(
          Font5x7::FONT,
          state.weatherLabel,
          72,
          18,
          1,
          true) ||
      !_graphics.drawText(
          Font5x7::FONT,
          state.temperature,
          72,
          36,
          2,
          true)) {
    return false;
  }

  const int16_t displayCenterX =
      static_cast<int16_t>(_graphics.width() / 2);

  if (!drawCenteredText(
          Font9x13::FONT,
          state.time,
          displayCenterX,
          16,
          3,
          true)) {
    return false;
  }

  if (!_graphics.drawText(
          Font5x7::FONT,
          "WIFI",
          658,
          18,
          1,
          true) ||
      !_graphics.drawText(
          Font9x13::FONT,
          state.rssi,
          658,
          34,
          1,
          true) ||
      !_graphics.drawBitmap(
          *state.wifiIcon,
          734,
          22,
          true)) {
    return false;
  }

  if (!drawCryptoCard(
          24,
          Icons::CRYPTO_BTC,
          "BTC",
          state.btcPrice) ||
      !drawCryptoCard(
          281,
          Icons::CRYPTO_ETH,
          "ETH",
          state.ethPrice) ||
      !drawCryptoCard(
          538,
          Icons::CRYPTO_HYPE,
          "HYPE",
          state.hypePrice)) {
    return false;
  }

  _graphics.fillRect(24, 222, 744, 30, true);
  return drawCenteredText(
      Font5x7::FONT,
      state.status,
      displayCenterX,
      230,
      2,
      false
  );
}
