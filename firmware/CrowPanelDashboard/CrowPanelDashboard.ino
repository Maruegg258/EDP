#include <Arduino.h>

#include "CrowEPD579.h"
#include "Font5x7.h"
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

static bool drawCenteredLabel(const char* text,
                              int16_t centerX,
                              int16_t y,
                              uint8_t scale = 1) {
  const uint16_t width = graphics.textWidth(Font5x7::FONT, text, scale);
  if (width == 0) {
    return false;
  }

  const int16_t x = static_cast<int16_t>(centerX - width / 2);
  return graphics.drawText(Font5x7::FONT, text, x, y, scale, true);
}

static bool buildPhase2C2Frame() {
  graphics.clear(true);

  if (!drawCenteredLabel("PROJECT ICON ASSETS", 396, 6, 2)) {
    return false;
  }

  graphics.drawLine(24, 30, 767, 30, true);

  if (!graphics.drawText(Font5x7::FONT, "WIFI", 40, 54, 2, true)) {
    return false;
  }

  constexpr int16_t WIFI_X[] = {210, 330, 450, 570};
  const Bitmap1bpp* WIFI_ICONS[] = {
    &Icons::WIFI_DISCONNECTED,
    &Icons::WIFI_WEAK,
    &Icons::WIFI_MEDIUM,
    &Icons::WIFI_STRONG
  };
  const char* WIFI_LABELS[] = {"OFF", "WEAK", "MEDIUM", "STRONG"};

  for (uint8_t i = 0; i < 4; ++i) {
    if (!graphics.drawBitmap(*WIFI_ICONS[i], WIFI_X[i] - 12, 42, true) ||
        !drawCenteredLabel(WIFI_LABELS[i], WIFI_X[i], 72, 1)) {
      return false;
    }
  }

  graphics.drawLine(24, 94, 767, 94, true);

  if (!graphics.drawText(Font5x7::FONT, "WEATHER", 40, 120, 2, true)) {
    return false;
  }

  constexpr int16_t WEATHER_X[] = {280, 420, 560};
  const Bitmap1bpp* WEATHER_ICONS[] = {
    &Icons::WEATHER_SUN,
    &Icons::WEATHER_CLOUD,
    &Icons::WEATHER_RAIN
  };
  const char* WEATHER_LABELS[] = {"SUN", "CLOUD", "RAIN"};

  for (uint8_t i = 0; i < 3; ++i) {
    if (!graphics.drawBitmap(*WEATHER_ICONS[i], WEATHER_X[i] - 16, 106, true) ||
        !drawCenteredLabel(WEATHER_LABELS[i], WEATHER_X[i], 144, 1)) {
      return false;
    }
  }

  graphics.drawLine(24, 166, 767, 166, true);

  if (!graphics.drawText(Font5x7::FONT, "CRYPTO", 40, 198, 2, true)) {
    return false;
  }

  constexpr int16_t CRYPTO_X[] = {280, 420, 560};
  const Bitmap1bpp* CRYPTO_ICONS[] = {
    &Icons::CRYPTO_BTC,
    &Icons::CRYPTO_ETH,
    &Icons::CRYPTO_HYPE
  };
  const char* CRYPTO_LABELS[] = {"BTC", "ETH", "HYPE"};

  for (uint8_t i = 0; i < 3; ++i) {
    if (!graphics.drawBitmap(*CRYPTO_ICONS[i], CRYPTO_X[i] - 16, 182, true) ||
        !drawCenteredLabel(CRYPTO_LABELS[i], CRYPTO_X[i], 222, 2)) {
      return false;
    }
  }

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 2C-2: project icon asset test");
  Serial.println("Testing Wi-Fi, weather, and crypto Bitmap1bpp assets.");

  if (!buildPhase2C2Frame()) {
    Serial.println("FAIL: Phase 2C-2 icon frame rendering failed.");
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

  Serial.println("PASS: project icon asset drawing commands and refresh completed.");
  Serial.println("Physical inspection is REQUIRED.");
  Serial.println("Check recognizability and visual integrity of all ten icons.");
}

void loop() {
  delay(1000);
}
