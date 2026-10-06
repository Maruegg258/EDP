#include <Arduino.h>
#include <cstring>

#include "ButtonManager.h"
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
#include "NavigationController.h"
#include "PageRenderer.h"
#include "SecureHttpClient.h"
#include "TimeService.h"
#include "TlsTrustAnchors.h"
#include "WeatherService.h"
#include "WeatherSnapshotCompare.h"
#include "WeatherWidgetMapper.h"
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
PageRenderer pageRenderer(graphics, dashboard);
DashboardUpdateCoalescer updateCoalescer;
ButtonManager buttonManager;

NavigationController navigationController;
PageId visiblePage = PageId::DASHBOARD;
bool visiblePageRefreshPending = false;

WeatherSnapshot lastRenderedWeatherSnapshot{};
bool hasRenderedWeatherSnapshot = false;

WiFiManager wifiManager;
TimeService timeService;
SecureHttpClient secureHttpClient(
    TlsTrustAnchors::DIGICERT_GLOBAL_ROOT_G2
);
MarketDataService marketDataService(secureHttpClient);

SecureHttpClient weatherSecureHttpClient(
    TlsTrustAnchors::ISRG_ROOT_X1
);
WeatherService weatherService(weatherSecureHttpClient);

enum class UpdateResult {
  FAILED,
  SKIPPED,
  REFRESHED
};

