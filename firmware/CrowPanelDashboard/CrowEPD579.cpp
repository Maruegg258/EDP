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

bool CrowEPD579::triggerFullRefresh() {
  _bus.writeCommand(0x22);
  _bus.writeData(0xF7);
  _bus.writeCommand(0x20);

  return _bus.waitUntilIdle(BUSY_TIMEOUT_MS);
}

void CrowEPD579::writePreviousWhite() {
  setMasterCursor();
  fillControllerRam(0x26, 0xFF);

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

void CrowEPD579::sleep() {
  _bus.writeCommand(0x10);
  _bus.writeData(0x01);
  delay(5);
}
