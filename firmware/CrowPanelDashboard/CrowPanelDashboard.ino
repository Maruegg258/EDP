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
#include "MarketDataService.h"
#include "SecureHttpClient.h"
#include "TimeService.h"
#include "TlsTrustAnchors.h"
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
TimeService timeService;
SecureHttpClient secureHttpClient(
    TlsTrustAnchors::DIGICERT_GLOBAL_ROOT_G2
);
MarketDataService marketDataService(secureHttpClient);

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
  wifiOnly.wifi.icon = &Icons::WIFI_MEDIUM;

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

static const Bitmap1bpp* wifiIconForRssi(int32_t rssi) {
  if (rssi >= -60) {
    return &Icons::WIFI_STRONG;
  }

  if (rssi >= -70) {
    return &Icons::WIFI_MEDIUM;
  }

  if (rssi >= -80) {
    return &Icons::WIFI_WEAK;
  }

  return &Icons::WIFI_VERY_WEAK;
}

static const char* wifiSignalLevelName(int32_t rssi) {
  if (rssi >= -60) {
    return "STRONG";
  }

  if (rssi >= -70) {
    return "MEDIUM";
  }

  if (rssi >= -80) {
    return "WEAK";
  }

  return "VERY_WEAK";
}

static WiFiConnectionState lastReportedWiFiState =
    WiFiConnectionState::DISCONNECTED;
static WiFiConnectionState lastUiWiFiState =
    WiFiConnectionState::DISCONNECTED;

static bool hasReportedWiFiState = false;
static bool hasUiWiFiState = false;

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

static bool stageLiveWiFiWidget() {
  WiFiWidgetState wifiState = {
    &Icons::WIFI_DISCONNECTED
  };

  if (wifiManager.isConnected()) {
    const int32_t currentRssi = wifiManager.rssi();
    wifiState.icon = wifiIconForRssi(currentRssi);

    Serial.print("Wi-Fi UI sample: ");
    Serial.print(currentRssi);
    Serial.print(" dBm -> ");
    Serial.println(wifiSignalLevelName(currentRssi));
  } else {
    Serial.println("Wi-Fi UI sample: DISCONNECTED");
  }

  if (!updateCoalescer.stageWiFi(wifiState)) {
    Serial.println("FAIL: could not stage live Wi-Fi widget state.");
    return false;
  }

  return true;
}

static const char* timeSyncStateName(TimeSyncState state) {
  switch (state) {
    case TimeSyncState::NOT_STARTED:
      return "NOT_STARTED";
    case TimeSyncState::WAITING_FOR_SYNC:
      return "WAITING_FOR_SYNC";
    case TimeSyncState::SYNCHRONIZED:
      return "SYNCHRONIZED";
  }

  return "UNKNOWN";
}

static constexpr char TAIPEI_TIMEZONE[] = "CST-8";
static constexpr char NTP_SERVER_PRIMARY[] = "time.cloudflare.com";
static constexpr char NTP_SERVER_SECONDARY[] = "pool.ntp.org";
static constexpr uint32_t TIME_REPORT_INTERVAL_MS = 10000;

static constexpr uint32_t MARKET_POLL_INTERVAL_MS = 60000;

static bool hasRunMarketPoll = false;
static uint32_t lastMarketPollMs = 0;

static bool timeServiceStartAttempted = false;
static bool hasReportedTimeState = false;
static TimeSyncState lastReportedTimeState =
    TimeSyncState::NOT_STARTED;
static uint32_t lastTimeReportMs = 0;

static bool hasStagedClockMinute = false;
static time_t lastStagedClockMinute = 0;

static bool startTimeServiceWhenConnected() {
  if (timeServiceStartAttempted || !wifiManager.isConnected()) {
    return true;
  }

  timeServiceStartAttempted = true;

  Serial.println("Starting NTP time synchronization...");
  Serial.println("Timezone: Asia/Taipei equivalent (UTC+8, no DST)");
  Serial.print("Primary NTP server: ");
  Serial.println(NTP_SERVER_PRIMARY);
  Serial.print("Secondary NTP server: ");
  Serial.println(NTP_SERVER_SECONDARY);

  if (!timeService.begin(
          TAIPEI_TIMEZONE,
          NTP_SERVER_PRIMARY,
          NTP_SERVER_SECONDARY)) {
    Serial.println("FAIL: TimeService could not start.");
    return false;
  }

  return true;
}

