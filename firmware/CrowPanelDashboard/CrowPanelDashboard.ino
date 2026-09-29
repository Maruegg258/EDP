#include <Arduino.h>

#include "CrowEPD579.h"
#include "Font5x7.h"
#include "GraphicsBW.h"
#include "TestBitmaps.h"

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

static bool buildPhase2C1Frame() {
  graphics.clear(true);

  if (!graphics.drawText(
          Font5x7::FONT,
          "BITMAP1BPP TEST",
          288,
          8,
          2,
          true)) {
    return false;
  }

  graphics.drawLine(24, 30, 767, 30, true);

  if (!graphics.drawText(Font5x7::FONT, "NORMAL", 120, 42, 2, true) ||
      !graphics.drawText(Font5x7::FONT, "SEAM X396", 340, 42, 2, true) ||
      !graphics.drawText(Font5x7::FONT, "RIGHT CLIP", 620, 42, 2, true)) {
    return false;
  }

  if (!graphics.drawBitmap(TestBitmaps::MARKER_33, 140, 68, true)) {
    return false;
  }

  constexpr int16_t SEAM_X = CrowEPD579::VISIBLE_HALF_WIDTH;
  if (!graphics.drawBitmap(
          TestBitmaps::MARKER_33,
          SEAM_X - 16,
          68,
          true)) {
    return false;
  }

  if (!graphics.drawBitmap(
          TestBitmaps::MARKER_33,
          CrowEPD579::VISIBLE_WIDTH - 17,
          68,
          true)) {
    return false;
  }

  graphics.drawLine(24, 118, 767, 118, true);

  if (!graphics.drawText(Font5x7::FONT, "33X33 5 BYTE ROWS", 72, 134, 2, true) ||
      !graphics.drawText(Font5x7::FONT, "WHITE ON BLACK", 455, 134, 2, true)) {
    return false;
  }

  if (!graphics.drawBitmap(TestBitmaps::MARKER_33, -16, 170, true)) {
    return false;
  }

  graphics.fillRect(510, 164, 120, 72, true);
  if (!graphics.drawBitmap(TestBitmaps::MARKER_33, 553, 184, false)) {
    return false;
  }

  if (!graphics.drawText(Font5x7::FONT, "LEFT CLIP", 40, 220, 2, true)) {
    return false;
  }

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 2C-1: generic Bitmap1bpp renderer test");
  Serial.println("Testing 33x33 / 5-byte rows, seam mapping, clipping, and white foreground.");

  if (!buildPhase2C1Frame()) {
    Serial.println("FAIL: Phase 2C-1 bitmap rendering failed.");
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

  Serial.println("PASS: Bitmap1bpp drawing commands and refresh completed.");
  Serial.println("Physical inspection is REQUIRED.");
  Serial.println("Check normal, seam-crossing, clipped, and white-on-black marker copies.");
}

void loop() {
  delay(1000);
}
