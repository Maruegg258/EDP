#pragma once

#include <Arduino.h>
#include "InputEvent.h"

// Application-owned selection state. ButtonManager and the display driver
// do not know about system actions.
enum class DashboardAction : uint8_t {
  STANDBY,
  DISPLAY_CLEAN
};

class DashboardActionMenu {
public:
  void reset() { _selected = DashboardAction::STANDBY; }
  DashboardAction selected() const { return _selected; }

  bool handle(InputEvent event) {
    if (event != InputEvent::UP && event != InputEvent::DOWN) {
      return false;
    }

    _selected = (_selected == DashboardAction::STANDBY)
        ? DashboardAction::DISPLAY_CLEAN
        : DashboardAction::STANDBY;
    return true;
  }

private:
  DashboardAction _selected = DashboardAction::STANDBY;
};
