#include <Arduino.h>

#include "CrowEPD579.h"
#include "GraphicsBW.h"

CrowEPD579 display;
uint8_t frameBuffer[CrowEPD579::FRAMEBUFFER_BYTES];
GraphicsBW graphics(
    frameBuffer,
    CrowEPD579::FRAMEBUFFER_WIDTH,
    CrowEPD579::FRAMEBUFFER_HEIGHT
);

void setup() {
  Serial.begin(115200);
  delay(500);

  display.begin();
  graphics.clear(true);

  Serial.println();
  Serial.println("EDP scaffold booted.");
  Serial.println("No SSD1683 refresh commands are enabled yet.");
  Serial.println("This build is a project skeleton, not the production firmware.");
}

void loop() {
  delay(1000);
}
