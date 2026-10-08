#pragma once

#include "DashboardActionMenu.h"
#include "GraphicsBW.h"

// UI only: compositions into the framebuffer; no display/wifi/sleep calls.
class SystemPages {
public:
  explicit SystemPages(GraphicsBW& graphics) : _graphics(graphics) {}
  bool renderMenu(DashboardAction selection);
  void renderStandby();

private:
  GraphicsBW& _graphics;
};
