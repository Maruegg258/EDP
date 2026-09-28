#pragma once

#include <Arduino.h>

#include "EpaperBus.h"

class CrowEPD579 {
public:
  static constexpr uint16_t FRAMEBUFFER_WIDTH = 800;
  static constexpr uint16_t FRAMEBUFFER_HEIGHT = 272;
  static constexpr uint16_t VISIBLE_WIDTH = 792;
  static constexpr uint16_t VISIBLE_HEIGHT = 272;
  static constexpr size_t FRAMEBUFFER_BYTES =
      static_cast<size_t>(FRAMEBUFFER_WIDTH) * FRAMEBUFFER_HEIGHT / 8U;

  static constexpr uint8_t PIN_PANEL_POWER = 7;

  void begin();

  // Controller-level operations will be implemented only after the
  // SSD1683 command sequence is cross-checked and tested on hardware.
  bool controllerDriverReady() const { return false; }

private:
  EpaperBus _bus;
};
