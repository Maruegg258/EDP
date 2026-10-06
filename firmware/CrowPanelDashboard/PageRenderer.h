#pragma once

#include "Dashboard.h"
#include "DashboardState.h"
#include "GraphicsBW.h"
#include "PageModel.h"

class PageRenderer {
public:
  PageRenderer(GraphicsBW& graphics, Dashboard& dashboard);

  bool render(PageId page, const DashboardState& dashboardState);

private:
  bool renderPlaceholder(const char* title);
  int16_t centeredTextX(const char* text, uint8_t scale) const;

  GraphicsBW& _graphics;
  Dashboard& _dashboard;
};
