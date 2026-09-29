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

static bool drawCenteredText(const char* text,
                             int16_t y,
                             uint8_t scale) {
  const uint16_t textWidth = graphics.textWidth(Font5x7::FONT, text, scale);
  if (textWidth == 0 || textWidth > graphics.width()) {
    return false;
  }

  const int16_t x = static_cast<int16_t>(
      (graphics.width() - textWidth) / 2U
  );
  return graphics.drawText(Font5x7::FONT, text, x, y, scale, true);
}

static bool buildPhase2B2Frame() {
  graphics.clear(true);

  if (!drawCenteredText("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 8, 2)) {
    return false;
  }
  if (!drawCenteredText("0123456789  : . - + % / ( )", 30, 2)) {
    return false;
  }

  graphics.drawLine(24, 53, 767, 53, true);

  if (!drawCenteredText("12:34", 64, 6)) {
    return false;
  }

  graphics.drawLine(24, 112, 767, 112, true);

  if (!graphics.drawText(Font5x7::FONT, "BTC 65234.50", 48, 124, 3, true)) {
    return false;
  }
  if (!graphics.drawText(Font5x7::FONT, "ETH 3921.75", 48, 154, 3, true)) {
    return false;
  }
  if (!graphics.drawText(Font5x7::FONT, "HYPE 48.26", 48, 184, 3, true)) {
    return false;
  }
  if (!graphics.drawText(
          Font5x7::FONT,
          "TEMP 28 C  WIFI -57",
          48,
          222,
          3,
          true)) {
    return false;
  }

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 2B-2: dashboard typography test");
  Serial.println("Testing A-Z, 0-9, dashboard punctuation, spacing, and multiple scales.");

  if (!buildPhase2B2Frame()) {
    Serial.println("FAIL: one or more Phase 2B-2 glyphs could not be rendered.");
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

  Serial.println("PASS: typography frame drawing commands and refresh completed.");
  Serial.println("Physical inspection is REQUIRED.");
  Serial.println("Check every alphabet/digit/punctuation glyph and the dashboard sample rows.");
}

void loop() {
  delay(1000);
}