enum class WeatherStageResult {
  FAILED,
  UNCHANGED,
  CHANGED
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
static constexpr uint32_t WEATHER_POLL_INTERVAL_MS =
    30UL * 60UL * 1000UL;

static bool hasRunMarketPoll = false;
static uint32_t lastMarketPollMs = 0;

static bool hasRunWeatherPoll = false;
static uint32_t lastWeatherPollMs = 0;
static bool hasRunWeatherSelfChecks = false;
static bool weatherSelfChecksPassed = false;
static bool hasReportedMissingWeatherConfig = false;

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

static bool runWeatherConditionMappingSelfCheck() {
  struct MappingExpectation {
    uint8_t code;
    WeatherCondition expected;
  };

  static const MappingExpectation EXPECTATIONS[] = {
    { 0, WeatherCondition::CLEAR },
    { 1, WeatherCondition::MAINLY_CLEAR },
    { 2, WeatherCondition::PARTLY_CLOUDY },
    { 3, WeatherCondition::OVERCAST },
    { 45, WeatherCondition::FOG },
    { 48, WeatherCondition::FOG },
    { 51, WeatherCondition::DRIZZLE },
    { 53, WeatherCondition::DRIZZLE },
    { 55, WeatherCondition::DRIZZLE },
    { 56, WeatherCondition::FREEZING_DRIZZLE },
    { 57, WeatherCondition::FREEZING_DRIZZLE },
    { 61, WeatherCondition::RAIN },
    { 63, WeatherCondition::RAIN },
    { 65, WeatherCondition::RAIN },
    { 66, WeatherCondition::FREEZING_RAIN },
    { 67, WeatherCondition::FREEZING_RAIN },
    { 71, WeatherCondition::SNOW },
    { 73, WeatherCondition::SNOW },
    { 75, WeatherCondition::SNOW },
    { 77, WeatherCondition::SNOW_GRAINS },
    { 80, WeatherCondition::RAIN_SHOWERS },
    { 81, WeatherCondition::RAIN_SHOWERS },
    { 82, WeatherCondition::RAIN_SHOWERS },
    { 85, WeatherCondition::SNOW_SHOWERS },
    { 86, WeatherCondition::SNOW_SHOWERS },
    { 95, WeatherCondition::THUNDERSTORM },
    { 96, WeatherCondition::THUNDERSTORM_HAIL },
    { 97, WeatherCondition::THUNDERSTORM },
    { 99, WeatherCondition::THUNDERSTORM_HAIL }
  };

  for (const MappingExpectation& entry : EXPECTATIONS) {
    const WeatherCondition actual =
        weatherConditionFromWmoCode(entry.code);

    if (actual != entry.expected) {
      Serial.print("FAIL: WMO code ");
      Serial.print(entry.code);
      Serial.print(" mapped to ");
      Serial.print(weatherConditionName(actual));
      Serial.print(", expected ");
      Serial.println(weatherConditionName(entry.expected));
      return false;
    }
  }

  if (weatherConditionFromWmoCode(4) != WeatherCondition::UNKNOWN ||
      weatherConditionFromWmoCode(98) != WeatherCondition::UNKNOWN) {
    Serial.println(
        "FAIL: undefined WMO weather codes must map to UNKNOWN."
    );
    return false;
  }

  Serial.println(
      "PASS: WeatherCondition mapping self-check "
      "(29 documented WMO codes + UNKNOWN fallback)."
  );
  return true;
}

static bool weatherPollDue(bool hasRun,
                           uint32_t lastPollMs,
                           uint32_t nowMs) {
  return !hasRun ||
         static_cast<uint32_t>(nowMs - lastPollMs) >=
             WEATHER_POLL_INTERVAL_MS;
}

static bool runWeatherPollCadenceSelfCheck() {
  const uint32_t base = 1000;
  const uint32_t almostDue =
      base + WEATHER_POLL_INTERVAL_MS - 1;
  const uint32_t exactlyDue =
      base + WEATHER_POLL_INTERVAL_MS;

  const uint32_t wrapLast = 0xFFF00000UL;
  const uint32_t wrapNow =
      static_cast<uint32_t>(
          wrapLast + WEATHER_POLL_INTERVAL_MS
      );

  if (!weatherPollDue(false, 0, base) ||
      weatherPollDue(true, base, almostDue) ||
      !weatherPollDue(true, base, exactlyDue) ||
      !weatherPollDue(true, wrapLast, wrapNow)) {
    Serial.println("FAIL: Weather 30-minute cadence self-check.");
    return false;
  }

  Serial.println(
      "PASS: Weather 30-minute cadence self-check "
      "(initial/immediate, pre-due, exact-due, millis wrap)."
  );
  return true;
}

static bool runWeatherSelfChecksIfNeeded() {
  if (hasRunWeatherSelfChecks) {
    return weatherSelfChecksPassed;
  }

  hasRunWeatherSelfChecks = true;

  weatherSelfChecksPassed =
      runWeatherConditionMappingSelfCheck() &&
      weatherWidgetMapperSelfCheck() &&
      runWeatherPollCadenceSelfCheck();

  if (!weatherSelfChecksPassed) {
    Serial.println("FAIL: weather production self-checks.");
    return false;
  }

  Serial.println(
      "PASS: WeatherWidgetMapper self-check "
      "(all WeatherCondition values fit current widget contract)."
  );
  Serial.print("Production weather polling interval: ");
  Serial.print(WEATHER_POLL_INTERVAL_MS / 60000UL);
  Serial.println(" minutes.");
  return true;
}

static WeatherStageResult fetchAndStageLiveWeather(
    double latitude,
    double longitude) {
  if (!weatherService.fetchLatest(latitude, longitude)) {
    Serial.print("Weather fetch failed: ");
    Serial.println(weatherService.lastError());

    const WeatherSnapshot* preserved =
        weatherService.lastValidSnapshot();

    if (preserved != nullptr) {
      Serial.println(
          "UI action: preserve last-valid Weather widget; "
          "no failure value staged."
      );
    } else {
      Serial.println(
          "UI action: no last-valid weather exists; "
          "startup placeholder remains."
      );
    }

    return WeatherStageResult::FAILED;
  }

  const WeatherSnapshot* snapshot =
      weatherService.lastValidSnapshot();

  if (snapshot == nullptr || !weatherService.hasValidSnapshot()) {
    Serial.println(
        "FAIL: WeatherService succeeded but no last-valid snapshot exists."
    );
    return WeatherStageResult::FAILED;
  }

  WeatherWidgetPresentation presentation{};

  if (!buildWeatherWidgetPresentation(
          snapshot->current,
          presentation)) {
    Serial.println(
        "FAIL: live current weather could not be mapped "
        "to WeatherWidgetState."
    );
    return WeatherStageResult::FAILED;
  }

  Serial.print("Weather widget mapping: ");
  Serial.print(weatherConditionName(snapshot->current.condition));
  Serial.print(" -> ");
  Serial.print(presentation.state.label);
  Serial.print(" | ");
  Serial.println(presentation.state.temperature);

  if (!updateCoalescer.stageWeather(presentation.state)) {
    Serial.println("FAIL: could not stage live Weather widget state.");
    return WeatherStageResult::FAILED;
  }

  Serial.print("Weather staging pending dirty: ");
  printDirtyMask(updateCoalescer.pendingDirty());

  if ((updateCoalescer.pendingDirty() &
       DashboardDirty::WEATHER) == 0) {
    Serial.println(
        "Weather visible state unchanged; "
        "no WEATHER refresh requested."
    );
    return WeatherStageResult::UNCHANGED;
  }

  Serial.println(
      "Weather visible state changed; "
      "WEATHER is pending for application flush."
  );
  return WeatherStageResult::CHANGED;
}

static void runProductionWeatherPollIfDue() {
  if (!wifiManager.isConnected() ||
      !timeService.isSynchronized()) {
    return;
  }

  if (!runWeatherSelfChecksIfNeeded()) {
    return;
  }

#if !defined(WEATHER_LATITUDE) || !defined(WEATHER_LONGITUDE)
  if (!hasReportedMissingWeatherConfig) {
    Serial.println(
        "SKIP: add WEATHER_LATITUDE and WEATHER_LONGITUDE "
        "to local config.h."
    );
    hasReportedMissingWeatherConfig = true;
  }
  return;
#else
  const uint32_t now = millis();

  if (!weatherPollDue(
          hasRunWeatherPoll,
          lastWeatherPollMs,
          now)) {
    return;
  }

  hasRunWeatherPoll = true;
  lastWeatherPollMs = now;

  Serial.println();
  Serial.println("Production weather poll starting...");
  Serial.println("TLS trust anchor: ISRG Root X1");
  Serial.println(
      "Weather polling is 30 minutes; "
      "display refresh remains application-owned."
  );

  const WeatherStageResult result =
      fetchAndStageLiveWeather(
          static_cast<double>(WEATHER_LATITUDE),
          static_cast<double>(WEATHER_LONGITUDE)
      );

  if (result == WeatherStageResult::FAILED) {
    Serial.println(
        "Weather poll completed with failure; "
        "next production attempt remains on the 30-minute cadence."
    );
  } else if (result == WeatherStageResult::UNCHANGED) {
    Serial.println(
        "Weather poll completed successfully with no visible change."
    );
  } else {
    Serial.println(
        "Weather poll completed successfully with a visible update staged."
    );
  }

  Serial.println(
      "WeatherService did not trigger E-paper refresh directly."
  );
  Serial.println();
#endif
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
  if (navigationController.state().page != PageId::DASHBOARD) {
    return true;
  }

  if (!updateCoalescer.hasPendingUpdate()) {
    return true;
  }

  Serial.print("Live dashboard flush dirty: ");
  printDirtyMask(updateCoalescer.pendingDirty());

  return flushPendingPartial() == UpdateResult::REFRESHED;
}

static bool flushVisiblePageChangeIfNeeded() {
  if (!visiblePageRefreshPending) {
    return true;
  }

  const NavigationState navigationState =
      navigationController.state();
  const PageId targetPage = navigationState.page;

  const DashboardState& dashboardState =
      updateCoalescer.hasPendingUpdate()
          ? updateCoalescer.pendingState()
          : updateCoalescer.displayedState();

  const WeatherSnapshot* weatherSnapshot =
      weatherService.lastValidSnapshot();

  Serial.print("Phase 7C-2 rendering page: ");
  Serial.println(PageModel::pageName(targetPage));

  if (!pageRenderer.render(
          targetPage,
          dashboardState,
          weatherSnapshot)) {
    Serial.println("FAIL: page renderer rejected selected page.");
    return false;
  }

  if (!display.begin()) {
    Serial.println("FAIL: page-switch display wake/reset timed out.");
    return false;
  }

  if (!display.restoreFrameStateForPartial(previousFrameBuffer)) {
    Serial.println("FAIL: page-switch previous-frame restore timed out.");
    return false;
  }

  if (!display.displayPartialFrame(frameBuffer)) {
    Serial.println("FAIL: page-switch partial refresh timed out.");
    return false;
  }

  memcpy(
      previousFrameBuffer,
      frameBuffer,
      CrowEPD579::FRAMEBUFFER_BYTES
  );

  if (targetPage == PageId::DASHBOARD &&
      updateCoalescer.hasPendingUpdate()) {
    if (!updateCoalescer.commitPending()) {
      display.sleep();
      Serial.println(
          "FAIL: dashboard state commit after page switch failed."
      );
      return false;
    }
  }

  display.sleep();

  visiblePage = targetPage;
  visiblePageRefreshPending = false;

  if (targetPage == PageId::WEATHER) {
    if (weatherSnapshot != nullptr) {
      lastRenderedWeatherSnapshot = *weatherSnapshot;
      hasRenderedWeatherSnapshot = true;

      Serial.println(
          "Weather page rendered from the current last-valid snapshot."
      );
    } else {
      hasRenderedWeatherSnapshot = false;

      Serial.println(
          "Weather page rendered DATA NOT READY; "
          "no last-valid snapshot exists yet."
      );
    }
  }

  Serial.print("Phase 7C-2 visible page is now: ");
  Serial.println(PageModel::pageName(visiblePage));
  return true;
}

static const char* inputEventName(InputEvent event) {
  switch (event) {
    case InputEvent::MENU:
      return "MENU (GPIO1)";
    case InputEvent::EXIT:
      return "EXIT (GPIO2)";
    case InputEvent::UP:
      return "UP (GPIO4)";
    case InputEvent::DOWN:
      return "DOWN (GPIO6)";
    case InputEvent::NONE:
      break;
  }

  return "NONE";
}

static const char* navigationModeName(NavigationMode mode) {
  switch (mode) {
    case NavigationMode::PAGE:
      return "PAGE";
    case NavigationMode::DETAIL:
      return "DETAIL";
  }

  return "UNKNOWN";
}

static void printNavigationState(
    const char* prefix,
    const NavigationState& state) {
  Serial.print(prefix);
  Serial.print(navigationModeName(state.mode));
  Serial.print(" ");
  Serial.print(PageModel::pageName(state.page));
  Serial.print(" [");
  Serial.print(static_cast<unsigned int>(state.pageIndex) + 1);
  Serial.print("/");
  Serial.print(static_cast<unsigned int>(state.pageCount));
  Serial.println("]");
}

static bool expectNavigationState(
    const char* label,
    const NavigationController& controller,
    PageId expectedPage,
    uint8_t expectedPageIndex,
    NavigationMode expectedMode) {
  const NavigationState actual = controller.state();

  Serial.print("Navigation self-check ");
  Serial.print(label);
  Serial.print(": ");
  printNavigationState("", actual);

  if (actual.page != expectedPage ||
      actual.pageIndex != expectedPageIndex ||
      actual.mode != expectedMode) {
    Serial.println("FAIL: unexpected navigation state.");
    return false;
  }

  return true;
}

static bool runNavigationSelfCheck() {
  NavigationController controller;

  if (!expectNavigationState(
          "initial",
          controller,
          PageId::DASHBOARD,
          0,
          NavigationMode::PAGE)) {
    return false;
  }

  if (!controller.handle(InputEvent::DOWN) ||
      !expectNavigationState(
          "down",
          controller,
          PageId::WEATHER,
          1,
          NavigationMode::PAGE)) {
    return false;
  }

  if (!controller.handle(InputEvent::UP) ||
      !expectNavigationState(
          "up",
          controller,
          PageId::DASHBOARD,
          0,
          NavigationMode::PAGE)) {
    return false;
  }

  if (!controller.handle(InputEvent::UP) ||
      !expectNavigationState(
          "up wrap",
          controller,
          PageId::MARKETS,
          2,
          NavigationMode::PAGE)) {
    return false;
  }

  if (!controller.handle(InputEvent::DOWN) ||
      !expectNavigationState(
          "down wrap",
          controller,
          PageId::DASHBOARD,
          0,
          NavigationMode::PAGE)) {
    return false;
  }

  if (!controller.handle(InputEvent::MENU) ||
      !expectNavigationState(
          "menu enters detail",
          controller,
          PageId::DASHBOARD,
          0,
          NavigationMode::DETAIL)) {
    return false;
  }

  if (controller.handle(InputEvent::UP) ||
      controller.handle(InputEvent::DOWN) ||
      !expectNavigationState(
          "detail ignores up/down",
          controller,
          PageId::DASHBOARD,
          0,
          NavigationMode::DETAIL)) {
    return false;
  }

  if (!controller.handle(InputEvent::EXIT) ||
      !expectNavigationState(
          "exit returns page",
          controller,
          PageId::DASHBOARD,
          0,
          NavigationMode::PAGE)) {
    return false;
  }

  if (controller.handle(InputEvent::EXIT) ||
      controller.handle(InputEvent::NONE) ||
      !expectNavigationState(
          "no-op events",
          controller,
          PageId::DASHBOARD,
          0,
          NavigationMode::PAGE)) {
    return false;
  }

  return true;
}

static void handlePanelInputIfAny() {
  const InputEvent event = buttonManager.tick();

  if (event == InputEvent::NONE) {
    return;
  }

  Serial.print("Phase 7C-2 input event: ");
  Serial.println(inputEventName(event));

  const NavigationState before = navigationController.state();
  const bool changed = navigationController.handle(event);
  const NavigationState after = navigationController.state();

  printNavigationState("Navigation before: ", before);
  printNavigationState(
      changed ? "Navigation after:  " : "Navigation unchanged: ",
      after
  );

  if (changed && before.page != after.page) {
    visiblePageRefreshPending = true;

    Serial.print("Visible page refresh queued: ");
    Serial.println(PageModel::pageName(after.page));
    return;
  }

  Serial.println(
      "No visible page change; "
      "Phase 7C-2 does not refresh the display."
  );
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  buttonManager.begin();

  Serial.println();
  Serial.println("EDP Phase 7C-2: live weather page");
  Serial.println("DASHBOARD keeps the production dashboard renderer.");
  Serial.println("WEATHER renders current, five future hours, and tomorrow.");
  Serial.println("MARKETS remains the Phase 7C-1 placeholder.");
  Serial.println("UP/DOWN page changes use the verified full-frame partial path.");
  Serial.println("MENU/EXIT mode changes remain non-rendering.");
  Serial.println("WeatherService remains data-only; application owns page refresh.");

  if (!runNavigationSelfCheck()) {
    Serial.println("FAIL: Phase 7C-2 page/navigation self-check failed.");
    return;
  }

  Serial.println("PASS: Phase 7C-2 page/navigation self-check.");

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
  initialState.weather.icon = &Icons::WEATHER_CLOUD;
  initialState.weather.label = "--";
  initialState.weather.temperature = "-- C";
  initialState.wifi.icon = &Icons::WIFI_DISCONNECTED;
  initialState.btc.price = "--";
  initialState.eth.price = "--";
  initialState.hype.price = "--";

  Serial.println("Establishing live dashboard baseline...");
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
  handlePanelInputIfAny();

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

  if (!flushVisiblePageChangeIfNeeded()) {
    Serial.println("FAIL: visible page refresh failed.");
    delay(1000);
    return;
  }

  if (!flushLiveDashboardIfNeeded()) {
    Serial.println("FAIL: live dashboard refresh failed.");
  }

  runPhase5B3MarketPollIfDue();
  runProductionWeatherPollIfDue();

  delay(20);
}
