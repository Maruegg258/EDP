#include "CrowEPD579.h"

bool CrowEPD579::begin() {
  pinMode(PIN_PANEL_POWER, OUTPUT);
  digitalWrite(PIN_PANEL_POWER, HIGH);
  delay(10);

  _bus.begin();

  if (!_bus.hardwareReset(BUSY_TIMEOUT_MS)) {
    return false;
  }

  _bus.writeCommand(CMD_SW_RESET);

  return _bus.waitUntilIdle(BUSY_TIMEOUT_MS);
}

bool CrowEPD579::configureRefreshEnvironment() {
  // Use the controller's internal temperature sensor.
  _bus.writeCommand(0x18);
  _bus.writeData(0x80);

  // Load the sensed temperature value into the waveform controller.
  _bus.writeCommand(0x22);
  _bus.writeData(0xB1);
  _bus.writeCommand(0x20);
  if (!_bus.waitUntilIdle(BUSY_TIMEOUT_MS)) {
    return false;
  }

  // Panel-specific temperature-register setup used by the verified
  // CrowPanel/SSD1683 sequence. Keep isolated here so it can be revised
  // independently after further controller-level testing.
  _bus.writeCommand(0x1A);
  _bus.writeData(0x64);
  _bus.writeData(0x00);

  _bus.writeCommand(0x22);
  _bus.writeData(0x91);
  _bus.writeCommand(0x20);
  if (!_bus.waitUntilIdle(BUSY_TIMEOUT_MS)) {
    return false;
  }

  // Border waveform control used by the known-good panel sequence.
  _bus.writeCommand(0x3C);
  _bus.writeData(0x03);

  return _bus.waitUntilIdle(BUSY_TIMEOUT_MS);
}

bool CrowEPD579::fastModeResetAndInit() {
  // Mirrors the reset + SWRESET + fast-mode environment sequence that was
  // previously verified on this panel.
  if (!begin()) {
    return false;
  }

  return configureRefreshEnvironment();
}

void CrowEPD579::setMasterWindow() {
  // Master controller: X increments while Y decrements.
  _bus.writeCommand(0x11);
  _bus.writeData(0x05);

  // X = 0..49 bytes -> 400 source pixels.
  _bus.writeCommand(0x44);
  _bus.writeData(0x00);
  _bus.writeData(0x31);

  // Y = 271..0 -> 272 gate lines.
  _bus.writeCommand(0x45);
  _bus.writeData(0x0F);
  _bus.writeData(0x01);
  _bus.writeData(0x00);
  _bus.writeData(0x00);
}

void CrowEPD579::setMasterCursor() {
  _bus.writeCommand(0x4E);
  _bus.writeData(0x00);

  _bus.writeCommand(0x4F);
  _bus.writeData(0x0F);
  _bus.writeData(0x01);
}

void CrowEPD579::setSlaveWindow() {
  // Enable cascade/slave RAM addressing used by the second SSD1683.
  _bus.writeCommand(0x91);
  _bus.writeData(0x04);

  // Slave X runs in the opposite direction: 49..0.
  _bus.writeCommand(0xC4);
  _bus.writeData(0x31);
  _bus.writeData(0x00);

  // Slave Y = 271..0.
  _bus.writeCommand(0xC5);
  _bus.writeData(0x0F);
  _bus.writeData(0x01);
  _bus.writeData(0x00);
  _bus.writeData(0x00);
}

void CrowEPD579::setSlaveCursor() {
  _bus.writeCommand(0xCE);
  _bus.writeData(0x31);

  _bus.writeCommand(0xCF);
  _bus.writeData(0x0F);
  _bus.writeData(0x01);
}

void CrowEPD579::fillControllerRam(uint8_t command, uint8_t value) {
  _bus.writeCommand(command);

  for (size_t i = 0; i < RAM_BYTES_PER_CONTROLLER; ++i) {
    _bus.writeData(value);
  }
}

void CrowEPD579::writeFramePlane(uint8_t masterCommand,
                                 uint8_t slaveCommand,
                                 const uint8_t* frameBuffer) {
  if (frameBuffer == nullptr) {
    return;
  }

  // The dual-controller panel is fed one byte-column at a time.
  // Master consumes raw framebuffer byte columns 0..49.
  setMasterWindow();
  setMasterCursor();
  _bus.writeCommand(masterCommand);

  for (uint16_t byteColumn = 0;
       byteColumn < BYTES_PER_LINE_PER_CONTROLLER;
       ++byteColumn) {
    for (uint16_t y = 0; y < CONTROLLER_HEIGHT; ++y) {
      const size_t index =
          static_cast<size_t>(y) * FRAMEBUFFER_STRIDE + byteColumn;
      _bus.writeData(frameBuffer[index]);
    }
  }

  // Slave consumes raw framebuffer byte columns 50..99. Its X addressing
  // is reversed by the slave-window setup, matching the physical cascade.
  setSlaveWindow();
  setSlaveCursor();
  _bus.writeCommand(slaveCommand);

  for (uint16_t byteColumn = BYTES_PER_LINE_PER_CONTROLLER;
       byteColumn < FRAMEBUFFER_STRIDE;
       ++byteColumn) {
    for (uint16_t y = 0; y < CONTROLLER_HEIGHT; ++y) {
      const size_t index =
          static_cast<size_t>(y) * FRAMEBUFFER_STRIDE + byteColumn;
      _bus.writeData(frameBuffer[index]);
    }
  }
}

