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

static uint16_t visibleToRawX(uint16_t x) {
  // The panel has 396 visible pixels per controller. The controller RAM
  // is 400 + 400 pixels wide, leaving an 8-pixel logical gap at the seam.
  return (x < CrowEPD579::VISIBLE_HALF_WIDTH)
      ? x
      : static_cast<uint16_t>(x + CrowEPD579::CONTROLLER_SEAM_GAP);
}

static void setVisiblePixel(uint16_t x, uint16_t y, bool black) {
  if (x >= CrowEPD579::VISIBLE_WIDTH ||
      y >= CrowEPD579::VISIBLE_HEIGHT) {
    return;
  }

  graphics.setPixel(visibleToRawX(x), y, black);
}

static void drawHorizontalLine(uint16_t x0,
                               uint16_t x1,
                               uint16_t y,
                               uint8_t thickness = 1) {
  for (uint8_t t = 0; t < thickness; ++t) {
    const uint16_t yy = static_cast<uint16_t>(y + t);
    if (yy >= CrowEPD579::VISIBLE_HEIGHT) {
      break;
    }

    for (uint16_t x = x0; x <= x1; ++x) {
      setVisiblePixel(x, yy, true);
    }
  }
}

static void drawVerticalLine(uint16_t x,
                             uint16_t y0,
                             uint16_t y1,
                             uint8_t thickness = 1) {
  for (uint8_t t = 0; t < thickness; ++t) {
    const uint16_t xx = static_cast<uint16_t>(x + t);
    if (xx >= CrowEPD579::VISIBLE_WIDTH) {
      break;
    }

    for (uint16_t y = y0; y <= y1; ++y) {
      setVisiblePixel(xx, y, true);
    }
  }
}

static void fillVisibleRect(uint16_t x0,
                            uint16_t y0,
                            uint16_t x1,
                            uint16_t y1) {
  for (uint16_t y = y0; y <= y1; ++y) {
    for (uint16_t x = x0; x <= x1; ++x) {
      setVisiblePixel(x, y, true);
    }
  }
}

static void drawVisibleRect(uint16_t x0,
                            uint16_t y0,
                            uint16_t x1,
                            uint16_t y1,
                            uint8_t thickness = 2) {
  drawHorizontalLine(x0, x1, y0, thickness);
  drawHorizontalLine(x0, x1,
                     static_cast<uint16_t>(y1 - thickness + 1),
                     thickness);
  drawVerticalLine(x0, y0, y1, thickness);
  drawVerticalLine(static_cast<uint16_t>(x1 - thickness + 1),
                   y0, y1, thickness);
}

static void buildPhase1CTestPattern() {
  graphics.clear(true);

  // Four asymmetric corner blocks verify orientation and full-area reach.
  fillVisibleRect(8, 8, 39, 39);          // top-left: 32x32
  fillVisibleRect(744, 8, 783, 31);       // top-right: 40x24
  fillVisibleRect(8, 232, 31, 263);       // bottom-left: 24x32
  fillVisibleRect(752, 224, 783, 263);     // bottom-right: 32x40

  // A horizontal line must cross the controller seam continuously.
  drawHorizontalLine(48, 743, 135, 3);

  // Two vertical reference lines on opposite halves.
  drawVerticalLine(96, 64, 207, 3);
  drawVerticalLine(693, 64, 207, 3);

  // Central rectangle crosses visible x=395/396, directly testing the
  // 396 + 8 + 396 mapping used by the dual-controller panel.
  drawVisibleRect(356, 92, 435, 179, 3);

  // Adjacent seam markers. These should appear as one 4-pixel-wide
  // vertical black bar on the physical display with no visible 8px gap.
  drawVerticalLine(394, 104, 167, 4);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 1C: black geometry + seam mapping test");
  Serial.println("This test performs one full refresh with a non-uniform frame.");

  Serial.println("Step 1/4: reset controllers...");
  if (!display.begin()) {
    Serial.println("FAIL: reset/SWRESET BUSY timeout.");
    return;
  }

  Serial.println("Step 2/4: build 792x272 visible test pattern...");
  buildPhase1CTestPattern();

  Serial.println("Step 3/4: write dual-controller frame and refresh...");
  if (!display.displayFullFrame(frameBuffer)) {
    Serial.println("FAIL: timeout while writing or refreshing the test frame.");
    return;
  }

  Serial.println("Step 4/4: enter controller deep sleep...");
  display.sleep();

  Serial.println("PASS: command sequence completed.");
  Serial.println("Physical inspection is REQUIRED before Phase 1C is accepted.");
  Serial.println("Check corners, orientation, center rectangle, and center seam.");
}

void loop() {
  delay(1000);
}
