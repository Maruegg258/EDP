#include <Arduino.h>
#include <cstring>

#include "CrowEPD579.h"
#include "Font5x7.h"
#include "Font9x13.h"
#include "GraphicsBW.h"
#include "Icons.h"

CrowEPD579 display;

uint8_t frameBuffer[CrowEPD579::FRAMEBUFFER_BYTES];
uint8_t previousFrameBuffer[CrowEPD579::FRAMEBUFFER_BYTES];

GraphicsBW graphics(
    frameBuffer,
    CrowEPD579::FRAMEBUFFER_WIDTH,
    CrowEPD579::VISIBLE_WIDTH,
    CrowEPD579::FRAMEBUFFER_HEIGHT,
    CrowEPD579::VISIBLE_HALF_WIDTH,
    CrowEPD579::CONTROLLER_SEAM_GAP
);

struct DashboardState {
  const char* time;
  const Bitmap1bpp* weatherIcon;
  const char* weatherLabel;
  const char* temperature;
  const Bitmap1bpp* wifiIcon;
  const char* rssi;
  const char* btcPrice;
  const char* ethPrice;
  const char* hypePrice;
  const char* status;
};

static const DashboardState BASELINE = {
  "12:34",
  &Icons::WEATHER_SUN,
  "SUN",
  "28 C",
  &Icons::WIFI_STRONG,
  "-57",
  "65234.50",
  "3921.75",
  "48.26",
  "FULL BASELINE"
};

static const DashboardState PRE_MAINTENANCE = {
  "12:35",
  &Icons::WEATHER_CLOUD,
  "CLOUD",
  "27 C",
  &Icons::WIFI_MEDIUM,
  "-64",
  "65240.10",
  "3924.20",
  "48.40",
  "PARTIAL ONE"
};

static const DashboardState MAINTENANCE = {
  "12:36",
  &Icons::WEATHER_RAIN,
  "RAIN",
  "26 C",
  &Icons::WIFI_WEAK,
  "-76",
  "65210.25",
  "3918.50",
  "48.05",
  "MAINTENANCE"
};

static const DashboardState POST_MAINTENANCE[] = {
  {
    "12:37",
    &Icons::WEATHER_CLOUD,
    "CLOUD",
    "26 C",
    &Icons::WIFI_MEDIUM,
    "-66",
    "65218.80",
    "3920.10",
    "48.12",
    "POST PARTIAL ONE"
  },
  {
    "12:38",
    &Icons::WEATHER_SUN,
    "SUN",
    "27 C",
    &Icons::WIFI_STRONG,
    "-58",
    "65255.60",
    "3928.40",
    "48.55",
    "POST PARTIAL TWO"
  },
  {
    "12:39",
    &Icons::WEATHER_RAIN,
    "RAIN",
    "25 C",
    &Icons::WIFI_DISCONNECTED,
    "-99",
    "65205.15",
    "3915.25",
    "47.98",
    "POST PARTIAL THREE"
  }
};

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

static bool buildDashboardFrame(const DashboardState& state) {
  graphics.clear(true);

  graphics.drawRect(8, 8, 776, 256, true);
  graphics.drawLine(20, 68, 772, 68, true);

  if (!graphics.drawBitmap(*state.weatherIcon, 26, 20, true) ||
      !graphics.drawText(
          Font5x7::FONT,
          state.weatherLabel,
          72,
          18,
          1,
          true) ||
      !graphics.drawText(
          Font5x7::FONT,
          state.temperature,
          72,
          36,
          2,
          true)) {
    return false;
  }

  if (!drawCenteredText(
          Font9x13::FONT,
          state.time,
          CrowEPD579::VISIBLE_HALF_WIDTH,
          16,
          3,
          true)) {
    return false;
  }

  if (!graphics.drawText(Font5x7::FONT, "WIFI", 658, 18, 1, true) ||
      !graphics.drawText(Font9x13::FONT, state.rssi, 658, 34, 1, true) ||
      !graphics.drawBitmap(*state.wifiIcon, 734, 22, true)) {
    return false;
  }

  if (!drawCryptoCard(24, Icons::CRYPTO_BTC, "BTC", state.btcPrice) ||
      !drawCryptoCard(281, Icons::CRYPTO_ETH, "ETH", state.ethPrice) ||
      !drawCryptoCard(538, Icons::CRYPTO_HYPE, "HYPE", state.hypePrice)) {
    return false;
  }

  graphics.fillRect(24, 222, 744, 30, true);
  return drawCenteredText(
      Font5x7::FONT,
      state.status,
      CrowEPD579::VISIBLE_HALF_WIDTH,
      230,
      2,
      false
  );
}

