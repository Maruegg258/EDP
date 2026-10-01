#include "Dashboard.h"

#include "Icons.h"

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
      _hype(graphics, 538, 82, Icons::CRYPTO_HYPE, "HYPE"),
      _status(graphics, 24, 222, 744, 30) {
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

  return _weather.render(state.weather) &&
         _clock.render(state.clock) &&
         _wifi.render(state.wifi) &&
         _btc.render(state.btc) &&
         _eth.render(state.eth) &&
         _hype.render(state.hype) &&
         _status.render(state.status);
}
