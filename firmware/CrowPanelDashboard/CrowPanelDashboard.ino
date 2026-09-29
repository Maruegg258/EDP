#include <Arduino.h>

#include "CrowEPD579.h"
#include "Font5x7.h"
#include "Font9x13.h"
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

static bool drawCenteredText(const BitmapFont& font,
                             const char* text,
                             int16_t y,
                             uint8_t scale) {
  const uint16_t textWidth = graphics.textWidth(font, text, scale);
  if (textWidth == 0 || textWidth > graphics.width()) {
    return false;
  }

  const int16_t x = static_cast<int16_t>(
      (graphics.width() - textWidth) / 2U
  );
  return graphics.drawText(font, text, x, y, scale, true);
}

static bool drawRightAlignedText(const BitmapFont& font,
                                 const char* text,
                                 int16_t rightX,
                                 int16_t y,
                                 uint8_t scale) {
  const uint16_t textWidth = graphics.textWidth(font, text, scale);
  if (textWidth == 0 || textWidth > graphics.width() ||
      rightX < static_cast<int16_t>(textWidth)) {
    return false;
  }

  const int16_t x = static_cast<int16_t>(rightX - textWidth);
  return graphics.drawText(font, text, x, y, scale, true);
}

static bool buildPhase2B3Frame() {
  graphics.clear(true);

  if (!drawCenteredText(Font5x7::FONT, "FONT5X7 + FONT9X13", 6, 2)) {
    return false;
  }
  if (!drawCenteredText(
          Font9x13::FONT,
          "0123456789 : . - + % /",
          28,
          1)) {
    return false;
  }

  graphics.drawLine(24, 48, 767, 48, true);

  if (!drawCenteredText(Font9x13::FONT, "12:34", 58, 3)) {
    return false;
  }

  graphics.drawLine(24, 106, 767, 106, true);

  if (!graphics.drawText(Font5x7::FONT, "BTC", 48, 122, 3, true) ||
      !drawRightAlignedText(Font9x13::FONT, "65234.50", 744, 116, 2)) {
    return false;
  }

  if (!graphics.drawText(Font5x7::FONT, "ETH", 48, 156, 3, true) ||
      !drawRightAlignedText(Font9x13::FONT, "3921.75", 744, 150, 2)) {
    return false;
  }

  if (!graphics.drawText(Font5x7::FONT, "HYPE", 48, 190, 3, true) ||
      !drawRightAlignedText(Font9x13::FONT, "48.26", 744, 184, 2)) {
    return false;
  }

  graphics.drawLine(24, 216, 767, 216, true);

  if (!graphics.drawText(Font5x7::FONT, "WIFI", 48, 236, 2, true) ||
      !graphics.drawText(Font9x13::FONT, "-57", 154, 226, 2, true)) {
    return false;
  }

  if (!graphics.drawText(Font5x7::FONT, "CHANGE", 350, 236, 2, true) ||
      !graphics.drawText(Font9x13::FONT, "+12.5%", 510, 226, 2, true)) {
    return false;
  }

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 2B-3: mixed-font / multi-byte glyph test");
  Serial.println("Testing Font5x7 and native Font9x13 through the same BitmapFont renderer.");

  if (!buildPhase2B3Frame()) {
    Serial.println("FAIL: mixed-font frame rendering failed.");
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

  Serial.println("PASS: mixed-font drawing commands and refresh completed.");
  Serial.println("Physical inspection is REQUIRED.");
  Serial.println("Check 9x13 digits, punctuation, right alignment, and mixed-font rows.");
}

void loop() {
  delay(1000);
}
