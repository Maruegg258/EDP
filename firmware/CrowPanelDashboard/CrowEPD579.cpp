#include "CrowEPD579.h"

void CrowEPD579::begin() {
  pinMode(PIN_PANEL_POWER, OUTPUT);
  digitalWrite(PIN_PANEL_POWER, HIGH);

  _bus.begin();
}
