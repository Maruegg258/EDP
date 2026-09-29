#include <Arduino.h>
#include <cstring>

#include "CrowEPD579.h"
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

static bool buildPhase1FFrame(uint16_t squareX) {
  graphics.clear(true);

  constexpr char TEXT[] = "HELLO";
  constexpr uint8_t SCALE = 8;
  const uint16_t textWidth = graphics.textWidth5x7(TEXT, SCALE);
  const int16_t startX = static_cast<int16_t>(
      (graphics.width() - textWidth) / 2U
  );
  constexpr int16_t START_Y = 72;

  if (!graphics.drawText5x7(TEXT, startX, START_Y, SCALE, true)) {
    return false;
  }

  // The square is rebuilt from scratch at exactly one position each time.
  // If previous/current synchronization works, the old square must disappear.
  graphics.fillRect(static_cast<int16_t>(squareX), 208, 32, 32, true);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 2A: graphics-layer extraction regression test");
  Serial.println("Phase 1F refresh behavior is intentionally unchanged.");

  constexpr uint16_t BASELINE_X = 80;
  constexpr uint16_t PRE_MAINTENANCE_X = 240;
  constexpr uint16_t MAINTENANCE_X = 400;
  constexpr uint16_t POST_MAINTENANCE_X[] = {560, 680, 80};

  Serial.println("Step 1/5: build and full-refresh baseline...");
  if (!display.begin()) {
    Serial.println("FAIL: baseline reset/SWRESET BUSY timeout.");
    return;
  }

  if (!buildPhase1FFrame(BASELINE_X)) {
    Serial.println("FAIL: baseline rendering failed.");
    return;
  }

  if (!display.displayFullFrame(frameBuffer)) {
    Serial.println("FAIL: baseline full refresh timed out.");
    return;
  }

  memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
  display.sleep();
  delay(3000);

  Serial.println("Step 2/5: one normal partial before maintenance...");
  if (!display.begin()) {
    Serial.println("FAIL: pre-maintenance wake reset timed out.");
    return;
  }
  if (!display.restoreFrameStateForPartial(previousFrameBuffer)) {
    Serial.println("FAIL: pre-maintenance state restore timed out.");
    return;
  }
  if (!buildPhase1FFrame(PRE_MAINTENANCE_X)) {
    Serial.println("FAIL: pre-maintenance frame rendering failed.");
    return;
  }
  if (!display.displayPartialFrame(frameBuffer)) {
    Serial.println("FAIL: pre-maintenance partial refresh timed out.");
    return;
  }
  memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
  display.sleep();
  delay(3000);

  Serial.println("Step 3/5: execute maintenance refresh...");
  Serial.println("  Fast clear -> physical white -> re-init -> previous white -> current new -> partial");
  if (!buildPhase1FFrame(MAINTENANCE_X)) {
    Serial.println("FAIL: maintenance frame rendering failed.");
    return;
  }
  if (!display.maintenanceRefresh(frameBuffer)) {
    Serial.println("FAIL: maintenance refresh timed out.");
    return;
  }
  memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
  display.sleep();
  delay(3000);

  Serial.println("Step 4/5: three normal partials after maintenance...");
  for (uint8_t update = 0; update < 3; ++update) {
    Serial.print("Post-maintenance partial ");
    Serial.print(update + 1);
    Serial.println("/3...");

    if (!display.begin()) {
      Serial.println("FAIL: post-maintenance wake reset timed out.");
      return;
    }
    if (!display.restoreFrameStateForPartial(previousFrameBuffer)) {
      Serial.println("FAIL: post-maintenance state restore timed out.");
      return;
    }
    if (!buildPhase1FFrame(POST_MAINTENANCE_X[update])) {
      Serial.println("FAIL: post-maintenance frame rendering failed.");
      return;
    }
    if (!display.displayPartialFrame(frameBuffer)) {
      Serial.println("FAIL: post-maintenance partial refresh timed out.");
      return;
    }

    memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
    display.sleep();
    delay(3000);
  }

  Serial.println("Step 5/5: test sequence complete.");
  Serial.println("PASS: graphics extraction + refresh regression commands completed.");
  Serial.println("Physical inspection is REQUIRED.");
  Serial.println("HELLO and the moving square should match the Phase 1F behavior.");
  Serial.println("There should be no blurred/doubled text and only one square should remain.");
}

void loop() {
  delay(1000);
}
