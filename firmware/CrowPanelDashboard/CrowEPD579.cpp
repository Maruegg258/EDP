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

  if (!_bus.waitUntilIdle(BUSY_TIMEOUT_MS)) {
    return false;
  }

  return true;
}
