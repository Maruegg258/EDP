#include <Arduino.h>

#include "CrowEPD579.h"

CrowEPD579 display;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 1A: SSD1683 reset bring-up");
  Serial.println("No framebuffer write or display refresh will be performed.");

  const bool resetOk = display.begin();

  if (resetOk) {
    Serial.println("PASS: hardware reset + SWRESET completed and BUSY is idle.");
  } else {
    Serial.println("FAIL: BUSY timeout during controller reset sequence.");
  }
}

void loop() {
  delay(1000);
}
