#include <Arduino.h>
#include <cstring>

#include "CrowEPD579.h"
#include "Dashboard.h"
#include "DashboardDirty.h"
#include "DashboardState.h"
#include "DashboardStateCompare.h"
#include "DashboardStateSnapshot.h"
#include "DashboardTestStates.h"
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
DashboardStateSnapshot previousStateSnapshot;

enum class UpdateResult {
  FAILED,
  SKIPPED,
  REFRESHED
};

static void printDirtyMask(DashboardDirtyMask dirty) {
  Serial.print("0x");
  Serial.print(static_cast<unsigned int>(dirty), HEX);
  Serial.print(" [");

  if (dirty == DashboardDirty::NONE) {
    Serial.print("NONE");
  } else {
    bool first = true;

    struct DirtyName {
      DashboardDirtyMask bit;
      const char* name;
    };

    static const DirtyName NAMES[] = {
      { DashboardDirty::CLOCK, "CLOCK" },
      { DashboardDirty::WEATHER, "WEATHER" },
      { DashboardDirty::WIFI, "WIFI" },
      { DashboardDirty::BTC, "BTC" },
      { DashboardDirty::ETH, "ETH" },
      { DashboardDirty::HYPE, "HYPE" },
      { DashboardDirty::STATUS, "STATUS" }
    };

    for (const DirtyName& entry : NAMES) {
      if ((dirty & entry.bit) == 0) {
        continue;
      }

      if (!first) {
        Serial.print("|");
      }

      Serial.print(entry.name);
      first = false;
    }
  }

  Serial.println("]");
}

static bool expectDirty(const char* label,
                        const DashboardState& previous,
                        const DashboardState& current,
                        DashboardDirtyMask expected) {
  const DashboardDirtyMask actual =
      detectDashboardDirty(previous, current);

  Serial.print("Dirty check ");
  Serial.print(label);
  Serial.print(": ");
  printDirtyMask(actual);

  if (actual != expected) {
    Serial.print("FAIL: expected ");
    printDirtyMask(expected);
    return false;
  }

  return true;
}

static bool runDirtySelfCheck() {
  const DashboardState& baseline = DashboardTestStates::BASELINE;

  DashboardState clockOnly = baseline;
  clockOnly.clock.time = "12:35";

  DashboardState weatherOnly = baseline;
  weatherOnly.weather.icon = &Icons::WEATHER_CLOUD;

  DashboardState wifiOnly = baseline;
  wifiOnly.wifi.rssi = "-58";

  DashboardState btcOnly = baseline;
  btcOnly.btc.price = "65235.00";

  DashboardState ethOnly = baseline;
  ethOnly.eth.price = "3922.00";

  DashboardState hypeOnly = baseline;
  hypeOnly.hype.price = "48.27";

  DashboardState statusOnly = baseline;
  statusOnly.status.text = "DIRTY TEST";

  DashboardState weatherAndWifi = baseline;
  weatherAndWifi.weather.temperature = "27 C";
  weatherAndWifi.wifi.icon = &Icons::WIFI_MEDIUM;

  return expectDirty(
             "same",
             baseline,
             baseline,
             DashboardDirty::NONE) &&
         expectDirty(
             "clock",
             baseline,
             clockOnly,
             DashboardDirty::CLOCK) &&
         expectDirty(
             "weather",
             baseline,
             weatherOnly,
             DashboardDirty::WEATHER) &&
         expectDirty(
             "wifi",
             baseline,
             wifiOnly,
             DashboardDirty::WIFI) &&
         expectDirty(
             "btc",
             baseline,
             btcOnly,
             DashboardDirty::BTC) &&
         expectDirty(
             "eth",
             baseline,
             ethOnly,
             DashboardDirty::ETH) &&
         expectDirty(
             "hype",
             baseline,
             hypeOnly,
             DashboardDirty::HYPE) &&
         expectDirty(
             "status",
             baseline,
             statusOnly,
             DashboardDirty::STATUS) &&
         expectDirty(
             "weather+wifi",
             baseline,
             weatherAndWifi,
             DashboardDirty::WEATHER | DashboardDirty::WIFI);
}