static bool runNormalPartial(const DashboardState& state) {
  if (!display.begin()) {
    Serial.println("FAIL: partial wake/reset timed out.");
    return false;
  }

  if (!display.restoreFrameStateForPartial(previousFrameBuffer)) {
    Serial.println("FAIL: partial state restore timed out.");
    return false;
  }

  if (!buildDashboardFrame(state)) {
    Serial.println("FAIL: partial frame rendering failed.");
    return false;
  }

  if (!display.displayPartialFrame(frameBuffer)) {
    Serial.println("FAIL: partial refresh timed out.");
    return false;
  }

  memcpy(
      previousFrameBuffer,
      frameBuffer,
      CrowEPD579::FRAMEBUFFER_BYTES
  );
  display.sleep();
  delay(4000);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 2D-2: mixed-content refresh regression");
  Serial.println("Full baseline -> partial -> maintenance -> three partials.");
  Serial.println("Watch text, icons, card contents, seam, and white-on-black status text.");

  Serial.println("Step 1/6: full baseline...");
  if (!buildDashboardFrame(BASELINE)) {
    Serial.println("FAIL: baseline frame rendering failed.");
    return;
  }

  if (!display.begin()) {
    Serial.println("FAIL: baseline display begin/reset timed out.");
    return;
  }

  if (!display.displayFullFrame(frameBuffer)) {
    Serial.println("FAIL: baseline full refresh timed out.");
    return;
  }

  memcpy(
      previousFrameBuffer,
      frameBuffer,
      CrowEPD579::FRAMEBUFFER_BYTES
  );
  display.sleep();
  delay(4000);

  Serial.println("Step 2/6: normal partial before maintenance...");
  if (!runNormalPartial(PRE_MAINTENANCE)) {
    return;
  }

  Serial.println("Step 3/6: maintenance refresh...");
  if (!buildDashboardFrame(MAINTENANCE)) {
    Serial.println("FAIL: maintenance frame rendering failed.");
    return;
  }

  if (!display.maintenanceRefresh(frameBuffer)) {
    Serial.println("FAIL: maintenance refresh timed out.");
    return;
  }

  memcpy(
      previousFrameBuffer,
      frameBuffer,
      CrowEPD579::FRAMEBUFFER_BYTES
  );
  display.sleep();
  delay(4000);

  Serial.println("Step 4/6: first partial after maintenance...");
  if (!runNormalPartial(POST_MAINTENANCE[0])) {
    return;
  }

  Serial.println("Step 5/6: second partial after maintenance...");
  if (!runNormalPartial(POST_MAINTENANCE[1])) {
    return;
  }

  Serial.println("Step 6/6: third partial after maintenance...");
  if (!runNormalPartial(POST_MAINTENANCE[2])) {
    return;
  }

  Serial.println("PASS: Phase 2D-2 command sequence completed.");
  Serial.println("Physical inspection is REQUIRED.");
  Serial.println("Final frame should show 12:39 / RAIN / -99 / disconnected Wi-Fi.");
  Serial.println("No old text/icon should remain and no content should be blurred or doubled.");
}

void loop() {
  delay(1000);
}
