#include "MarketPage.h"

#include "Font5x7.h"

namespace {

constexpr int16_t BORDER_X = 8;
constexpr int16_t BORDER_Y = 8;
constexpr uint16_t BORDER_WIDTH = 776;
constexpr uint16_t BORDER_HEIGHT = 256;

constexpr int16_t LABEL_X = 32;
constexpr int16_t PRICE_X = 280;
constexpr int16_t ROW1_Y = 78;
constexpr int16_t ROW2_Y = 137;
constexpr int16_t ROW3_Y = 196;

}  // namespace

MarketPage::MarketPage(GraphicsBW& graphics)
    : _graphics(graphics) {
}

bool MarketPage::renderRow(
    const char* label,
    const char* price,
    int16_t y) {
  if (label == nullptr || price == nullptr) {
    return false;
  }

  return _graphics.drawText(
             Font5x7::FONT,
             label,
             LABEL_X,
             y,
             3,
             true) &&
         _graphics.drawText(
             Font5x7::FONT,
             price,
             PRICE_X,
             y,
             3,
             true);
}

bool MarketPage::render(const MarketPageState& state) {
  _graphics.clear(true);

  _graphics.drawRect(
      BORDER_X,
      BORDER_Y,
      BORDER_WIDTH,
      BORDER_HEIGHT,
      true
  );

  if (!_graphics.drawText(
          Font5x7::FONT,
          "MARKETS",
          24,
          20,
          2,
          true) ||
      !_graphics.drawText(
          Font5x7::FONT,
          "USDT PERPETUAL",
          570,
          24,
          1,
          true)) {
    return false;
  }

  _graphics.drawLine(20, 54, 772, 54, true);
  _graphics.drawLine(20, 119, 772, 119, true);
  _graphics.drawLine(20, 178, 772, 178, true);

  return renderRow("BTC", state.btcPrice, ROW1_Y) &&
         renderRow("ETH", state.ethPrice, ROW2_Y) &&
         renderRow("HYPE", state.hypePrice, ROW3_Y);
}