static void reportTimeServiceStatus() {
  if (!timeService.started()) {
    return;
  }

  timeService.tick();

  const TimeSyncState currentState = timeService.state();

  if (!hasReportedTimeState ||
      currentState != lastReportedTimeState) {
    Serial.print("Time sync state: ");
    Serial.println(timeSyncStateName(currentState));

    lastReportedTimeState = currentState;
    hasReportedTimeState = true;
  }

  const uint32_t nowMs = millis();

  if (nowMs - lastTimeReportMs < TIME_REPORT_INTERVAL_MS) {
    return;
  }

  lastTimeReportMs = nowMs;

  if (!timeService.isSynchronized()) {
    Serial.println("Local time: waiting for NTP synchronization...");
    return;
  }

  char localTime[32];

  if (!timeService.formatLocalTime(
          localTime,
          sizeof(localTime),
          "%Y-%m-%d %H:%M:%S")) {
    Serial.println("FAIL: synchronized clock could not be formatted.");
    return;
  }

  Serial.print("Local time: ");
  Serial.print(localTime);
  Serial.print(" UTC+8 | Wi-Fi: ");
  Serial.println(
      wifiManager.isConnected() ? "CONNECTED" : "DISCONNECTED"
  );
}

static void printMarketValue(
    const char* label,
    const MarketPriceValue& value) {
  Serial.print(label);
  Serial.print(" symbol: ");
  Serial.println(value.symbol);

  Serial.print(label);
  Serial.print(" price: ");
  Serial.println(value.price);

  if (value.hasSourceTime) {
    Serial.print(label);
    Serial.print(" source time: ");
    Serial.printf(
        "%llu\n",
        static_cast<unsigned long long>(value.sourceTime)
    );
  } else {
    Serial.print(label);
    Serial.println(" source time: not present");
  }
}

static bool stageLiveMarketWidget(
    const MarketPriceValue& value) {
  const CryptoWidgetState widgetState = {
    value.price
  };

  bool staged = false;

  if (strcmp(value.symbol, "BTCUSDT") == 0) {
    staged = updateCoalescer.stageBtc(widgetState);
  } else if (strcmp(value.symbol, "ETHUSDT") == 0) {
    staged = updateCoalescer.stageEth(widgetState);
  } else if (strcmp(value.symbol, "HYPEUSDT") == 0) {
    staged = updateCoalescer.stageHype(widgetState);
  } else {
    Serial.print("FAIL: no Crypto widget mapping for market symbol ");
    Serial.println(value.symbol);
    return false;
  }

  if (!staged) {
    Serial.print("FAIL: could not stage Crypto widget for ");
    Serial.println(value.symbol);
    return false;
  }

  Serial.print("Staged live ");
  Serial.print(value.symbol);
  Serial.print("; pending dirty: ");
  printDirtyMask(updateCoalescer.pendingDirty());
  return true;
}

static void runPhase5B3MarketPollIfDue() {
  if (!wifiManager.isConnected() ||
      !timeService.isSynchronized()) {
    return;
  }

  const uint32_t now = millis();

  if (hasRunMarketPoll &&
      now - lastMarketPollMs < MARKET_POLL_INTERVAL_MS) {
    return;
  }

  hasRunMarketPoll = true;
  lastMarketPollMs = now;

  Serial.println();
  Serial.println("Phase 5B-3 production market poll starting...");

  size_t fetchSuccessCount = 0;
  size_t stageSuccessCount = 0;

  for (size_t index = 0;
       index < marketDataService.trackedSymbolCount();
       ++index) {
    const char* symbol =
        marketDataService.trackedSymbol(index);

    if (symbol == nullptr) {
      Serial.println("FAIL: configured market symbol is null.");
      continue;
    }

    Serial.print("Fetching ");
    Serial.print(symbol);
    Serial.println("...");

    if (!marketDataService.fetchLatest(symbol)) {
      Serial.print("FAIL ");
      Serial.print(symbol);
      Serial.print(": ");
      Serial.println(marketDataService.lastError(symbol));

      const MarketPriceValue* preserved =
          marketDataService.lastValidValue(symbol);

      if (preserved != nullptr) {
        printMarketValue("Preserved", *preserved);
        Serial.println(
            "UI action: keep the previously displayed price; no failure value staged."
        );
      } else {
        Serial.println(
            "No last-valid value exists; startup placeholder remains displayed."
        );
      }

      continue;
    }

    ++fetchSuccessCount;

    const MarketPriceValue* value =
        marketDataService.lastValidValue(symbol);

    if (value == nullptr) {
      Serial.print("FAIL: ");
      Serial.print(symbol);
      Serial.println(
          " fetch succeeded but no last-valid value is stored."
      );
      continue;
    }

    printMarketValue("Updated", *value);

    if (!stageLiveMarketWidget(*value)) {
      continue;
    }

    ++stageSuccessCount;
  }

  Serial.print("Market fetch result: ");
  Serial.print(fetchSuccessCount);
  Serial.print("/");
  Serial.print(marketDataService.trackedSymbolCount());
  Serial.println(" symbols updated.");

  Serial.print("Market staging result: ");
  Serial.print(stageSuccessCount);
  Serial.print("/");
  Serial.print(marketDataService.trackedSymbolCount());
  Serial.println(" symbols accepted by dashboard state.");

  Serial.print("Pending dashboard dirty after market poll: ");
  printDirtyMask(updateCoalescer.pendingDirty());

  Serial.println(
      "Market-data code does not trigger E-paper refresh directly."
  );
  Serial.println(
      "The existing application flush handles staged market changes on the next loop."
  );
  Serial.println();
}

