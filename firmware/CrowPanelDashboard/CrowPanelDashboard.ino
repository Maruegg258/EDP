#include <Arduino.h>

#include "CrowEPD579.h"
#include "Font5x7.h"
#include "GraphicsBW.h"

CrowEPD579 display;

uint8_t frameBuffer[CrowEPD579::FRAMEBUFFER_BYTES];
GraphicsBW graphics(
    frameBuffer,
    CrowEPD579::FRAMEBUFFER_WIDTH,
    CrowEPD579::VISIBLE_WIDTH,
    CrowEPD579::FRAMEBUFFER_HEIGHT,
    CrowEPD579::VISIBLE_HALF_WIDTH,
    CrowEPD579::CONTROLLER_SEAM_GAP
);

static bool buildPhase2B1Frame() {
  graphics.clear(true);

  constexpr char TEXT[] = "HELLO";
  constexpr uint8_t SCALE = 8;
  const uint16_t textWidth = graphics.textWidth(Font5x7::FONT, TEXT, SCALE);
  if (textWidth == 0) {
    return false;
  }

  const int16_t startX = static_cast<int16_t>(
      (graphics.width() - textWidth) / 2U
  );
  constexpr int16_t START_Y = 72;

  return graphics.drawText(
      Font5x7::FONT,
      TEXT,
      startX,
      START_Y,
      SCALE,
      true
  );
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 2B-1: generic BitmapFont regression test");
  Serial.println("Expected visible output: the same centered HELLO used before the font refactor.");

  if (!buildPhase2B1Frame()) {
    Serial.println("FAIL: generic BitmapFont rendering failed.");
    return;
  }

  if (!display.begin()) {
    Serial.println("FAIL: display begin/reset timed out.");
    return;
  }

  if (!display.displayFullFrame(frameBuffer)) {
    Serial.println("FAIL: full-frame refresh timed out.");
    return;
  }

  display.sleep();

  Serial.println("PASS: generic BitmapFont drawing commands and refresh completed.");
  Serial.println("Physical inspection is REQUIRED.");
  Serial.println("HELLO should match the previous 5x7 rendering with no geometry change.");
}

void loop() {
  delay(1000);
}
