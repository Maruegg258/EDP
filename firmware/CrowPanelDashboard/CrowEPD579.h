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

  // Phase 1A only:
  // power the panel, initialize the GPIO bus, perform hardware reset,
  // issue SSD1683 SWRESET (0x12), and wait for BUSY to return idle.
  //
  // This function intentionally does not write display RAM or refresh
  // the physical panel.
  bool begin();

private:
  static constexpr uint8_t CMD_SW_RESET = 0x12;
  static constexpr uint32_t BUSY_TIMEOUT_MS = 5000;

  EpaperBus _bus;
};
