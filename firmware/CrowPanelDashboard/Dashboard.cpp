#include "Dashboard.h"

#include "Font5x7.h"
#include "Icons.h"
#include "UiText.h"

namespace {

constexpr uint16_t DASHBOARD_BORDER_WIDTH = 776;
constexpr uint16_t DASHBOARD_BORDER_HEIGHT = 256;

}  // namespace

Dashboard::Dashboard(GraphicsBW& graphics)
    : _graphics(graphics),
      _weather(graphics, 26, 18),
      _clock(
          graphics,
          static_cast<int16_t>(graphics.width() / 2),
          16),
      _wifi(graphics, 658, 18),
      _btc(graphics, 24, 82, Icons::CRYPTO_BTC, "BTC"),
      _eth(graphics, 281, 82, Icons::CRYPTO_ETH, "ETH"),
      _hype(graphics, 538, 82, Icons::CRYPTO_HYPE, "HYPE") {
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

  if (!_weather.render(
          *state.weatherIcon,
          state.weatherLabel,
          state.temperature) ||
      !_clock.render(state.time) ||
      !_wifi.render(*state.wifiIcon, state.rssi) ||
      !_btc.render(state.btcPrice) ||
      !_eth.render(state.ethPrice) ||
      !_hype.render(state.hypePrice)) {
    return false;
  }

  _graphics.fillRect(24, 222, 744, 30, true);
  return UiText::drawCentered(
      _graphics,
      Font5x7::FONT,
      state.status,
      static_cast<int16_t>(_graphics.width() / 2),
      230,
      2,
      false
  );
}