static bool runSnapshotSelfCheck() {
  char timeBuffer[8] = "12:34";
  char btcBuffer[24] = "65234.50";
  char statusBuffer[48] = "FULL BASELINE";

  DashboardState liveState = DashboardTestStates::BASELINE;
  liveState.clock.time = timeBuffer;
  liveState.btc.price = btcBuffer;
  liveState.status.text = statusBuffer;

  DashboardStateSnapshot snapshot;

  if (!snapshot.capture(liveState) || !snapshot.valid()) {
    Serial.println("FAIL: initial snapshot capture failed.");
    return false;
  }

  strcpy(timeBuffer, "12:35");
  strcpy(btcBuffer, "65240.10");
  strcpy(statusBuffer, "UPDATED");

  if (!expectDirty(
          "snapshot/live-buffer mutation",
          snapshot.state(),
          liveState,
          DashboardDirty::CLOCK |
              DashboardDirty::BTC |
              DashboardDirty::STATUS)) {
    return false;
  }

  if (!snapshot.capture(liveState)) {
    Serial.println("FAIL: snapshot recapture failed.");
    return false;
  }

  return expectDirty(
      "snapshot recapture",
      snapshot.state(),
      liveState,
      DashboardDirty::NONE
  );
}

static DashboardDirtyMask dirtyAgainstPrevious(
    const DashboardState& state) {
  if (!previousStateSnapshot.valid()) {
    return DashboardDirty::ALL;
  }

  return detectDashboardDirty(
      previousStateSnapshot.state(),
      state
  );
}

static bool capturePreviousState(const DashboardState& state) {
  if (!previousStateSnapshot.capture(state)) {
    Serial.println("FAIL: previous-state snapshot capture failed.");
    return false;
  }

  return true;
}

static UpdateResult runPartialIfChanged(const DashboardState& state) {
  const DashboardDirtyMask dirty = dirtyAgainstPrevious(state);

  Serial.print("Partial candidate dirty: ");
  printDirtyMask(dirty);

  if (dirty == DashboardDirty::NONE) {
    Serial.println("SKIP: state unchanged; display remains asleep.");
    delay(4000);
    return UpdateResult::SKIPPED;
  }

  if (!display.begin()) {
    Serial.println("FAIL: partial wake/reset timed out.");
    return UpdateResult::FAILED;
  }

  if (!display.restoreFrameStateForPartial(previousFrameBuffer)) {
    Serial.println("FAIL: partial state restore timed out.");
    return UpdateResult::FAILED;
  }

  if (!dashboard.render(state)) {
    Serial.println("FAIL: partial frame rendering failed.");
    return UpdateResult::FAILED;
  }

  if (!display.displayPartialFrame(frameBuffer)) {
    Serial.println("FAIL: partial refresh timed out.");
    return UpdateResult::FAILED;
  }

  memcpy(
      previousFrameBuffer,
      frameBuffer,
      CrowEPD579::FRAMEBUFFER_BYTES
  );

  const bool snapshotCaptured = capturePreviousState(state);
  display.sleep();

  if (!snapshotCaptured) {
    return UpdateResult::FAILED;
  }

  delay(4000);
  return UpdateResult::REFRESHED;
}

static UpdateResult runMaintenanceIfChanged(
    const DashboardState& state) {
  const DashboardDirtyMask dirty = dirtyAgainstPrevious(state);

  Serial.print("Maintenance candidate dirty: ");
  printDirtyMask(dirty);

  if (dirty == DashboardDirty::NONE) {
    Serial.println("SKIP: state unchanged; maintenance refresh not required.");
    delay(4000);
    return UpdateResult::SKIPPED;
  }

  if (!dashboard.render(state)) {
    Serial.println("FAIL: maintenance frame rendering failed.");
    return UpdateResult::FAILED;
  }

  if (!display.maintenanceRefresh(frameBuffer)) {
    Serial.println("FAIL: maintenance refresh timed out.");
    return UpdateResult::FAILED;
  }

  memcpy(
      previousFrameBuffer,
      frameBuffer,
      CrowEPD579::FRAMEBUFFER_BYTES
  );

  const bool snapshotCaptured = capturePreviousState(state);
  display.sleep();

  if (!snapshotCaptured) {
    return UpdateResult::FAILED;
  }

  delay(4000);
  return UpdateResult::REFRESHED;
}

static bool expectUpdateResult(const char* label,
                               UpdateResult actual,
                               UpdateResult expected) {
  if (actual == expected) {
    return true;
  }

  Serial.print("FAIL: unexpected update result for ");
  Serial.println(label);
  return false;
}

