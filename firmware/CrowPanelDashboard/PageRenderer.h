#pragma once

#include "Dashboard.h"
#include "DashboardState.h"
#include "GraphicsBW.h"
#include "MarketPage.h"
#include "MarketPageState.h"
#include "PageModel.h"
#include "WeatherPage.h"
#include "WeatherService.h"

class PageRenderer {
public:
  PageRenderer(GraphicsBW& graphics, Dashboard& dashboard);

  bool render(PageId page,
              const DashboardState& dashboardState,
              const WeatherSnapshot* weatherSnapshot,
              const MarketPageState& marketState);

private:
  GraphicsBW& _graphics;
  Dashboard& _dashboard;
  WeatherPage _weatherPage;
  MarketPage _marketPage;
};
