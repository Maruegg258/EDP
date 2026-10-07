#include "PageRenderer.h"


PageRenderer::PageRenderer(
    GraphicsBW& graphics,
    Dashboard& dashboard)
    : _graphics(graphics),
      _dashboard(dashboard),
      _weatherPage(graphics),
      _marketPage(graphics) {
}

bool PageRenderer::render(
    PageId page,
    const DashboardState& dashboardState,
    const WeatherSnapshot* weatherSnapshot,
    const MarketPageState& marketState) {
  switch (page) {
    case PageId::DASHBOARD:
      return _dashboard.render(dashboardState);

    case PageId::WEATHER:
      return _weatherPage.render(weatherSnapshot);

    case PageId::MARKETS:
      return _marketPage.render(marketState);
  }

  return false;
}

