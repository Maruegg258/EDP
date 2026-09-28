#include <Arduino.h>

#include "CrowEPD579.h"

CrowEPD579 display;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 1B: dual-SSD1683 white-screen bring-up");
  Serial.println("This test will perform one physical full refresh.");

  Serial.println("Step 1/3: reset controllers...");
  if (!display.begin()) {
    Serial.println("FAIL: reset/SWRESET BUSY timeout.");
    return;
  }

  Serial.println("Step 2/3: write white RAM and refresh panel...");
  if (!display.clearToWhiteFull()) {
    Serial.println("FAIL: timeout while preparing or refreshing the panel.");
    return;
  }

  Serial.println("Step 3/3: enter controller deep sleep...");
  display.sleep();

  Serial.println("PASS: physical panel should now be uniformly white.");
  Serial.println("Inspect the full 792x272 visible area, especially the center seam.");
}

void loop() {
  delay(1000);
}
