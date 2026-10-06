#pragma once

#include <Arduino.h>

#include "InputEvent.h"

enum class NavigationMode : uint8_t {
  PAGE,
  DETAIL
};

struct NavigationState {
  uint8_t pageIndex;
  uint8_t pageCount;
  NavigationMode mode;
};

class NavigationController {
public:
  explicit NavigationController(uint8_t pageCount);

  NavigationState state() const;
  bool handle(InputEvent event);

private:
  uint8_t _pageIndex = 0;
  uint8_t _pageCount = 1;
  NavigationMode _mode = NavigationMode::PAGE;

  void moveUp();
  void moveDown();
};