bool CrowEPD579::triggerFullRefresh() {
  _bus.writeCommand(0x22);
  _bus.writeData(0xF7);
  _bus.writeCommand(0x20);

  return _bus.waitUntilIdle(BUSY_TIMEOUT_MS);
}

bool CrowEPD579::triggerFastRefresh() {
  _bus.writeCommand(0x22);
  _bus.writeData(0xC7);
  _bus.writeCommand(0x20);

  return _bus.waitUntilIdle(BUSY_TIMEOUT_MS);
}

bool CrowEPD579::triggerPartialRefresh() {
  _bus.writeCommand(0x22);
  _bus.writeData(0xDC);
  _bus.writeCommand(0x20);

  return _bus.waitUntilIdle(BUSY_TIMEOUT_MS);
}

bool CrowEPD579::fastClearToWhite() {
  // Known-good fast-clear preparation: current RAM white, previous RAM black.
  setMasterWindow();
  setMasterCursor();
  fillControllerRam(0x24, 0xFF);
  setMasterCursor();
  fillControllerRam(0x26, 0x00);

  setSlaveWindow();
  setSlaveCursor();
  fillControllerRam(0xA4, 0xFF);
  setSlaveCursor();
  fillControllerRam(0xA6, 0x00);

  return triggerFastRefresh();
}

void CrowEPD579::writePreviousWhite() {
  setMasterWindow();
  setMasterCursor();
  fillControllerRam(0x26, 0xFF);

  setSlaveWindow();
  setSlaveCursor();
  fillControllerRam(0xA6, 0xFF);
}

bool CrowEPD579::clearToWhiteFull() {
  if (!configureRefreshEnvironment()) {
    return false;
  }

  // Master SSD1683: establish the 400x272 window and a known RAM state.
  setMasterWindow();
  setMasterCursor();
  fillControllerRam(0x24, 0xFF);  // current image: white

  setMasterCursor();
  fillControllerRam(0x26, 0x00);  // previous plane for the full clear cycle

  // Slave SSD1683: same 400x272 RAM depth, mirrored X addressing.
  setSlaveWindow();
  setSlaveCursor();
  fillControllerRam(0xA4, 0xFF);  // current image: white

  setSlaveCursor();
  fillControllerRam(0xA6, 0x00);  // previous plane for the full clear cycle

  if (!triggerFullRefresh()) {
    return false;
  }

  // The physical panel is now our known white baseline. Synchronize
  // previous RAM to that physical state for the next incremental test.
  writePreviousWhite();

  return true;
}

bool CrowEPD579::displayFullFrame(const uint8_t* frameBuffer) {
  if (frameBuffer == nullptr) {
    return false;
  }

  if (!configureRefreshEnvironment()) {
    return false;
  }

  // Use a known previous-plane state for this full-refresh bring-up test.
  // This mirrors the clear-cycle preparation that already passed Phase 1B.
  setMasterWindow();
  setMasterCursor();
  fillControllerRam(0x26, 0x00);

  setSlaveWindow();
  setSlaveCursor();
  fillControllerRam(0xA6, 0x00);

  // Current image plane: raw 800x272 framebuffer.
  writeFramePlane(0x24, 0xA4, frameBuffer);

  if (!triggerFullRefresh()) {
    return false;
  }

  // Synchronize previous RAM to the physical result so the controller has
  // a coherent baseline for later partial-refresh work.
  writeFramePlane(0x26, 0xA6, frameBuffer);

  return true;
}

bool CrowEPD579::displayPartialFrame(const uint8_t* frameBuffer) {
  if (frameBuffer == nullptr) {
    return false;
  }

  // At entry, previous RAM is expected to describe the current physical
  // image. Only the new current image is written before the partial update.
  writeFramePlane(0x24, 0xA4, frameBuffer);

  if (!triggerPartialRefresh()) {
    return false;
  }

  // After the physical panel reaches the new image, make previous RAM
  // describe that same state before the next partial update.
  writeFramePlane(0x26, 0xA6, frameBuffer);

  return true;
}

bool CrowEPD579::restoreFrameStateForPartial(const uint8_t* previousFrame) {
  if (previousFrame == nullptr) {
    return false;
  }

  // After deep sleep + HW reset/SWRESET, controller RAM contents are treated
  // as unknown. Rebuild the refresh environment and both image planes from
  // the ESP32-owned copy of the image still visible on the panel.
  if (!configureRefreshEnvironment()) {
    return false;
  }

  writeFramePlane(0x26, 0xA6, previousFrame);
  writeFramePlane(0x24, 0xA4, previousFrame);

  return true;
}

bool CrowEPD579::maintenanceRefresh(const uint8_t* newFrame) {
  if (newFrame == nullptr) {
    return false;
  }

  // Stage 1: fast clear the physical panel to white.
  if (!fastModeResetAndInit()) {
    return false;
  }
  if (!fastClearToWhite()) {
    return false;
  }

  // Stage 2: reset/reinitialize again so the white physical state and RAM
  // baseline are established deliberately before the partial transition.
  if (!fastModeResetAndInit()) {
    return false;
  }

  writePreviousWhite();
  writeFramePlane(0x24, 0xA4, newFrame);

  if (!triggerPartialRefresh()) {
    return false;
  }

  // Keep the controller state coherent with the physical result, matching
  // the synchronization model already verified in Phase 1E.
  writeFramePlane(0x26, 0xA6, newFrame);

  return true;
}

void CrowEPD579::sleep() {
  _bus.writeCommand(0x10);
  _bus.writeData(0x01);
  delay(5);
}