static bool establishBaseline(const DashboardState& state) {
  if (!dashboard.render(state)) {
    Serial.println("FAIL: baseline frame rendering failed.");
    return false;
  }

  if (!display.begin()) {
    Serial.println("FAIL: baseline display begin/reset timed out.");
    return false;
  }

  if (!display.displayFullFrame(frameBuffer)) {
    Serial.println("FAIL: baseline full refresh timed out.");
    return false;
  }

  memcpy(
      previousFrameBuffer,
      frameBuffer,
      CrowEPD579::FRAMEBUFFER_BYTES
  );

  const bool snapshotCaptured = capturePreviousState(state);
  display.sleep();

  if (!snapshotCaptured) {
    return false;
  }

  delay(4000);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 3C-3: skip unchanged dashboard refreshes");
  Serial.println("Dirty NONE skips rendering and all display wake/refresh work.");

  if (!runDirtySelfCheck()) {
    Serial.println("FAIL: dirty comparison self-check failed.");
    return;
  }

  Serial.println("PASS: dirty comparison self-check.");

  if (!runSnapshotSelfCheck()) {
    Serial.println("FAIL: snapshot self-check failed.");
    return;
  }

  Serial.println("PASS: durable snapshot self-check.");
  Serial.println("Regression includes unchanged-state SKIP checks.");

  Serial.println("Step 1/10: establish full baseline...");
  if (!establishBaseline(DashboardTestStates::BASELINE)) {
    return;
  }

  Serial.println("Step 2/10: repeat baseline; MUST SKIP...");
  if (!expectUpdateResult(
          "repeat baseline",
          runPartialIfChanged(DashboardTestStates::BASELINE),
          UpdateResult::SKIPPED)) {
    return;
  }

  Serial.println("Step 3/10: changed state; normal partial...");
  if (!expectUpdateResult(
          "pre-maintenance partial",
          runPartialIfChanged(DashboardTestStates::PRE_MAINTENANCE),
          UpdateResult::REFRESHED)) {
    return;
  }

  Serial.println("Step 4/10: repeat partial state; MUST SKIP...");
  if (!expectUpdateResult(
          "repeat pre-maintenance",
          runPartialIfChanged(DashboardTestStates::PRE_MAINTENANCE),
          UpdateResult::SKIPPED)) {
    return;
  }

  Serial.println("Step 5/10: changed state; maintenance refresh...");
  if (!expectUpdateResult(
          "maintenance",
          runMaintenanceIfChanged(DashboardTestStates::MAINTENANCE),
          UpdateResult::REFRESHED)) {
    return;
  }

  Serial.println("Step 6/10: repeat maintenance state; MUST SKIP...");
  if (!expectUpdateResult(
          "repeat maintenance",
          runMaintenanceIfChanged(DashboardTestStates::MAINTENANCE),
          UpdateResult::SKIPPED)) {
    return;
  }

  Serial.println("Step 7/10: first partial after maintenance...");
  if (!expectUpdateResult(
          "post partial one",
          runPartialIfChanged(DashboardTestStates::POST_MAINTENANCE[0]),
          UpdateResult::REFRESHED)) {
    return;
  }

  Serial.println("Step 8/10: second partial after maintenance...");
  if (!expectUpdateResult(
          "post partial two",
          runPartialIfChanged(DashboardTestStates::POST_MAINTENANCE[1]),
          UpdateResult::REFRESHED)) {
    return;
  }

  Serial.println("Step 9/10: third partial after maintenance...");
  if (!expectUpdateResult(
          "post partial three",
          runPartialIfChanged(DashboardTestStates::POST_MAINTENANCE[2]),
          UpdateResult::REFRESHED)) {
    return;
  }

  Serial.println("Step 10/10: repeat final state; MUST SKIP...");
  if (!expectUpdateResult(
          "repeat final",
          runPartialIfChanged(DashboardTestStates::POST_MAINTENANCE[2]),
          UpdateResult::SKIPPED)) {
    return;
  }

  Serial.println("PASS: Phase 3C-3 application dirty-gate regression completed.");
  Serial.println("Physical inspection is REQUIRED.");
  Serial.println("Steps 2, 4, 6, and 10 must cause NO E-paper refresh activity.");
  Serial.println("Final frame should show 12:39 / RAIN / -99 / disconnected Wi-Fi.");
  Serial.println("Changed-state refresh quality must match Phase 3C-2.");
}

void loop() {
  delay(1000);
}
