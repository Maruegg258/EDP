#pragma once

#include <Arduino.h>

#include "InputEvent.h"
#include "PageModel.h"

enum class NavigationMode : uint8_t {
  PAGE,
  DETAIL
};

struct NavigationState {
  PageId page;
  uint8_t pageIndex;
  uint8_t pageCount;
  NavigationMode mode;
};

class NavigationController {
public:
  NavigationController();

  NavigationState state() const;
  bool handle(InputEvent event);

private:
  uint8_t _pageIndex = 0;
  NavigationMode _mode = NavigationMode::PAGE;

  void moveUp();
  void moveDown();
};
