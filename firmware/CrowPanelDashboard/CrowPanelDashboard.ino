#include <Arduino.h>
#include <cstring>

#include "CrowEPD579.h"
#include "Dashboard.h"
#include "DashboardDirty.h"
#include "DashboardState.h"
#include "DashboardStateCompare.h"
#include "DashboardStateSnapshot.h"
#include "DashboardTestStates.h"
#include "DashboardUpdateCoalescer.h"
#include "GraphicsBW.h"
#include "Icons.h"
#include "WiFiManager.h"
#include "config.h"

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
DashboardUpdateCoalescer updateCoalescer;
WiFiManager wifiManager;

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

static bool expectPendingDirty(
    const char* label,
    const DashboardUpdateCoalescer& coalescer,
    DashboardDirtyMask expected) {
  const DashboardDirtyMask actual = coalescer.pendingDirty();

  Serial.print("Coalescer ");
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

static bool runCoalescerSelfCheck() {
  DashboardUpdateCoalescer coalescer;
  const DashboardState& baseline = DashboardTestStates::BASELINE;
  const DashboardState& target = DashboardTestStates::PRE_MAINTENANCE;

  if (!coalescer.establishDisplayed(baseline)) {
    Serial.println("FAIL: coalescer baseline setup failed.");
    return false;
  }

  if (!coalescer.stageClock(target.clock) ||
      !expectPendingDirty(
          "clock",
          coalescer,
          DashboardDirty::CLOCK)) {
    return false;
  }

  if (!coalescer.stageBtc(target.btc) ||
      !expectPendingDirty(
          "clock+btc",
          coalescer,
          DashboardDirty::CLOCK | DashboardDirty::BTC)) {
    return false;
  }

  if (!coalescer.stageWiFi(target.wifi) ||
      !expectPendingDirty(
          "clock+wifi+btc",
          coalescer,
          DashboardDirty::CLOCK |
              DashboardDirty::WIFI |
              DashboardDirty::BTC)) {
    return false;
  }

  if (!coalescer.stageClock(baseline.clock) ||
      !expectPendingDirty(
          "clock reverted",
          coalescer,
          DashboardDirty::WIFI | DashboardDirty::BTC)) {
    return false;
  }

  if (!coalescer.stageWiFi(baseline.wifi) ||
      !coalescer.stageBtc(baseline.btc) ||
      !expectPendingDirty(
          "all reverted",
          coalescer,
          DashboardDirty::NONE) ||
      coalescer.hasPendingUpdate()) {
    Serial.println("FAIL: reverted coalescer should have no pending update.");
    return false;
  }

  if (!coalescer.stage(target) ||
      !expectPendingDirty(
          "full target",
          coalescer,
          DashboardDirty::ALL) ||
      !coalescer.hasPendingUpdate()) {
    return false;
  }

  if (!coalescer.commitPending() ||
      coalescer.hasPendingUpdate() ||
      coalescer.pendingDirty() != DashboardDirty::NONE) {
    Serial.println("FAIL: coalescer commit did not clear pending state.");
    return false;
  }

  return expectDirty(
      "coalescer committed target",
      coalescer.displayedState(),
      target,
      DashboardDirty::NONE
  );
}

static bool runCoalescerOwnershipSelfCheck() {
  DashboardUpdateCoalescer coalescer;
  const DashboardState& baseline = DashboardTestStates::BASELINE;
  const DashboardState& target = DashboardTestStates::PRE_MAINTENANCE;

  if (!coalescer.establishDisplayed(baseline)) {
    Serial.println("FAIL: ownership check baseline setup failed.");
    return false;
  }

  char timeBuffer[8] = "12:35";
  char btcBuffer[24] = "65240.10";
  char statusBuffer[48] = "PARTIAL ONE";

  DashboardState liveTarget = target;
  liveTarget.clock.time = timeBuffer;
  liveTarget.btc.price = btcBuffer;
  liveTarget.status.text = statusBuffer;

  if (!coalescer.stage(liveTarget)) {
    Serial.println("FAIL: ownership check stage failed.");
    return false;
  }

  strcpy(timeBuffer, "19:19");
  strcpy(btcBuffer, "99999.99");
  strcpy(statusBuffer, "MUTATED");

  if (!expectDirty(
          "coalescer pending owns staged values",
          coalescer.pendingState(),
          target,
          DashboardDirty::NONE)) {
    return false;
  }

  if (!expectDirty(
          "mutated live source differs from pending",
          coalescer.pendingState(),
          liveTarget,
          DashboardDirty::CLOCK |
              DashboardDirty::BTC |
              DashboardDirty::STATUS)) {
    return false;
  }

  coalescer.discardPending();

  if (coalescer.hasPendingUpdate() ||
      coalescer.pendingDirty() != DashboardDirty::NONE) {
    Serial.println("FAIL: discard did not clear pending state.");
    return false;
  }

  if (!expectDirty(
          "discard preserves displayed state",
          coalescer.displayedState(),
          baseline,
          DashboardDirty::NONE)) {
    return false;
  }

  if (!coalescer.stage(target) ||
      !coalescer.commitPending()) {
    Serial.println("FAIL: ownership check commit failed.");
    return false;
  }

  return expectDirty(
      "commit advances displayed state",
      coalescer.displayedState(),
      target,
      DashboardDirty::NONE
  );
}

static bool stageAndReport(
    const char* label,
    bool staged) {
  if (!staged) {
    Serial.print("FAIL: could not stage ");
    Serial.println(label);
    return false;
  }

  Serial.print("Staged ");
  Serial.print(label);
  Serial.print("; pending dirty: ");
  printDirtyMask(updateCoalescer.pendingDirty());
  return true;
}

static bool stageStateAsWidgetEvents(
    const DashboardState& target) {
  return stageAndReport(
             "clock",
             updateCoalescer.stageClock(target.clock)) &&
         stageAndReport(
             "weather",
             updateCoalescer.stageWeather(target.weather)) &&
         stageAndReport(
             "wifi",
             updateCoalescer.stageWiFi(target.wifi)) &&
         stageAndReport(
             "btc",
             updateCoalescer.stageBtc(target.btc)) &&
         stageAndReport(
             "eth",
             updateCoalescer.stageEth(target.eth)) &&
         stageAndReport(
             "hype",
             updateCoalescer.stageHype(target.hype)) &&
         stageAndReport(
             "status",
             updateCoalescer.stageStatus(target.status));
}

static UpdateResult flushPendingPartial() {
  Serial.print("Partial flush pending dirty: ");
  printDirtyMask(updateCoalescer.pendingDirty());

  if (!updateCoalescer.hasPendingUpdate()) {
    Serial.println("SKIP: no coalesced dashboard changes to refresh.");
    delay(4000);
    return UpdateResult::SKIPPED;
  }

  const DashboardState& state = updateCoalescer.pendingState();

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

  const bool committed = updateCoalescer.commitPending();
  display.sleep();

  if (!committed) {
    Serial.println("FAIL: coalesced state commit failed.");
    return UpdateResult::FAILED;
  }

  delay(4000);
  return UpdateResult::REFRESHED;
}

static UpdateResult flushPendingMaintenance() {
  Serial.print("Maintenance flush pending dirty: ");
  printDirtyMask(updateCoalescer.pendingDirty());

  if (!updateCoalescer.hasPendingUpdate()) {
    Serial.println("SKIP: no coalesced dashboard changes; maintenance not required.");
    delay(4000);
    return UpdateResult::SKIPPED;
  }

  const DashboardState& state = updateCoalescer.pendingState();

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

  const bool committed = updateCoalescer.commitPending();
  display.sleep();

  if (!committed) {
    Serial.println("FAIL: coalesced maintenance state commit failed.");
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

  const bool established = updateCoalescer.establishDisplayed(state);
  display.sleep();

  if (!established) {
    Serial.println("FAIL: coalescer baseline state capture failed.");
    return false;
  }

  delay(4000);
  return true;
}

static const char* wifiConnectionStateName(
    WiFiConnectionState state) {
  switch (state) {
    case WiFiConnectionState::DISCONNECTED:
      return "DISCONNECTED";
    case WiFiConnectionState::CONNECTING:
      return "CONNECTING";
    case WiFiConnectionState::CONNECTED:
      return "CONNECTED";
  }

  return "UNKNOWN";
}

static WiFiConnectionState lastReportedWiFiState =
    WiFiConnectionState::DISCONNECTED;
static bool hasReportedWiFiState = false;
static uint32_t lastReportedWiFiAttempt = 0;
static uint32_t lastRssiReportMs = 0;

static constexpr uint32_t WIFI_RSSI_REPORT_INTERVAL_MS = 10000;

static void reportWiFiManagerStatus() {
  const uint32_t attempts = wifiManager.connectionAttempts();

  if (attempts != lastReportedWiFiAttempt) {
    Serial.print("Wi-Fi connection attempt #");
    Serial.println(attempts);
    lastReportedWiFiAttempt = attempts;
  }

  const WiFiConnectionState currentState = wifiManager.state();

  if (!hasReportedWiFiState ||
      currentState != lastReportedWiFiState) {
    Serial.print("Wi-Fi state: ");
    Serial.println(wifiConnectionStateName(currentState));

    lastReportedWiFiState = currentState;
    hasReportedWiFiState = true;

    if (currentState == WiFiConnectionState::CONNECTED) {
      Serial.print("Wi-Fi RSSI: ");
      Serial.print(wifiManager.rssi());
      Serial.println(" dBm");
      lastRssiReportMs = millis();
    }
  }

  if (currentState != WiFiConnectionState::CONNECTED) {
    return;
  }

  const uint32_t now = millis();

  if (now - lastRssiReportMs >= WIFI_RSSI_REPORT_INTERVAL_MS) {
    Serial.print("Wi-Fi RSSI: ");
    Serial.print(wifiManager.rssi());
    Serial.println(" dBm");
    lastRssiReportMs = now;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 4A-1: Wi-Fi connection manager hardware test");
  Serial.println("E-paper display is intentionally untouched in this phase.");
  Serial.println("Credentials are loaded from local config.h and are not printed.");

  if (!wifiManager.begin(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("FAIL: Wi-Fi manager could not start. Check config.h.");
    return;
  }

  reportWiFiManagerStatus();
}

void loop() {
  wifiManager.tick();
  reportWiFiManagerStatus();
  delay(20);
}
