#include <Arduino.h>

#include "CrowEPD579.h"
#include "Font5x7.h"
#include "Font9x13.h"
#include "GraphicsBW.h"
#include "Icons.h"

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
                             int16_t centerX,
                             int16_t y,
                             uint8_t scale,
                             bool black = true) {
  const uint16_t width = graphics.textWidth(font, text, scale);
  if (width == 0 || width > graphics.width()) {
    return false;
  }

  const int16_t x = static_cast<int16_t>(centerX - width / 2);
  return graphics.drawText(font, text, x, y, scale, black);
}

static bool drawCryptoCard(int16_t x,
                           const Bitmap1bpp& icon,
                           const char* symbol,
                           const char* price) {
  constexpr uint16_t CARD_WIDTH = 230;
  constexpr uint16_t CARD_HEIGHT = 126;
  constexpr int16_t CARD_Y = 82;
  const int16_t centerX = static_cast<int16_t>(x + CARD_WIDTH / 2);

  graphics.drawRect(x, CARD_Y, CARD_WIDTH, CARD_HEIGHT, true);

  if (!graphics.drawBitmap(icon, centerX - 16, CARD_Y + 12, true)) {
    return false;
  }

  if (!drawCenteredText(
          Font5x7::FONT,
          symbol,
          centerX,
          CARD_Y + 50,
          2,
          true)) {
    return false;
  }

  if (!drawCenteredText(
          Font9x13::FONT,
          price,
          centerX,
          CARD_Y + 72,
          2,
          true)) {
    return false;
  }

  return drawCenteredText(
      Font5x7::FONT,
      "PERP",
      centerX,
      CARD_Y + 108,
      1,
      true
  );
}

static bool buildPhase2D1Frame() {
  graphics.clear(true);

  // Outer frame and the top separator exercise geometry across the full
  // visible width.
  graphics.drawRect(8, 8, 776, 256, true);
  graphics.drawLine(20, 68, 772, 68, true);

  // Weather summary.
  if (!graphics.drawBitmap(Icons::WEATHER_SUN, 26, 20, true) ||
      !graphics.drawText(Font5x7::FONT, "SUN", 72, 18, 1, true) ||
      !graphics.drawText(Font5x7::FONT, "28 C", 72, 36, 2, true)) {
    return false;
  }

  // The centered 9x13 clock intentionally spans visible x=396.
  if (!drawCenteredText(
          Font9x13::FONT,
          "12:34",
          CrowEPD579::VISIBLE_HALF_WIDTH,
          16,
          3,
          true)) {
    return false;
  }

  // Wi-Fi summary.
  if (!graphics.drawText(Font5x7::FONT, "WIFI", 658, 18, 1, true) ||
      !graphics.drawText(Font9x13::FONT, "-57", 658, 34, 1, true) ||
      !graphics.drawBitmap(Icons::WIFI_STRONG, 734, 22, true)) {
    return false;
  }

  // Three dashboard cards. The ETH card crosses the x=396 controller seam.
  if (!drawCryptoCard(24, Icons::CRYPTO_BTC, "BTC", "65234.50") ||
      !drawCryptoCard(281, Icons::CRYPTO_ETH, "ETH", "3921.75") ||
      !drawCryptoCard(538, Icons::CRYPTO_HYPE, "HYPE", "48.26")) {
    return false;
  }

  // Filled status bar exercises white text over a black primitive.
  graphics.fillRect(24, 222, 744, 30, true);
  if (!drawCenteredText(
          Font5x7::FONT,
          "STATIC TEST DATA",
          CrowEPD579::VISIBLE_HALF_WIDTH,
          230,
          2,
          false)) {
    return false;
  }

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 2D-1: Phase 2 graphics integration frame");
  Serial.println("Static dashboard-like test; no network services or live data.");

  if (!buildPhase2D1Frame()) {
    Serial.println("FAIL: Phase 2D-1 integration frame rendering failed.");
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

  Serial.println("PASS: Phase 2D-1 frame drawing commands and refresh completed.");
  Serial.println("Physical inspection is REQUIRED.");
  Serial.println("Check clock seam continuity, all three cards, icons, text, and status bar.");
}

void loop() {
  delay(1000);
}
