#include <Arduino.h>
#include <cstring>

#include "CrowEPD579.h"
#include "Dashboard.h"
#include "DashboardState.h"
#include "DashboardTestStates.h"
#include "GraphicsBW.h"

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
  Serial.println("EDP Phase 3A-2: application/UI boundary regression");
  Serial.println("Full baseline -> partial -> maintenance -> three partials.");
  Serial.println("Static dashboard test content now lives outside the application sketch.");

  Serial.println("Step 1/6: full baseline...");
  if (!dashboard.render(DashboardTestStates::BASELINE)) {
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
  if (!runNormalPartial(DashboardTestStates::PRE_MAINTENANCE)) {
    return;
  }

  Serial.println("Step 3/6: maintenance refresh...");
  if (!dashboard.render(DashboardTestStates::MAINTENANCE)) {
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
  if (!runNormalPartial(DashboardTestStates::POST_MAINTENANCE[0])) {
    return;
  }

  Serial.println("Step 5/6: second partial after maintenance...");
  if (!runNormalPartial(DashboardTestStates::POST_MAINTENANCE[1])) {
    return;
  }

  Serial.println("Step 6/6: third partial after maintenance...");
  if (!runNormalPartial(DashboardTestStates::POST_MAINTENANCE[2])) {
    return;
  }

  Serial.println("PASS: Phase 3A-2 command sequence completed.");
  Serial.println("Physical inspection is REQUIRED.");
  Serial.println("Final frame should show 12:39 / RAIN / -99 / disconnected Wi-Fi.");
  Serial.println("Layout, text, icons, refresh behavior, and image quality must match Phase 3A-1.");
}

void loop() {
  delay(1000);
}
