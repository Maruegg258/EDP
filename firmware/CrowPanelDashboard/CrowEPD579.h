#pragma once

#include <Arduino.h>

#include "EpaperBus.h"

class CrowEPD579 {
public:
  static constexpr uint16_t FRAMEBUFFER_WIDTH = 800;
  static constexpr uint16_t FRAMEBUFFER_HEIGHT = 272;
  static constexpr uint16_t VISIBLE_WIDTH = 792;
  static constexpr uint16_t VISIBLE_HEIGHT = 272;
  static constexpr uint16_t VISIBLE_HALF_WIDTH = 396;
  static constexpr uint16_t CONTROLLER_SEAM_GAP = 8;
  static constexpr size_t FRAMEBUFFER_BYTES =
      static_cast<size_t>(FRAMEBUFFER_WIDTH) * FRAMEBUFFER_HEIGHT / 8U;

  static constexpr uint8_t PIN_PANEL_POWER = 7;

  // Phase 1A: power, GPIO, hardware reset, SWRESET, BUSY verification.
  bool begin();

  // Phase 1B: establish a known all-white physical/display-RAM baseline.
  bool clearToWhiteFull();

  // Phase 1C: display a complete raw 800x272 controller framebuffer using
  // one full refresh. The caller is responsible for the 792->800 seam map.
  bool displayFullFrame(const uint8_t* frameBuffer);

  // Phase 1E-A: write a complete new current frame, perform a partial
  // waveform update, then synchronize previous RAM to the physical result.
  // No reset or deep sleep is performed by this method.
  bool displayPartialFrame(const uint8_t* frameBuffer);

  // Phase 1E-B: after deep sleep / hardware reset, rebuild both controller
  // RAM planes from the ESP32-owned previous framebuffer before the next
  // partial update. Do not rely on controller RAM retention across reset.
  bool restoreFrameStateForPartial(const uint8_t* previousFrame);

  // Phase 1F: known-good maintenance sequence reconstructed in our driver:
  // fast clear to physical white -> re-init -> previous RAM white ->
  // current RAM new frame -> partial refresh -> previous RAM new frame.
  bool maintenanceRefresh(const uint8_t* newFrame);

  void sleep();

private:
  static constexpr uint16_t CONTROLLER_WIDTH = 400;
  static constexpr uint16_t CONTROLLER_HEIGHT = 272;
  static constexpr uint16_t BYTES_PER_LINE_PER_CONTROLLER =
      CONTROLLER_WIDTH / 8U;
  static constexpr uint16_t FRAMEBUFFER_STRIDE =
      FRAMEBUFFER_WIDTH / 8U;
  static constexpr size_t RAM_BYTES_PER_CONTROLLER =
      static_cast<size_t>(BYTES_PER_LINE_PER_CONTROLLER) * CONTROLLER_HEIGHT;

  static constexpr uint8_t CMD_SW_RESET = 0x12;
  static constexpr uint32_t BUSY_TIMEOUT_MS = 5000;

  bool configureRefreshEnvironment();
  bool fastModeResetAndInit();
  bool fastClearToWhite();
  bool triggerFullRefresh();
  bool triggerFastRefresh();
  bool triggerPartialRefresh();

  void setMasterWindow();
  void setMasterCursor();
  void setSlaveWindow();
  void setSlaveCursor();

  void fillControllerRam(uint8_t command, uint8_t value);
  void writeFramePlane(uint8_t masterCommand,
                       uint8_t slaveCommand,
                       const uint8_t* frameBuffer);
  void writePreviousWhite();

  EpaperBus _bus;
};
