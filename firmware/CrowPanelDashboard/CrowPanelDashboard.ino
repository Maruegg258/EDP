#include <Arduino.h>

#include "CrowEPD579.h"
#include "Font5x7.h"
#include "GraphicsBW.h"

CrowEPD579 display;

uint8_t frameBuffer[CrowEPD579::FRAMEBUFFER_BYTES];
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

static bool buildPhase1DTextFrame() {
  graphics.clear(true);

  constexpr char TEXT[] = "HELLO";
  constexpr uint8_t SCALE = 8;
  constexpr uint16_t TEXT_WIDTH =
      ((Font5x7::GLYPH_WIDTH + Font5x7::GLYPH_SPACING) * 5U -
       Font5x7::GLYPH_SPACING) * SCALE;
  constexpr uint16_t TEXT_HEIGHT =
      Font5x7::GLYPH_HEIGHT * SCALE;
  constexpr uint16_t START_X =
      (CrowEPD579::VISIBLE_WIDTH - TEXT_WIDTH) / 2U;
  constexpr uint16_t START_Y =
      (CrowEPD579::VISIBLE_HEIGHT - TEXT_HEIGHT) / 2U;

  return drawText5x7(TEXT, START_X, START_Y, SCALE);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 1D: project-owned HELLO text rendering");
  Serial.println("This test uses a minimal 5x7 font created for EDP.");

  Serial.println("Step 1/4: reset controllers...");
  if (!display.begin()) {
    Serial.println("FAIL: reset/SWRESET BUSY timeout.");
    return;
  }

  Serial.println("Step 2/4: render HELLO into the 800x272 framebuffer...");
  if (!buildPhase1DTextFrame()) {
    Serial.println("FAIL: HELLO glyph rendering failed.");
    return;
  }

  Serial.println("Step 3/4: write frame and perform full refresh...");
  if (!display.displayFullFrame(frameBuffer)) {
    Serial.println("FAIL: timeout while writing or refreshing the HELLO frame.");
    return;
  }

  Serial.println("Step 4/4: enter controller deep sleep...");
  display.sleep();

  Serial.println("PASS: command sequence completed.");
  Serial.println("Physical inspection is REQUIRED before Phase 1D is accepted.");
  Serial.println("Expected result: centered black HELLO on a white background.");
}

void loop() {
  delay(1000);
}
