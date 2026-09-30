#include <Arduino.h>
#include <cstring>

#include "CrowEPD579.h"
#include "Dashboard.h"
#include "DashboardState.h"
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

Dashboard dashboard(graphics);

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

static bool runNormalPartial(const DashboardState& state) {
  if (!display.begin()) {
    Serial.println("FAIL: partial wake/reset timed out.");
    return false;
  }

  if (!display.restoreFrameStateForPartial(previousFrameBuffer)) {
    Serial.println("FAIL: partial state restore timed out.");
    return false;
  }

  if (!dashboard.render(state)) {
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
  Serial.println("EDP Phase 3A-1: dashboard UI extraction regression");
  Serial.println("Full baseline -> partial -> maintenance -> three partials.");
  Serial.println("The physical frame must remain identical to the Phase 2D baseline.");

  Serial.println("Step 1/6: full baseline...");
  if (!dashboard.render(BASELINE)) {
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
  if (!dashboard.render(MAINTENANCE)) {
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

  Serial.println("PASS: Phase 3A-1 command sequence completed.");
  Serial.println("Physical inspection is REQUIRED.");
  Serial.println("Final frame should show 12:39 / RAIN / -99 / disconnected Wi-Fi.");
  Serial.println("Layout, text, icons, refresh behavior, and image quality must match Phase 2D.");
}

void loop() {
  delay(1000);
}