static bool stageLiveClockIfMinuteChanged(bool& minuteChanged) {
  minuteChanged = false;

  if (!timeService.isSynchronized()) {
    return true;
  }

  const time_t currentMinute = timeService.epoch() / 60;

  if (hasStagedClockMinute &&
      currentMinute == lastStagedClockMinute) {
    return true;
  }

  char clockText[8];

  if (!timeService.formatLocalTime(
          clockText,
          sizeof(clockText),
          "%H:%M")) {
    Serial.println("FAIL: could not format live clock value.");
    return false;
  }

  const ClockWidgetState clockState = {
    clockText
  };

  if (!updateCoalescer.stageClock(clockState)) {
    Serial.println("FAIL: could not stage live Clock widget state.");
    return false;
  }

  lastStagedClockMinute = currentMinute;
  hasStagedClockMinute = true;
  minuteChanged = true;

  Serial.print("Clock minute staged: ");
  Serial.println(clockText);
  return true;
}

static bool flushLiveDashboardIfNeeded() {
  if (!updateCoalescer.hasPendingUpdate()) {
    return true;
  }

  Serial.print("Live dashboard flush dirty: ");
  printDirtyMask(updateCoalescer.pendingDirty());

  return flushPendingPartial() == UpdateResult::REFRESHED;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("EDP Phase 5B-3: production market path");
  Serial.println("Live BTC/ETH/HYPE integration remains active.");
  Serial.println("Diagnostic injection removed; normal 60-second polling only.");

  if (!runDirtySelfCheck() ||
      !runSnapshotSelfCheck() ||
      !runCoalescerSelfCheck() ||
      !runCoalescerOwnershipSelfCheck()) {
    Serial.println("FAIL: Phase 3 state/coalescing regression self-check failed.");
    return;
  }

  Serial.println("PASS: Phase 3 state/coalescing regression self-checks.");

  DashboardState initialState = DashboardTestStates::BASELINE;
  initialState.clock.time = "--:--";
  initialState.wifi.icon = &Icons::WIFI_DISCONNECTED;
  initialState.btc.price = "--";
  initialState.eth.price = "--";
  initialState.hype.price = "--";

  Serial.println("Establishing Phase 5B-3 dashboard baseline...");
  if (!establishBaseline(initialState)) {
    return;
  }

  if (!wifiManager.begin(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("FAIL: Wi-Fi manager could not start. Check config.h.");
    return;
  }

  reportWiFiManagerStatus();
}

void loop() {
  wifiManager.tick();
  reportWiFiManagerStatus();

  if (!startTimeServiceWhenConnected()) {
    delay(1000);
    return;
  }

  reportTimeServiceStatus();
  bool minuteChanged = false;

  if (!stageLiveClockIfMinuteChanged(minuteChanged)) {
    delay(1000);
    return;
  }

  const WiFiConnectionState currentWiFiState =
      wifiManager.state();

  const bool wifiStateChanged =
      !hasUiWiFiState ||
      currentWiFiState != lastUiWiFiState;

  if (wifiStateChanged || minuteChanged) {
    lastUiWiFiState = currentWiFiState;
    hasUiWiFiState = true;

    if (!stageLiveWiFiWidget()) {
      delay(1000);
      return;
    }
  }

  if (!flushLiveDashboardIfNeeded()) {
    Serial.println("FAIL: live dashboard refresh failed.");
  }

  runPhase5B3MarketPollIfDue();

  delay(20);
}
