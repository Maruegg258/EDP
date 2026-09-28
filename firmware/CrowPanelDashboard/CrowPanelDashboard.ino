#include <Arduino.h>
#include <cstring>

#include "CrowEPD579.h"
#include "Font5x7.h"
#include "GraphicsBW.h"

CrowEPD579 display;

uint8_t frameBuffer[CrowEPD579::FRAMEBUFFER_BYTES];
uint8_t previousFrameBuffer[CrowEPD579::FRAMEBUFFER_BYTES];
GraphicsBW graphics(
    frameBuffer,
    CrowEPD579::FRAMEBUFFER_WIDTH,
    CrowEPD579::FRAMEBUFFER_HEIGHT
);

static uint16_t visibleToRawX(uint16_t x) {
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

static void drawFilledVisibleRect(uint16_t x,
                                  uint16_t y,
                                  uint16_t width,
                                  uint16_t height,
                                  bool black) {
  for (uint16_t yy = 0; yy < height; ++yy) {
    for (uint16_t xx = 0; xx < width; ++xx) {
      setVisiblePixel(
          static_cast<uint16_t>(x + xx),
          static_cast<uint16_t>(y + yy),
          black
      );
    }
  }
}

static bool drawGlyph5x7(char c,
                         uint16_t x,
                         uint16_t y,
                         uint8_t scale) {
  const uint8_t* rows = Font5x7::glyph(c);
  if (rows == nullptr || scale == 0) {
    return false;
  }

  for (uint8_t row = 0; row < Font5x7::GLYPH_HEIGHT; ++row) {
    for (uint8_t col = 0; col < Font5x7::GLYPH_WIDTH; ++col) {
      const uint8_t mask = static_cast<uint8_t>(
          1U << (Font5x7::GLYPH_WIDTH - 1U - col)
      );

      if ((rows[row] & mask) != 0U) {
        drawFilledVisibleRect(
            static_cast<uint16_t>(x + col * scale),
            static_cast<uint16_t>(y + row * scale),
            scale,
            scale,
            true
        );
      }
    }
  }

  return true;
}

static bool drawText5x7(const char* text,
                        uint16_t x,
                        uint16_t y,
                        uint8_t scale) {
  if (text == nullptr || scale == 0) {
    return false;
  }

  const uint16_t advance = static_cast<uint16_t>(
      (Font5x7::GLYPH_WIDTH + Font5x7::GLYPH_SPACING) * scale
  );

  uint16_t cursorX = x;
  while (*text != '\0') {
    if (!drawGlyph5x7(*text, cursorX, y, scale)) {
      return false;
    }
    cursorX = static_cast<uint16_t>(cursorX + advance);
    ++text;
  }
  return true;
}

static bool buildPhase1EBFrame(uint16_t squareX) {
  graphics.clear(true);

  constexpr char TEXT[] = "HELLO";
  constexpr uint8_t SCALE = 8;
  constexpr uint16_t TEXT_WIDTH =
      ((Font5x7::GLYPH_WIDTH + Font5x7::GLYPH_SPACING) * 5U -
       Font5x7::GLYPH_SPACING) * SCALE;
  constexpr uint16_t START_X =
      (CrowEPD579::VISIBLE_WIDTH - TEXT_WIDTH) / 2U;
  constexpr uint16_t START_Y = 72;

  if (!drawText5x7(TEXT, START_X, START_Y, SCALE)) {
    return false;
  }

  // The square is rebuilt from scratch at exactly one position each time.
  // If previous/current synchronization works, the old square must disappear.
  drawFilledVisibleRect(squareX, 208, 32, 32, true);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 1E-B: partial refresh across sleep/reset cycles");
  Serial.println("Previous frame will be restored from ESP32 RAM after each wake.");

  constexpr uint16_t SQUARE_X[] = {80, 240, 400, 560, 680};

  Serial.println("Step 1/4: reset controllers and build baseline...");
  if (!display.begin()) {
    Serial.println("FAIL: baseline reset/SWRESET BUSY timeout.");
    return;
  }

  if (!buildPhase1EBFrame(SQUARE_X[0])) {
    Serial.println("FAIL: baseline rendering failed.");
    return;
  }

  Serial.println("Step 2/4: full-refresh baseline, save snapshot, deep sleep...");
  if (!display.displayFullFrame(frameBuffer)) {
    Serial.println("FAIL: baseline full refresh timed out.");
    return;
  }

  memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
  display.sleep();
  delay(3000);

  Serial.println("Step 3/4: run four wake/restore/partial/sleep cycles...");

  for (uint8_t update = 1; update < 5; ++update) {
    Serial.print("Wake cycle ");
    Serial.print(update);
    Serial.println("/4: HW reset + SWRESET...");

    if (!display.begin()) {
      Serial.println("FAIL: wake reset/SWRESET BUSY timeout.");
      return;
    }

    Serial.println("  Restore previous/current RAM from ESP32 snapshot...");
    if (!display.restoreFrameStateForPartial(previousFrameBuffer)) {
      Serial.println("FAIL: controller state restore timed out.");
      return;
    }

    Serial.println("  Build next frame and partial refresh...");
    if (!buildPhase1EBFrame(SQUARE_X[update])) {
      Serial.println("FAIL: frame rendering failed.");
      return;
    }

    if (!display.displayPartialFrame(frameBuffer)) {
      Serial.println("FAIL: partial refresh timed out.");
      return;
    }

    memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);

    Serial.println("  Deep sleep...");
    display.sleep();
    delay(3000);
  }

  Serial.println("Step 4/4: test sequence complete.");
  Serial.println("PASS: all four sleep/reset partial-update cycles completed.");
  Serial.println("Physical inspection is REQUIRED.");
  Serial.println("Check that only one square remains and HELLO stays crisp.");
}
void loop() {
  delay(1000);
}
