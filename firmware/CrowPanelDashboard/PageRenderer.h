#pragma once

#include "Dashboard.h"
#include "DashboardState.h"
#include "GraphicsBW.h"
#include "MarketPage.h"
#include "MarketPageState.h"
#include "Phase8DashboardFrame.h"
#include "PageModel.h"
#include "WeatherPage.h"
#include "WeatherService.h"

class PageRenderer {
public:
  PageRenderer(GraphicsBW& graphics, Dashboard& dashboard);

  // Optional application-owned live visual frame (no copy or side effects).
  // Null keeps existing Phase 7 dashboard behavior.
  void useLiveDashboardFrame(const Phase8DashboardFrame* frame) {
    _liveDashboard = frame;
  }

  bool render(PageId page,
              const DashboardState& dashboardState,
              const WeatherSnapshot* weatherSnapshot,
              const MarketPageState& marketState);

private:
  GraphicsBW& _graphics;
  Dashboard& _dashboard;
  const Phase8DashboardFrame* _liveDashboard = nullptr;
  WeatherPage _weatherPage;
  MarketPage _marketPage;
};
