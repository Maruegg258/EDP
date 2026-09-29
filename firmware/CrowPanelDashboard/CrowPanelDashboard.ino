#include <Arduino.h>

#include "CrowEPD579.h"
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

static void buildPhase2A2Frame() {
  graphics.clear(true);

  constexpr int16_t SEAM_X = CrowEPD579::VISIBLE_HALF_WIDTH;
  constexpr int16_t RIGHT_X = CrowEPD579::VISIBLE_WIDTH - 1;
  constexpr int16_t BOTTOM_Y = CrowEPD579::FRAMEBUFFER_HEIGHT - 1;

  // 1. Exact visible-screen border: verifies all four visible edges.
  graphics.drawRect(
      0,
      0,
      CrowEPD579::VISIBLE_WIDTH,
      CrowEPD579::FRAMEBUFFER_HEIGHT,
      true
  );

  // 2. Adjacent vertical lines at x=395 and x=396 straddle the controller seam.
  graphics.drawLine(SEAM_X - 1, 16, SEAM_X - 1, BOTTOM_Y - 16, true);
  graphics.drawLine(SEAM_X, 16, SEAM_X, BOTTOM_Y - 16, true);

  // 3. A long horizontal line must cross the seam without an 8-pixel gap.
  graphics.drawLine(SEAM_X - 120, 40, SEAM_X + 120, 40, true);

  // 4. An outlined rectangle centered on the seam.
  graphics.drawRect(SEAM_X - 60, 64, 120, 56, true);

  // 5. Horizontal and vertical primitives away from the seam.
  graphics.drawLine(40, 144, 240, 144, true);
  graphics.drawLine(120, 128, 120, 224, true);
  graphics.drawRect(40, 168, 160, 56, true);

  // 6. Two diagonals cross each other and the seam.
  graphics.drawLine(SEAM_X - 96, 144, SEAM_X + 96, 232, true);
  graphics.drawLine(SEAM_X - 96, 232, SEAM_X + 96, 144, true);

  // 7. Right-side primitives provide a visual mirror/reference.
  graphics.drawLine(RIGHT_X - 240, 144, RIGHT_X - 40, 144, true);
  graphics.drawLine(RIGHT_X - 120, 128, RIGHT_X - 120, 224, true);
  graphics.drawRect(RIGHT_X - 199, 168, 160, 56, true);

  // 8. These rectangles intentionally extend beyond the visible left/right
  // edges. Only their in-bounds portions should appear; no wraparound is valid.
  graphics.drawRect(-12, 92, 36, 32, true);
  graphics.drawRect(RIGHT_X - 23, 92, 36, 32, true);

  // 9. A few explicit visible pixels around the seam exercise setPixel().
  graphics.setPixel(SEAM_X - 2, 132, true);
  graphics.setPixel(SEAM_X - 1, 132, true);
  graphics.setPixel(SEAM_X, 132, true);
  graphics.setPixel(SEAM_X + 1, 132, true);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 2A-2: graphics primitives + seam test");
  Serial.println("SSD1683 refresh behavior is unchanged; this is a static full-frame test.");

  buildPhase2A2Frame();

  if (!display.begin()) {
    Serial.println("FAIL: display begin/reset timed out.");
    return;
  }

  if (!display.displayFullFrame(frameBuffer)) {
    Serial.println("FAIL: full-frame refresh timed out.");
    return;
  }

  display.sleep();

  Serial.println("PASS: drawing commands and full-frame refresh completed.");
  Serial.println("Physical inspection is REQUIRED.");
  Serial.println("Check border, clipped side rectangles, straight lines, rectangles,");
  Serial.println("and especially continuity across visible x=396.");
}

void loop() {
  delay(1000);
}
