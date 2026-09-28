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

  // Phase 1A: power, GPIO, hardware reset, SWRESET, BUSY verification.
  bool begin();

  // Phase 1B: write both SSD1683 current/previous RAM planes for a known
  // white baseline, perform one full physical refresh, then synchronize
  // previous RAM to the resulting white panel.
  bool clearToWhiteFull();

  void sleep();

private:
  static constexpr uint16_t CONTROLLER_WIDTH = 400;
  static constexpr uint16_t CONTROLLER_HEIGHT = 272;
  static constexpr uint16_t BYTES_PER_LINE_PER_CONTROLLER =
      CONTROLLER_WIDTH / 8U;
  static constexpr size_t RAM_BYTES_PER_CONTROLLER =
      static_cast<size_t>(BYTES_PER_LINE_PER_CONTROLLER) * CONTROLLER_HEIGHT;

  static constexpr uint8_t CMD_SW_RESET = 0x12;
  static constexpr uint32_t BUSY_TIMEOUT_MS = 5000;

  bool configureRefreshEnvironment();
  bool triggerFullRefresh();

  void setMasterWindow();
  void setMasterCursor();
  void setSlaveWindow();
  void setSlaveCursor();

  void fillControllerRam(uint8_t command, uint8_t value);
  void writePreviousWhite();

  EpaperBus _bus;
};
