#include <Arduino.h>
#include <WiFi.h>
#include <cstring>
#include "esp_sleep.h"
#include "driver/gpio.h"

#include "ApplicationResultTypes.h"
#include "ButtonManager.h"
#include "CrowEPD579.h"
#include "Dashboard.h"
#include "DashboardFontTest.h"
#include "DashboardActionMenu.h"
#include "DashboardDirty.h"
#include "DashboardState.h"
#include "DashboardStateCompare.h"
#include "DashboardStateSnapshot.h"
#include "DashboardTestStates.h"
#include "DashboardUpdateCoalescer.h"
#include "GraphicsBW.h"
#include "Icons.h"
#include "MarketDataService.h"
#include "MarketPageState.h"
#include "NavigationController.h"
#include "PageRenderer.h"
#include "Phase8A1Preview.h"
#include "Phase8CalendarPresentation.h"
#include "Phase8MarketPresentation.h"
#include "Phase8DashboardFrame.h"
#include "Phase8SixDayPresentation.h"
#include "Phase8WeatherPresentation.h"
#include "SecureHttpClient.h"
#include "SystemPages.h"
#include "TimeService.h"
#include "TlsTrustAnchors.h"
#include "WeatherService.h"
#include "WeatherSnapshotCompare.h"
#include "WeatherIconGallery.h"
#include "WeatherIconSelector.h"
#include "WeatherWidgetMapper.h"
#include "WiFiManager.h"
#include "config.h"

// These isolated boot modes must never be active simultaneously.
#if defined(EDP_PHASE8_FONT_TEST) && (EDP_PHASE8_FONT_TEST == 1) && \
    defined(EDP_PHASE8A1_PREVIEW) && (EDP_PHASE8A1_PREVIEW == 1)
#error "Choose one preview mode: EDP_PHASE8_FONT_TEST or EDP_PHASE8A1_PREVIEW."
#endif

// The weather icon gallery is a third, mutually exclusive test mode.
#if defined(EDP_PHASE8_WEATHER_ICON_TEST) && (EDP_PHASE8_WEATHER_ICON_TEST == 1) && \
    ((defined(EDP_PHASE8_FONT_TEST) && (EDP_PHASE8_FONT_TEST == 1)) || \
     (defined(EDP_PHASE8A1_PREVIEW) && (EDP_PHASE8A1_PREVIEW == 1)))
#error "Disable font test and Dashboard preview before enabling Weather Icon Gallery."
#endif

// Phase 8B-2 isolated NTP/calendar experiment. All preview modes are exclusive.
#if defined(EDP_PHASE8B2_CLOCK_TEST) && (EDP_PHASE8B2_CLOCK_TEST == 1) && \
    ((defined(EDP_PHASE8_FONT_TEST) && (EDP_PHASE8_FONT_TEST == 1)) || \
     (defined(EDP_PHASE8A1_PREVIEW) && (EDP_PHASE8A1_PREVIEW == 1)) || \
     (defined(EDP_PHASE8_WEATHER_ICON_TEST) && (EDP_PHASE8_WEATHER_ICON_TEST == 1)))
#error "Phase 8B-2 NTP test must be the only enabled preview mode."
#endif

// 8B-3 real public Binance market test; all test flags are exclusive.
#if defined(EDP_PHASE8B3_MARKET_TEST) && (EDP_PHASE8B3_MARKET_TEST == 1) && \
    ((defined(EDP_PHASE8B2_CLOCK_TEST) && (EDP_PHASE8B2_CLOCK_TEST == 1)) || \
     (defined(EDP_PHASE8_FONT_TEST) && (EDP_PHASE8_FONT_TEST == 1)) || \
     (defined(EDP_PHASE8A1_PREVIEW) && (EDP_PHASE8A1_PREVIEW == 1)) || \
     (defined(EDP_PHASE8_WEATHER_ICON_TEST) && (EDP_PHASE8_WEATHER_ICON_TEST == 1)))
#error "Phase 8B-3 market test must be the only enabled preview mode."
#endif

// 8B-4 isolated live-current-weather test; never run two test modes at once.
#if (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1)) && \
    ((defined(EDP_PHASE8B3_MARKET_TEST) && (EDP_PHASE8B3_MARKET_TEST == 1)) || \
     (defined(EDP_PHASE8B2_CLOCK_TEST) && (EDP_PHASE8B2_CLOCK_TEST == 1)) || \
     (defined(EDP_PHASE8_FONT_TEST) && (EDP_PHASE8_FONT_TEST == 1)) || \
     (defined(EDP_PHASE8A1_PREVIEW) && (EDP_PHASE8A1_PREVIEW == 1)) || \
     (defined(EDP_PHASE8_WEATHER_ICON_TEST) && (EDP_PHASE8_WEATHER_ICON_TEST == 1)))
#error "Phase 8B-4 weather test must be the only enabled preview mode."
#endif

#if (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1)) && \
    (!defined(WEATHER_LATITUDE) || !defined(WEATHER_LONGITUDE))
#error "8B-4 requires WEATHER_LATITUDE and WEATHER_LONGITUDE in ignored local config.h."
#endif

// 8B-5A opt-in Wi-Fi bitmap test: no two preview flags may be active.
#if (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1)) && \
    ((defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1)) || (defined(EDP_PHASE8B3_MARKET_TEST) && (EDP_PHASE8B3_MARKET_TEST == 1)) || \
     (defined(EDP_PHASE8B2_CLOCK_TEST) && (EDP_PHASE8B2_CLOCK_TEST == 1)) || \
     (defined(EDP_PHASE8_FONT_TEST) && (EDP_PHASE8_FONT_TEST == 1)) || \
     (defined(EDP_PHASE8A1_PREVIEW) && (EDP_PHASE8A1_PREVIEW == 1)) || \
     (defined(EDP_PHASE8_WEATHER_ICON_TEST) && (EDP_PHASE8_WEATHER_ICON_TEST == 1)))
#error "Phase 8B-5A Wi-Fi test must be the only enabled preview mode."
#endif

#if (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1)) && \
    (!defined(WEATHER_LATITUDE) || !defined(WEATHER_LONGITUDE))
#error "8B-5A requires WEATHER_LATITUDE and WEATHER_LONGITUDE in ignored local config.h."
#endif

// Phase 8B-5B production-app integration remains explicitly opt-in until
// user hardware acceptance. Unlike previews, this RUNS Phase 7 navigation.
#if (defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1)) && \
    ((defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1)) || (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1)) || (defined(EDP_PHASE8B3_MARKET_TEST) && (EDP_PHASE8B3_MARKET_TEST == 1)) || \
     (defined(EDP_PHASE8B2_CLOCK_TEST) && (EDP_PHASE8B2_CLOCK_TEST == 1)) || \
     (defined(EDP_PHASE8A1_PREVIEW) && (EDP_PHASE8A1_PREVIEW == 1)) || \
     (defined(EDP_PHASE8_FONT_TEST) && (EDP_PHASE8_FONT_TEST == 1)) || \
     (defined(EDP_PHASE8_WEATHER_ICON_TEST) && (EDP_PHASE8_WEATHER_ICON_TEST == 1)))
#error "8B-5B application mode must not be combined with isolated preview flags."
#endif
#if (defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1)) && \
    (!defined(WEATHER_LATITUDE) || !defined(WEATHER_LONGITUDE))
#error "8B-5B live Dashboard needs WEATHER_LATITUDE/LONGITUDE in ignored config.h."
#endif

// 8C-1 is an isolated SERIAL-only seven-day contract test.
// No E-paper refresh, other API poller, navigation or app behavior in mode.
#if (defined(EDP_PHASE8C1_DAILY_TEST) && (EDP_PHASE8C1_DAILY_TEST == 1)) && \
    ((defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1)) || \
     (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1)) || \
     (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1)) || \
     (defined(EDP_PHASE8B3_MARKET_TEST) && (EDP_PHASE8B3_MARKET_TEST == 1)) || \
     (defined(EDP_PHASE8B2_CLOCK_TEST) && (EDP_PHASE8B2_CLOCK_TEST == 1)) || \
     (defined(EDP_PHASE8A1_PREVIEW) && (EDP_PHASE8A1_PREVIEW == 1)) || \
     (defined(EDP_PHASE8_FONT_TEST) && (EDP_PHASE8_FONT_TEST == 1)) || \
     (defined(EDP_PHASE8_WEATHER_ICON_TEST) && (EDP_PHASE8_WEATHER_ICON_TEST == 1)))
#error "8C-1 daily contract test must be the ONLY active preview/app mode."
#endif
#if (defined(EDP_PHASE8C1_DAILY_TEST) && (EDP_PHASE8C1_DAILY_TEST == 1)) && \
    (!defined(WEATHER_LATITUDE) || !defined(WEATHER_LONGITUDE))
#error "8C-1 requires WEATHER_LATITUDE and WEATHER_LONGITUDE in local config.h."
#endif

#if (defined(EDP_PHASE8C2_DISPLAY_TEST) && (EDP_PHASE8C2_DISPLAY_TEST == 1)) && \
    ((defined(EDP_PHASE8C1_DAILY_TEST) && (EDP_PHASE8C1_DAILY_TEST == 1)) || \
     (defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1)) || \
     (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1)) || \
     (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1)) || \
     (defined(EDP_PHASE8B3_MARKET_TEST) && (EDP_PHASE8B3_MARKET_TEST == 1)) || \
     (defined(EDP_PHASE8B2_CLOCK_TEST) && (EDP_PHASE8B2_CLOCK_TEST == 1)) || \
     (defined(EDP_PHASE8A1_PREVIEW) && (EDP_PHASE8A1_PREVIEW == 1)) || \
     (defined(EDP_PHASE8_FONT_TEST) && (EDP_PHASE8_FONT_TEST == 1)) || \
     (defined(EDP_PHASE8_WEATHER_ICON_TEST) && (EDP_PHASE8_WEATHER_ICON_TEST == 1)))
#error "8C-2 six-day display test requires ALL other preview/app modes off."
#endif
#if (defined(EDP_PHASE8C2_DISPLAY_TEST) && (EDP_PHASE8C2_DISPLAY_TEST == 1)) && \
    (!defined(WEATHER_LATITUDE) || !defined(WEATHER_LONGITUDE))
#error "8C-2 requires WEATHER_LATITUDE and WEATHER_LONGITUDE in local config.h."
#endif

#if (defined(EDP_PHASE8C1_DAILY_TEST) && (EDP_PHASE8C1_DAILY_TEST == 1))
static bool phase8C1Ready = false;
static bool phase8C1Fetched = false;
static bool phase8C1Attempted = false;
static uint32_t phase8C1LastAttemptMs = 0;
#endif

#if (defined(EDP_PHASE8C2_DISPLAY_TEST) && (EDP_PHASE8C2_DISPLAY_TEST == 1))
static bool phase8C2Ready = false;
static bool phase8C2Displayed = false;
static bool phase8C2Attempted = false;
static uint32_t phase8C2LastAttemptMs = 0;
#endif

#if (defined(EDP_PHASE8C3_APP_TEST) && (EDP_PHASE8C3_APP_TEST == 1))
#if !(defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1))
#error "8C-3 requires EDP_PHASE8B5B_APP_TEST=1: reuse normal Phase 7 app."
#endif
#if (defined(EDP_PHASE8C1_DAILY_TEST) && (EDP_PHASE8C1_DAILY_TEST == 1)) || \
    (defined(EDP_PHASE8C2_DISPLAY_TEST) && (EDP_PHASE8C2_DISPLAY_TEST == 1))
#error "Disable isolated 8C-1/8C-2 modes before enabling 8C-3."
#endif
#endif

CrowEPD579 display;

#if defined(EDP_PHASE8_FONT_TEST) && (EDP_PHASE8_FONT_TEST == 1)
static uint8_t diagnosticFontPage = 1;
#endif

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
SystemPages systemPages(graphics);
DashboardActionMenu dashboardActionMenu;
DashboardUpdateCoalescer updateCoalescer;
ButtonManager buttonManager;

NavigationController navigationController;
PageId visiblePage = PageId::DASHBOARD;
bool visiblePageRefreshPending = false;

enum class PendingSystemAction : uint8_t {
  NONE,
  STANDBY,
  DISPLAY_CLEAN
};
PendingSystemAction pendingSystemAction = PendingSystemAction::NONE;

WeatherSnapshot lastRenderedWeatherSnapshot{};
bool hasRenderedWeatherSnapshot = false;

MarketPageState lastRenderedMarketPageState{};
bool hasRenderedMarketPageState = false;

WiFiManager wifiManager;
TimeService timeService;

#if (defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1))
static Phase8DashboardVisualState phase8LiveVisual{};
static WiFiConnectionState phase8LastWifiConnection =
    WiFiConnectionState::DISCONNECTED;
static const Bitmap1bpp* phase8LastSampledWifi = &Icons::WIFI_DISCONNECTED;
static bool phase8WifiSampled = false;
static uint32_t phase8WifiLastSampleMs = 0;
static constexpr uint32_t PHASE8_WIFI_SAMPLE_MS = 60000UL;
#endif

#if defined(EDP_PHASE8B2_CLOCK_TEST) && (EDP_PHASE8B2_CLOCK_TEST == 1)
static Phase8CalendarHeader phase8B2Displayed = {
    "--:--", "-- ---", "WAITING"
};
static bool phase8B2Ready = false;
#endif
SecureHttpClient secureHttpClient(
    TlsTrustAnchors::DIGICERT_GLOBAL_ROOT_G2
);
MarketDataService marketDataService(secureHttpClient);

#if (defined(EDP_PHASE8B3_MARKET_TEST) && (EDP_PHASE8B3_MARKET_TEST == 1)) || (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1)) || (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1))
static Phase8CalendarHeader phase8B3DisplayedCalendar = {
    "--:--", "-- ---", "WAITING"
};
static Phase8MarketPrices phase8B3DisplayedMarkets = {"--", "--", "--"};
static bool phase8B3Ready = false;
static bool phase8B3HasPolled = false;
static uint32_t phase8B3LastPollMs = 0;
#endif

#if (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1)) || (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1))
static Phase8CurrentWeather phase8B4DisplayedWeather{};
static bool phase8B4HasPolled = false;
static uint32_t phase8B4LastPollMs = 0;
#endif

#if (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1))
static const Bitmap1bpp* phase8B5DisplayedWifi = &Icons::WIFI_DISCONNECTED;
static const Bitmap1bpp* phase8B5SampledWifi = &Icons::WIFI_DISCONNECTED;
static bool phase8B5HasSampled = false;
static uint32_t phase8B5LastSampleMs = 0;
static WiFiConnectionState phase8B5LastConnection = WiFiConnectionState::DISCONNECTED;
static constexpr uint32_t PHASE8B5_WIFI_SAMPLE_MS = 60000UL;
#endif

SecureHttpClient weatherSecureHttpClient(
    TlsTrustAnchors::ISRG_ROOT_X1
);
WeatherService weatherService(weatherSecureHttpClient);

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

static bool renderApplicationDashboard(const DashboardState& legacyState) {
#if (defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1))
  // The verified native typography/layout, now owned by the normal app.
  return dashboard.renderLive(phase8LiveVisual.latest());
#else
  return dashboard.render(legacyState);
#endif
}

static bool applicationDashboardHasPending() {
#if (defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1))
  // Compare actual approved Dashboard pixels: legacy status/raw precision
  // changes must not force E-paper refresh when the live visual is identical.
  return phase8LiveVisual.hasPending();
#else
  return updateCoalescer.hasPendingUpdate();
#endif
}

static bool commitApplicationDashboard() {
  // Both snapshots advance only after the actual panel has been updated.
  if (updateCoalescer.hasPendingUpdate() &&
      !updateCoalescer.commitPending()) return false;
#if (defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1))
  if (!phase8LiveVisual.commitPending()) return false;
#endif
  return true;
}

static UpdateResult flushPendingPartial() {
  Serial.print("Partial flush pending dirty: ");
  printDirtyMask(updateCoalescer.pendingDirty());

  if (!applicationDashboardHasPending()) {
    Serial.println("SKIP: no coalesced dashboard changes to refresh.");
    delay(4000);
    return UpdateResult::SKIPPED;
  }

  const DashboardState& state =
      updateCoalescer.hasPendingUpdate()
          ? updateCoalescer.pendingState()
          : updateCoalescer.displayedState();

  if (!display.begin()) {
    Serial.println("FAIL: partial wake/reset timed out.");
    return UpdateResult::FAILED;
  }

  if (!display.restoreFrameStateForPartial(previousFrameBuffer)) {
    Serial.println("FAIL: partial state restore timed out.");
    return UpdateResult::FAILED;
  }

  if (!renderApplicationDashboard(state)) {
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

  const bool committed = commitApplicationDashboard();
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

  if (!applicationDashboardHasPending()) {
    Serial.println("SKIP: no coalesced dashboard changes; maintenance not required.");
    delay(4000);
    return UpdateResult::SKIPPED;
  }

  const DashboardState& state =
      updateCoalescer.hasPendingUpdate()
          ? updateCoalescer.pendingState()
          : updateCoalescer.displayedState();

  if (!renderApplicationDashboard(state)) {
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

  const bool committed = commitApplicationDashboard();
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
  if (!renderApplicationDashboard(state)) {
    Serial.println("FAIL: baseline frame rendering failed.");
    return false;
  }

#if (defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1))
  // Accepted Phase 8B test baselines call maintenanceRefresh directly:
  // it manages its own controller start and the verified clear/white/partial.
  if (!display.maintenanceRefresh(frameBuffer)) {
    display.sleep();
    Serial.println("FAIL: modern baseline maintenance refresh timed out.");
    return false;
  }
#else
  if (!display.begin()) {
    Serial.println("FAIL: baseline display begin/reset timed out.");
    return false;
  }
  if (!display.displayFullFrame(frameBuffer)) {
    Serial.println("FAIL: baseline full refresh timed out.");
    return false;
  }
#endif

  memcpy(
      previousFrameBuffer,
      frameBuffer,
      CrowEPD579::FRAMEBUFFER_BYTES
  );

  const bool established = updateCoalescer.establishDisplayed(state);
#if (defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1))
  if (established) {
    // Rendered boot placeholder is now the physically displayed baseline.
    phase8LiveVisual.establishDisplayed(phase8LiveVisual.displayed());
  }
#endif
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

static bool copyMarketPagePrice(
    char* destination,
    size_t capacity,
    const char* symbol) {
  if (destination == nullptr || capacity == 0 ||
      symbol == nullptr) {
    return false;
  }

  const MarketPriceValue* value =
      marketDataService.lastValidValue(symbol);
  const char* source =
      value != nullptr ? value->price : "--";

  if (strlen(source) >= capacity) {
    return false;
  }

  strcpy(destination, source);
  return true;
}

static bool buildLiveMarketPageState(
    MarketPageState& state) {
  initializeMarketPageState(state);

  return copyMarketPagePrice(
             state.btcPrice,
             sizeof(state.btcPrice),
             "BTCUSDT") &&
         copyMarketPagePrice(
             state.ethPrice,
             sizeof(state.ethPrice),
             "ETHUSDT") &&
         copyMarketPagePrice(
             state.hypePrice,
             sizeof(state.hypePrice),
             "HYPEUSDT");
}

static void queueVisibleMarketPageRefreshIfNeeded() {
  if (visiblePage != PageId::MARKETS) {
    return;
  }

  MarketPageState latest{};

  if (!buildLiveMarketPageState(latest)) {
    Serial.println(
        "FAIL: could not build visible Market page state."
    );
    return;
  }

  if (hasRenderedMarketPageState &&
      sameMarketPageContent(
          lastRenderedMarketPageState,
          latest)) {
    Serial.println(
        "Market page prices unchanged; "
        "no physical page refresh queued."
    );
    return;
  }

  visiblePageRefreshPending = true;

  Serial.println(
      "Market page prices changed; "
      "application queued one visible-page refresh."
  );
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
  queueVisibleMarketPageRefreshIfNeeded();

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
  // Phase 8C-3 reuses this *same* production 30-minute poll;
  // seven daily dates include the already used current/hours/tomorrow.
  const bool fetched =
#if (defined(EDP_PHASE8C3_APP_TEST) && (EDP_PHASE8C3_APP_TEST == 1))
      weatherService.fetchLatestSixDay(latitude, longitude);
#else
      weatherService.fetchLatest(latitude, longitude);
#endif
  if (!fetched) {
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

#if (defined(EDP_PHASE8C3_APP_TEST) && (EDP_PHASE8C3_APP_TEST == 1))
  Phase8SixDayForecast sixDay{};
  if (!Phase8SixDayPresentation::fromSnapshot(*snapshot, sixDay)) {
    // The WeatherService JSON can be valid but too wide for the 14px grid:
    // preserve the LAST DISPLAYED six-day text rather than clipping.
    Serial.println(
        "8C-3: six-day typography unavailable/invalid; "
        "keep previously visible forecast.");
  } else {
    Serial.println(
        "8C-3: six future daily records validated for Dashboard staging.");
  }
#endif

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

static void queueVisibleWeatherPageRefreshIfNeeded() {
  if (visiblePage != PageId::WEATHER) {
    return;
  }

  const WeatherSnapshot* latest =
      weatherService.lastValidSnapshot();

  if (latest == nullptr) {
    return;
  }

  if (hasRenderedWeatherSnapshot &&
      sameWeatherPageContent(
          lastRenderedWeatherSnapshot,
          *latest)) {
    Serial.println(
        "Weather page snapshot unchanged; "
        "no physical page refresh queued."
    );
    return;
  }

  visiblePageRefreshPending = true;

  Serial.println(
      "Weather page snapshot changed; "
      "application queued one visible-page refresh."
  );
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

  if (result != WeatherStageResult::FAILED) {
    queueVisibleWeatherPageRefreshIfNeeded();
  }

  Serial.println(
      "WeatherService did not trigger E-paper refresh directly."
  );
  Serial.println();
#endif
}

#if (defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1))
static bool stagePhase8LiveDashboardFromServices() {
  // Stage a complete visual candidate, not raw API state. All service polls
  // and all refresh decisions stay owned by the existing application loop.
  Phase8DashboardFrame next = phase8LiveVisual.latest();

  if (timeService.isSynchronized()) {
    struct tm local{};
    if (!timeService.getLocalTime(local) ||
        !Phase8CalendarPresentation::fromLocalTm(local, next.calendar)) {
      Serial.println("FAIL: Phase 8 live calendar conversion.");
      return false;
    }
  }

  struct PriceTarget {
    const char* symbol;
    char* text;
  };
  PriceTarget prices[] = {
      {"ETHUSDT", next.markets.eth},
      {"BTCUSDT", next.markets.btc},
      {"HYPEUSDT", next.markets.hype}
  };
  for (auto& entry : prices) {
    const MarketPriceValue* value =
        marketDataService.lastValidValue(entry.symbol);
    if (value == nullptr) continue; // retain previous/-- placeholder
    char candidate[Phase8MarketPrices::DISPLAY_CAPACITY]{};
    if (Phase8MarketPresentation::formatPrice(
            entry.symbol, value->price, candidate, sizeof(candidate))) {
      strcpy(entry.text, candidate);
    } // Never replace last valid with a clipped/invalid price.
  }

  const WeatherSnapshot* weatherSnapshot =
      weatherService.lastValidSnapshot();
  if (weatherSnapshot != nullptr) {
    Phase8CurrentWeather candidate{};
    if (Phase8WeatherPresentation::fromCurrent(
            weatherSnapshot->current, candidate)) {
      next.weather = candidate;
    } // Preserve the previous visible weather if conversion fails.
#if (defined(EDP_PHASE8C3_APP_TEST) && (EDP_PHASE8C3_APP_TEST == 1))
    if (weatherSnapshot->hasSixDayForecast) {
      Phase8SixDayForecast daily{};
      if (Phase8SixDayPresentation::fromSnapshot(*weatherSnapshot, daily)) {
        if (!Phase8SixDayPresentation::equal(next.forecast, daily)) {
          Serial.println("8C-3: six-day forecast visual change staged.");
        }
        next.forecast = daily;
      } // A failed new provider/formatter value never erases last-valid.
    }
#endif
  }

  const uint32_t now = millis();
  const WiFiConnectionState link = wifiManager.state();
  if (!phase8WifiSampled || link != phase8LastWifiConnection ||
      static_cast<uint32_t>(now - phase8WifiLastSampleMs) >=
          PHASE8_WIFI_SAMPLE_MS) {
    phase8WifiSampled = true;
    phase8LastWifiConnection = link;
    phase8WifiLastSampleMs = now;
    phase8LastSampledWifi = wifiManager.isConnected()
        ? wifiIconForRssi(wifiManager.rssi())
        : &Icons::WIFI_DISCONNECTED;
  }
  next.wifi = phase8LastSampledWifi;

  if (!phase8LiveVisual.stage(next)) {
    Serial.println("FAIL: Phase 8 live visual state staging.");
    return false;
  }
  return true;
}
#endif

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
  const NavigationState nav = navigationController.state();
  // While the DETAIL menu is visible, data may accumulate but must not
  // paint the production dashboard over the menu.
  if (nav.page != PageId::DASHBOARD ||
      nav.mode != NavigationMode::PAGE ||
      visiblePageRefreshPending) {
    return true;
  }

  if (!applicationDashboardHasPending()) {
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

  MarketPageState marketState{};
  if (!buildLiveMarketPageState(marketState)) {
    Serial.println("FAIL: could not build Market page state.");
    return false;
  }

  const bool dashboardMenuVisible =
      targetPage == PageId::DASHBOARD &&
      navigationState.mode == NavigationMode::DETAIL;

  Serial.print("Phase 7D-1 rendering: ");
  Serial.println(dashboardMenuVisible
      ? "DASHBOARD DETAIL"
      : PageModel::pageName(targetPage));

#if (defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1))
  // PageRenderer may paint Dashboard only; Weather/Markets remain Phase 7.
  pageRenderer.useLiveDashboardFrame(&phase8LiveVisual.latest());
#endif
  const bool rendered = dashboardMenuVisible
      ? systemPages.renderMenu(dashboardActionMenu.selected())
      : pageRenderer.render(
            targetPage, dashboardState, weatherSnapshot, marketState);

  if (!rendered) {
    Serial.println("FAIL: selected page rendering failed.");
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
      navigationState.mode == NavigationMode::PAGE &&
      (applicationDashboardHasPending() ||
       updateCoalescer.hasPendingUpdate())) {
    // A page transition physically draws the latest Dashboard. Commit
    // legacy raw status updates as well as visible modern data after success.
    if (!commitApplicationDashboard()) {
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

  if (targetPage == PageId::MARKETS) {
    lastRenderedMarketPageState = marketState;
    hasRenderedMarketPageState = true;

    Serial.println(
        "Market page rendered from current per-symbol last-valid prices."
    );
  }

  Serial.print("Phase 7D-1 visible content now: ");
  Serial.println(dashboardMenuVisible
      ? "DASHBOARD DETAIL"
      : PageModel::pageName(visiblePage));
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

static bool enterStandbyDeepSleep() {
  // Input events arrive only after a stable release. Recheck both wake
  // pins before arming level-sensitive ANY_LOW wakeup.
  if (digitalRead(ButtonManager::PIN_MENU) != HIGH ||
      digitalRead(ButtonManager::PIN_EXIT) != HIGH) {
    Serial.println("STANDBY cancelled: release MENU and EXIT first.");
    return false;
  }

  const uint64_t wakeMask =
      (1ULL << ButtonManager::PIN_MENU) |
      (1ULL << ButtonManager::PIN_EXIT);

  const esp_err_t wakeConfigResult =
      esp_sleep_enable_ext1_wakeup(
          wakeMask, ESP_EXT1_WAKEUP_ANY_LOW);

  if (wakeConfigResult != ESP_OK) {
    Serial.printf(
        "STANDBY cancelled: EXT1 wake configuration failed (%d).\n",
        static_cast<int>(wakeConfigResult));
    return false;
  }

  // SystemPages owns pixels only; the application performs physical I/O.
  systemPages.renderStandby();

  if (!display.begin()) {
    Serial.println("STANDBY cancelled: display begin failed.");
    return false;
  }
  if (!display.restoreFrameStateForPartial(previousFrameBuffer) ||
      !display.displayPartialFrame(frameBuffer)) {
    display.sleep();
    Serial.println(
        "STANDBY cancelled: moon frame could not be refreshed.");
    return false;
  }

  memcpy(previousFrameBuffer,
         frameBuffer,
         CrowEPD579::FRAMEBUFFER_BYTES);
  display.sleep();

  // The verified board driver powers the E-paper rail via GPIO7 HIGH.
  // Disable it only after the controller has finished the final frame.
  digitalWrite(CrowEPD579::PIN_PANEL_POWER, LOW);
  const gpio_num_t powerPin =
      static_cast<gpio_num_t>(CrowEPD579::PIN_PANEL_POWER);
  const esp_err_t holdResult = gpio_hold_en(powerPin);
  if (holdResult != ESP_OK) {
    // Do not sleep without guaranteeing panel-power pin state.
    digitalWrite(CrowEPD579::PIN_PANEL_POWER, HIGH);
    visiblePageRefreshPending = true;
    Serial.println("STANDBY cancelled: E-paper power GPIO hold failed.");
    return false;
  }
  gpio_deep_sleep_hold_en();

  // No data polling or application loop executes during deep sleep.
  WiFi.mode(WIFI_OFF);
  Serial.println(
      "STANDBY: moon displayed, panel power off, Wi-Fi off.");
  Serial.println(
      "STANDBY: MENU (GPIO1) or EXIT (GPIO2) wakes by EXT1 ANY_LOW.");
  Serial.flush();
  esp_deep_sleep_start();
  return false;  // esp_deep_sleep_start() does not normally return.
}

static bool runDisplayClean() {
  const DashboardState& latestDashboard =
      updateCoalescer.hasPendingUpdate()
          ? updateCoalescer.pendingState()
          : updateCoalescer.displayedState();

  // Never clean the menu framebuffer: rebuild the actual Dashboard.
  if (!renderApplicationDashboard(latestDashboard)) {
    Serial.println("DISPLAY CLEAN failed: Dashboard render.");
    return false;
  }

  Serial.println(
      "DISPLAY CLEAN: starting verified maintenanceRefresh().");
  if (!display.maintenanceRefresh(frameBuffer)) {
    display.sleep();
    Serial.println("DISPLAY CLEAN failed: maintenanceRefresh().");
    return false;
  }

  memcpy(previousFrameBuffer,
         frameBuffer,
         CrowEPD579::FRAMEBUFFER_BYTES);

  const bool committed = commitApplicationDashboard();

  display.sleep();

  // The physical screen now shows Dashboard, not the old DETAIL menu.
  navigationController.handle(InputEvent::EXIT);
  visiblePage = PageId::DASHBOARD;
  visiblePageRefreshPending = false;

  if (!committed) {
    Serial.println(
        "DISPLAY CLEAN: physical refresh succeeded, but state commit failed.");
    return false;
  }

  Serial.println(
      "DISPLAY CLEAN PASS: maintenance path complete; PAGE DASHBOARD.");
  return true;
}

static bool processPendingSystemAction() {
  const PendingSystemAction action = pendingSystemAction;
  pendingSystemAction = PendingSystemAction::NONE;

  switch (action) {
    case PendingSystemAction::NONE:
      return true;
    case PendingSystemAction::STANDBY:
      return enterStandbyDeepSleep();
    case PendingSystemAction::DISPLAY_CLEAN:
      return runDisplayClean();
  }

  return false;
}

static bool runDashboardMenuSelfCheck() {
  DashboardActionMenu test;
  if (test.selected() != DashboardAction::STANDBY ||
      !test.handle(InputEvent::DOWN) ||
      test.selected() != DashboardAction::DISPLAY_CLEAN ||
      !test.handle(InputEvent::UP) ||
      test.selected() != DashboardAction::STANDBY ||
      !test.handle(InputEvent::UP) ||
      test.selected() != DashboardAction::DISPLAY_CLEAN ||
      test.handle(InputEvent::MENU)) {
    Serial.println("FAIL: Phase 7D DETAIL menu self-check.");
    return false;
  }
  test.reset();
  if (test.selected() != DashboardAction::STANDBY) {
    Serial.println("FAIL: DETAIL menu reset.");
    return false;
  }
  Serial.println(
      "PASS: Phase 7D DETAIL menu selection self-check.");
  return true;
}

static void handlePanelInputIfAny() {
  const InputEvent event = buttonManager.tick();
  if (event == InputEvent::NONE) {
    return;
  }

  Serial.print("Phase 7D input event: ");
  Serial.println(inputEventName(event));
  const NavigationState before = navigationController.state();

  // Dashboard owns its two DETAIL options; other pages retain the
  // Phase 7C mode-only behavior without creating their own menus.
  if (before.page == PageId::DASHBOARD &&
      before.mode == NavigationMode::DETAIL) {
    if (event == InputEvent::UP || event == InputEvent::DOWN) {
      if (dashboardActionMenu.handle(event)) {
        visiblePageRefreshPending = true;
        Serial.println("DETAIL selection changed; redraw queued.");
      }
      return;
    }

    if (event == InputEvent::MENU) {
      pendingSystemAction =
          dashboardActionMenu.selected() == DashboardAction::STANDBY
              ? PendingSystemAction::STANDBY
              : PendingSystemAction::DISPLAY_CLEAN;
      Serial.println(
          pendingSystemAction == PendingSystemAction::STANDBY
              ? "DETAIL selected: STANDBY"
              : "DETAIL selected: DISPLAY CLEAN");
      return;
    }

    if (event == InputEvent::EXIT) {
      navigationController.handle(InputEvent::EXIT);
      visiblePageRefreshPending = true;
      Serial.println("DETAIL exit: Dashboard redraw queued.");
      return;
    }
  }

  const bool changed = navigationController.handle(event);
  const NavigationState after = navigationController.state();
  printNavigationState("Navigation before: ", before);
  printNavigationState(
      changed ? "Navigation after:  " : "Navigation unchanged: ",
      after);

  if (changed && before.page != after.page) {
    visiblePageRefreshPending = true;
    Serial.print("Visible page refresh queued: ");
    Serial.println(PageModel::pageName(after.page));
    return;
  }

  if (changed && before.page == PageId::DASHBOARD &&
      before.mode == NavigationMode::PAGE &&
      after.mode == NavigationMode::DETAIL) {
    dashboardActionMenu.reset();
    visiblePageRefreshPending = true;
    Serial.println("Dashboard DETAIL menu refresh queued.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Always release a possible retained GPIO7 hold before driver init.
  // This covers EXT1 wake as well as an EN/reset-button restart while asleep.
  pinMode(CrowEPD579::PIN_PANEL_POWER, OUTPUT);
  digitalWrite(CrowEPD579::PIN_PANEL_POWER, LOW);
  gpio_deep_sleep_hold_dis();
  gpio_hold_dis(
      static_cast<gpio_num_t>(CrowEPD579::PIN_PANEL_POWER));

  // Phase 8A-2: isolated, single-frame 11-icon weather gallery.
  // Pure bitmap and semantic mapping self-test; never starts Wi-Fi/APIs.
#if defined(EDP_PHASE8_WEATHER_ICON_TEST) && (EDP_PHASE8_WEATHER_ICON_TEST == 1)
  Serial.println("EDP Phase 8A-2: WEATHER ICON GALLERY (11 x 32x32)");
  if (!WeatherIconSelector::selfCheck()) {
    Serial.println("FAIL: weather icon condition mapping self-check.");
    return;
  }
  Serial.println("PASS: weather icon mapping self-check.");
  if (!WeatherIconGallery::render(graphics)) {
    Serial.println("FAIL: weather icon gallery composition.");
    return;
  }
  if (!display.maintenanceRefresh(frameBuffer)) {
    Serial.println("FAIL: weather icon gallery E-paper refresh.");
    return;
  }
  memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
  display.sleep();
  Serial.println("PASS: Phase 8A-2 gallery rendered once; Wi-Fi/APIs disabled.");
  return;
#endif

  // Rev-C3 FONT-ONLY preview. Explicit opt-in, no network or production services.
  // Display each sheet using the already verified maintenanceRefresh sequence.
#if defined(EDP_PHASE8_FONT_TEST) && (EDP_PHASE8_FONT_TEST == 1)
  Serial.println("EDP Phase 8A-1 Rev-C3-JB2 FONT TEST: native 1-bit 34/17/13/14px.");
  Serial.println("Send 1, 2, 3, 4, or 5 via Serial Monitor to choose a font sheet.");
  if (!DashboardFontTest::render(graphics, diagnosticFontPage)) {
    Serial.println("FAIL: font diagnostic composition failed.");
    return;
  }
  if (!display.maintenanceRefresh(frameBuffer)) {
    Serial.println("FAIL: font diagnostic E-paper refresh failed.");
    return;
  }
  memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
  display.sleep();
  Serial.println("PASS: Rev-C3-JB2 font sheet 1/5 rendered; Wi-Fi/APIs disabled.");
  return;
#endif

  // Isolated Phase 8A-1 ONE-SHOT preview; never contacts Wi-Fi or data APIs.
  // Enabled only when local ignored config.h explicitly defines:
  // #define EDP_PHASE8A1_PREVIEW 1
#if defined(EDP_PHASE8A1_PREVIEW) && (EDP_PHASE8A1_PREVIEW == 1)
  Serial.println("EDP Phase 8A-1: STATIC MOCK DATA PREVIEW");
  if (!Phase8A1Preview::render(graphics)) {
    Serial.println("FAIL: static prototype composition failed.");
    return;
  }
  // Reuse the Phase 1F verified fast-clear -> white -> partial maintenance.
  if (!display.maintenanceRefresh(frameBuffer)) {
    Serial.println("FAIL: static prototype maintenance refresh failed.");
    return;
  }
  memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
  display.sleep();
  Serial.println("PASS: static dashboard preview shown once; Wi-Fi/APIs disabled.");
  return;
#endif

  // Phase 8B-2 NTP-only test of the accepted Phase 8A renderer.
  // The header is live after sync; every other Dashboard field is a mock.
#if defined(EDP_PHASE8B2_CLOCK_TEST) && (EDP_PHASE8B2_CLOCK_TEST == 1)
  Serial.println("EDP Phase 8B-2: NTP TIME / DATE / WEEKDAY TEST");
  Serial.println("Only calendar header is live. Crypto/weather/forecast/Wi-Fi icons are static MOCK values.");
  if (!Phase8CalendarPresentation::selfCheck()) {
    Serial.println("FAIL: 8B-2 calendar formatter / midnight self-check.");
    return;
  }
  Serial.println("PASS: calendar formatter, midnight and weekday self-check.");

  if (!Phase8A1Preview::renderWithCalendar(graphics, phase8B2Displayed) ||
      !display.maintenanceRefresh(frameBuffer)) {
    display.sleep();
    Serial.println("FAIL: 8B-2 WAITING baseline composition/refresh.");
    return;
  }
  memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
  display.sleep();
  Serial.println("PASS: 8B-2 WAITING baseline displayed via maintenanceRefresh().");

  if (!wifiManager.begin(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("FAIL: Wi-Fi manager could not start; verify local config.h.");
    return;
  }
  phase8B2Ready = true;
  Serial.println("Waiting for NTP; only TimeService and Wi-Fi reconnect are active.");
  return;
#endif

  // Phase 8B-3: isolated NTP + three public, TLS-verified futures prices.
  // The header weather and six-day forecast are unchanged STATIC fixtures.
#if defined(EDP_PHASE8B3_MARKET_TEST) && (EDP_PHASE8B3_MARKET_TEST == 1)
  Serial.println("EDP Phase 8B-3: NTP + BINANCE USDT PERPETUAL PRICES");
  Serial.println("Weather/forecast/Wi-Fi ICON are MOCK; no WeatherService poll.");
  if (!Phase8CalendarPresentation::selfCheck() ||
      !Phase8MarketPresentation::selfCheck()) {
    Serial.println("FAIL: 8B-3 calendar or price formatter self-check.");
    return;
  }
  Serial.println("PASS: 8B-3 calendar and price formatting self-checks.");

  if (!Phase8A1Preview::renderWithCalendarAndMarkets(
          graphics, phase8B3DisplayedCalendar, phase8B3DisplayedMarkets) ||
      !display.maintenanceRefresh(frameBuffer)) {
    display.sleep();
    Serial.println("FAIL: 8B-3 startup placeholder maintenance refresh.");
    return;
  }
  memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
  display.sleep();
  Serial.println("PASS: 8B-3 WAITING / -- placeholders displayed.");

  if (!wifiManager.begin(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("FAIL: 8B-3 Wi-Fi start. Check local ignored config.h.");
    return;
  }
  phase8B3Ready = true;
  Serial.println("Awaiting NTP; market polling starts after valid local time.");
  return;
#endif

  // Phase 8B-4: same approved NTP + Binance rendering path plus
  // current Open-Meteo condition/temperature/day-night; daily strip stays MOCK.
#if (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1))
  Serial.println("EDP Phase 8B-4: LIVE NTP + CRYPTO + CURRENT WEATHER");
  Serial.println("Forecast strip and Wi-Fi icon are STATIC MOCKS.");
  if (!Phase8CalendarPresentation::selfCheck() ||
      !Phase8MarketPresentation::selfCheck() ||
      !WeatherIconSelector::selfCheck() ||
      !Phase8WeatherPresentation::selfCheck()) {
    Serial.println("FAIL: 8B-4 calendar/market/weather self-checks.");
    return;
  }
  Serial.println("PASS: 8B-4 all presentation/icon self-checks.");

  Phase8WeatherPresentation::setUnavailable(phase8B4DisplayedWeather);
  if (!Phase8A1Preview::renderWithCalendarMarketsWeather(
          graphics, phase8B3DisplayedCalendar, phase8B3DisplayedMarkets,
          phase8B4DisplayedWeather) ||
      !display.maintenanceRefresh(frameBuffer)) {
    display.sleep();
    Serial.println("FAIL: 8B-4 startup placeholder maintenance refresh.");
    return;
  }
  memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
  display.sleep();
  Serial.println("PASS: 8B-4 WAITING / -- / NO DATA shown.");

  if (!wifiManager.begin(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("FAIL: 8B-4 Wi-Fi start; check ignored local config.h.");
    return;
  }
  phase8B3Ready = true;
  Serial.println("Waiting for NTP before TLS-verified market/weather polling.");
  return;
#endif

  // Phase 8B-5A: accepted 8B-4 NTP/market/weather fields with live
  // Wi-Fi RSSI/connection bitmap only; production navigation remains isolated.
#if (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1))
  Serial.println("EDP Phase 8B-5A: LIVE NTP + CRYPTO + WEATHER + WIFI");
  Serial.println("Bottom six-day forecast remains STATIC MOCK; normal pages unchanged.");
  if (!Phase8CalendarPresentation::selfCheck() ||
      !Phase8MarketPresentation::selfCheck() ||
      !WeatherIconSelector::selfCheck() ||
      !Phase8WeatherPresentation::selfCheck()) {
    Serial.println("FAIL: 8B-5A inherited presentation self-check.");
    return;
  }

  // Reuse the exact Phase 4 RSSI grade mapping, including boundary values.
  const struct {
    int32_t rssi;
    const Bitmap1bpp* expected;
  } wifiCases[] = {
      {-50, &Icons::WIFI_STRONG},
      {-60, &Icons::WIFI_STRONG},
      {-61, &Icons::WIFI_MEDIUM},
      {-70, &Icons::WIFI_MEDIUM},
      {-71, &Icons::WIFI_WEAK},
      {-80, &Icons::WIFI_WEAK},
      {-81, &Icons::WIFI_VERY_WEAK}
  };
  for (const auto& sample : wifiCases) {
    const Bitmap1bpp* icon = wifiIconForRssi(sample.rssi);
    if (icon != sample.expected || icon->width != 32 ||
        icon->height != 24 || icon->data == nullptr) {
      Serial.println("FAIL: 8B-5A RSSI grade self-check.");
      return;
    }
  }
  if (Icons::WIFI_DISCONNECTED.width != 32 ||
      Icons::WIFI_DISCONNECTED.height != 24 ||
      Icons::WIFI_DISCONNECTED.data == nullptr) {
    Serial.println("FAIL: 8B-5A disconnected Wi-Fi bitmap.");
    return;
  }
  Serial.println("PASS: 8B-5A weather, market, NTP and Wi-Fi RSSI self-checks.");

  Phase8WeatherPresentation::setUnavailable(phase8B4DisplayedWeather);
  if (!Phase8A1Preview::renderWithCalendarMarketsWeatherWifi(
          graphics, phase8B3DisplayedCalendar, phase8B3DisplayedMarkets,
          phase8B4DisplayedWeather, *phase8B5DisplayedWifi) ||
      !display.maintenanceRefresh(frameBuffer)) {
    display.sleep();
    Serial.println("FAIL: 8B-5A startup frame or E-paper maintenance.");
    return;
  }
  memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
  display.sleep();
  Serial.println("PASS: 8B-5A WAITING / -- / NO DATA / Wi-Fi disconnected baseline.");

  if (!wifiManager.begin(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("FAIL: 8B-5A Wi-Fi begin; check ignored local config.h.");
    return;
  }
  phase8B3Ready = true;
  return;
#endif

  // Phase 8C-2 one-shot REAL six-day E-paper rendering.
  // No Binance, NTP or Phase 7 buttons in this isolated typography test.
#if (defined(EDP_PHASE8C2_DISPLAY_TEST) && (EDP_PHASE8C2_DISPLAY_TEST == 1))
  Serial.println("EDP Phase 8C-2: SIX REAL DAILY WEATHER COLUMNS");
  Serial.println("Upper calendar/market fields are WAITING/-- placeholders.");
  Serial.println("Lower 6 columns use actual Open-Meteo seven-day data.");
  if (!WeatherService::sixDayDateSelfCheck() ||
      !WeatherIconSelector::selfCheck() ||
      !Phase8SixDayPresentation::selfCheck()) {
    Serial.println("FAIL: 8C-2 weekday/summary/icon self-check.");
    return;
  }
  Serial.println("PASS: 8C-2 weekday, temperature, precipitation and icon self-check.");
  if (!wifiManager.begin(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("FAIL: 8C-2 Wi-Fi begin.");
    return;
  }
  phase8C2Ready = true;
  return;
#endif

  // Serial-only real-data test, no display refresh or panel animation.
#if (defined(EDP_PHASE8C1_DAILY_TEST) && (EDP_PHASE8C1_DAILY_TEST == 1))
  Serial.println("EDP Phase 8C-1: REAL OPEN-METEO SIX-FUTURE-DAY DATA");
  Serial.println("No E-paper refresh in this test; previous panel image stays.");
  if (!WeatherService::sixDayDateSelfCheck() ||
      !runWeatherConditionMappingSelfCheck()) {
    Serial.println("FAIL: 8C-1 date/calendar or condition mapping self-check.");
    return;
  }
  Serial.println("PASS: 8C-1 date/leap/year and WMO self-check.");
  if (!wifiManager.begin(WIFI_SSID, WIFI_PASSWORD)) {
    Serial.println("FAIL: 8C-1 Wi-Fi begin.");
    return;
  }
  phase8C1Ready = true;
  return;
#endif

  const esp_sleep_wakeup_cause_t wakeCause =
      esp_sleep_get_wakeup_cause();
  if (wakeCause == ESP_SLEEP_WAKEUP_EXT1) {
    Serial.println("Wake reason: EXT1 MENU/EXIT, restarting Dashboard.");
    const uint64_t pins = esp_sleep_get_ext1_wakeup_status();
    Serial.printf(
        "EXT1 wake pin mask: 0x%llX\n",
        static_cast<unsigned long long>(pins));

    // Avoid treating the wake press release as a new MENU/EXIT event.
    pinMode(ButtonManager::PIN_MENU, INPUT);
    pinMode(ButtonManager::PIN_EXIT, INPUT);
    const uint32_t releasedWaitStart = millis();
    while ((digitalRead(ButtonManager::PIN_MENU) == LOW ||
            digitalRead(ButtonManager::PIN_EXIT) == LOW) &&
           static_cast<uint32_t>(
               millis() - releasedWaitStart) < 5000U) {
      delay(10);
    }
    delay(50);
  }

  buttonManager.begin();

  Serial.println();
  Serial.println("EDP Phase 7D: Dashboard DETAIL system actions");
  Serial.println("DASHBOARD MENU opens STANDBY / DISPLAY CLEAN.");
  Serial.println("DETAIL UP/DOWN selects; MENU executes; EXIT returns.");
  Serial.println("STANDBY draws a small moon then enters ESP32-S3 deep sleep.");
  Serial.println("GPIO1 MENU or GPIO2 EXIT wakes the chip; no timer wake.");
  Serial.println("DISPLAY CLEAN uses verified maintenanceRefresh(), not raw Full Refresh.");
  Serial.println("WEATHER/MARKETS render and polling paths remain unchanged.");

  if (!runNavigationSelfCheck() ||
      !runDashboardMenuSelfCheck()) {
    Serial.println("FAIL: Phase 7D navigation/menu self-checks.");
    return;
  }

  Serial.println("PASS: Phase 7D navigation/menu self-checks.");

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

#if (defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1))
  Serial.println("EDP Phase 8B-5B: normal Phase 7 app with approved live Dashboard.");
  Serial.println("Six-day forecast: UNAVAILABLE, not mock daily predictions.");
  if (!Phase8CalendarPresentation::selfCheck() ||
      !Phase8MarketPresentation::selfCheck() ||
      !Phase8WeatherPresentation::selfCheck() ||
      !WeatherIconSelector::selfCheck() ||
      !Phase8DashboardFrameCompare::selfCheck()
#if (defined(EDP_PHASE8C3_APP_TEST) && (EDP_PHASE8C3_APP_TEST == 1))
      || !Phase8SixDayPresentation::selfCheck() ||
         !WeatherService::sixDayDateSelfCheck()
#endif
      ) {
    Serial.println("FAIL: Phase 8B-5B live presentation self-checks.");
    return;
  }
  Serial.println("PASS: Phase 8B-5B owned state and presentation self-checks.");
#if (defined(EDP_PHASE8C3_APP_TEST) && (EDP_PHASE8C3_APP_TEST == 1))
  Serial.println("EDP Phase 8C-3: COMPLETE LIVE DASHBOARD + SIX REAL DAYS.");
  Serial.println("8C-3: seven daily dates on existing 30-minute poll.");
  Serial.println("PASS: 8C-3 six-day/date/visible-state self-checks.");
#endif
#endif

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
#if (defined(EDP_PHASE8C2_DISPLAY_TEST) && (EDP_PHASE8C2_DISPLAY_TEST == 1))
  if (!phase8C2Ready || phase8C2Displayed) {
    delay(1000);
    return;
  }
  wifiManager.tick();
  if (!wifiManager.isConnected()) {
    delay(250);
    return;
  }
  const uint32_t attemptNow = millis();
  if (phase8C2Attempted &&
      static_cast<uint32_t>(attemptNow - phase8C2LastAttemptMs) < 30000UL) {
    delay(250);
    return;
  }
  phase8C2Attempted = true;
  phase8C2LastAttemptMs = attemptNow;
  Serial.println("8C-2: requesting 7 verified daily records from Open-Meteo...");
  if (!weatherService.fetchLatestSixDay(
          static_cast<double>(WEATHER_LATITUDE),
          static_cast<double>(WEATHER_LONGITUDE))) {
    Serial.print("FAIL: 8C-2 forecast fetch/validation: ");
    Serial.println(weatherService.lastError());
    delay(250);
    return;
  }

  const WeatherSnapshot* snapshot = weatherService.lastValidSnapshot();
  if (snapshot == nullptr || !snapshot->hasSixDayForecast) {
    Serial.println("FAIL: 8C-2 no validated futureDays[6].");
    delay(250);
    return;
  }
  Phase8SixDayForecast forecast{};
  Phase8SixDayPresentation::setUnavailable(forecast);
  if (!Phase8SixDayPresentation::fromSnapshot(*snapshot, forecast)) {
    Serial.println("FAIL: 8C-2 daily values cannot fit the native 14px columns.");
    delay(250);
    return;
  }
  Phase8DashboardFrame frame{};
  Phase8DashboardFrameCompare::setUnavailable(frame);
  if (!Phase8WeatherPresentation::fromCurrent(snapshot->current,
                                               frame.weather)) {
    Serial.println("FAIL: 8C-2 current weather preview mapping.");
    delay(250);
    return;
  }
  frame.wifi = wifiIconForRssi(wifiManager.rssi());
  if (!Phase8A1Preview::renderWithSixDayForecast(
          graphics, frame, forecast)) {
    Serial.println("FAIL: 8C-2 six-column typography/frame composition.");
    delay(250);
    return;
  }
  if (!display.maintenanceRefresh(frameBuffer)) {
    display.sleep();
    Serial.println("FAIL: 8C-2 verified maintenanceRefresh().");
    delay(1000);
    return;
  }
  memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
  display.sleep();
  phase8C2Displayed = true;
  Serial.println("PASS: 8C-2 SIX REAL FORECAST COLUMNS DRAWN ONCE.");
  for (size_t i = 0; i < Phase8SixDayForecast::COUNT; ++i) {
    Serial.print("  +");
    Serial.print(static_cast<unsigned>(i + 1));
    Serial.print("d ");
    Serial.print(snapshot->futureDays[i].date);
    Serial.print(" -> ");
    Serial.print(forecast.days[i].weekday);
    Serial.print(" ");
    Serial.println(forecast.days[i].summary);
  }
  delay(1000);
  return;
#endif
#if (defined(EDP_PHASE8C1_DAILY_TEST) && (EDP_PHASE8C1_DAILY_TEST == 1))
  if (!phase8C1Ready || phase8C1Fetched) {
    delay(1000);
    return;
  }
  wifiManager.tick();
  if (!wifiManager.isConnected()) {
    delay(250);
    return;
  }
  const uint32_t attemptNow = millis();
  if (phase8C1Attempted &&
      static_cast<uint32_t>(attemptNow - phase8C1LastAttemptMs) < 30000UL) {
    delay(250);
    return;
  }
  phase8C1Attempted = true;
  phase8C1LastAttemptMs = attemptNow;
  Serial.println("8C-1: HTTPS Open-Meteo forecast_days=7, timezone=Asia/Taipei.");
  if (!weatherService.fetchLatestSixDay(
          static_cast<double>(WEATHER_LATITUDE),
          static_cast<double>(WEATHER_LONGITUDE))) {
    Serial.print("FAIL: 8C-1 7-day fetch/validation: ");
    Serial.println(weatherService.lastError());
    Serial.println("Retry after 30 seconds; no fabricated daily values.");
    delay(250);
    return;
  }
  const WeatherSnapshot* value = weatherService.lastValidSnapshot();
  if (value == nullptr || !value->hasSixDayForecast) {
    Serial.println("FAIL: 8C-1 service returned no validated 6-day list.");
    delay(1000);
    return;
  }
  Serial.print("8C-1 provider local today: ");
  Serial.println(value->today.date);
  for (size_t i = 0; i < WeatherSnapshot::FUTURE_DAY_COUNT; ++i) {
    const WeatherDailyValue& day = value->futureDays[i];
    Serial.print("+");
    Serial.print(static_cast<unsigned>(i + 1));
    Serial.print("d ");
    Serial.print(day.date);
    Serial.print(" | ");
    Serial.print(weatherConditionName(day.condition));
    Serial.print(" (WMO ");
    Serial.print(day.weatherCode);
    Serial.print(") | max ");
    Serial.print(day.temperatureMaxC, 1);
    Serial.print(" C min ");
    Serial.print(day.temperatureMinC, 1);
    Serial.print(" C | precipitation max ");
    Serial.print(day.precipitationProbabilityMax);
    Serial.println("%");
  }
  Serial.println("PASS: 8C-1 SIX REAL FUTURE DAYS VALIDATED (serial only).");
  phase8C1Fetched = true; // one-shot test; no repeated request
  delay(1000);
  return;
#endif
#if (defined(EDP_PHASE8B3_MARKET_TEST) && (EDP_PHASE8B3_MARKET_TEST == 1)) || (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1)) || (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1))
  if (!phase8B3Ready) {
    delay(1000);
    return;
  }
  wifiManager.tick();
  if (!startTimeServiceWhenConnected()) {
    Serial.println("FAIL: 8B-3 NTP service start.");
    delay(1000);
    return;
  }
  timeService.tick();

  // Copy the last physically displayed state, then stage all potential
  // changes together. One successful refresh commits one coherent frame.
  Phase8CalendarHeader nextCalendar = phase8B3DisplayedCalendar;
  Phase8MarketPrices nextMarkets = phase8B3DisplayedMarkets;

  if (timeService.isSynchronized()) {
    struct tm local{};
    if (!timeService.getLocalTime(local) ||
        !Phase8CalendarPresentation::fromLocalTm(local, nextCalendar)) {
      Serial.println("FAIL: 8B-3 NTP local-time formatting.");
      delay(1000);
      return;
    }
  }

  const uint32_t now = millis();
  if (timeService.isSynchronized() && wifiManager.isConnected() &&
      (!phase8B3HasPolled ||
       static_cast<uint32_t>(now - phase8B3LastPollMs) >=
           MARKET_POLL_INTERVAL_MS)) {
    phase8B3HasPolled = true;
    phase8B3LastPollMs = now;
    Serial.println("8B-3: polling three existing USD-M perpetual symbols.");
    for (size_t i = 0; i < marketDataService.trackedSymbolCount(); ++i) {
      const char* symbol = marketDataService.trackedSymbol(i);
      if (symbol == nullptr) continue;

      char* dest = nullptr;
      if (strcmp(symbol, "ETHUSDT") == 0) dest = nextMarkets.eth;
      else if (strcmp(symbol, "BTCUSDT") == 0) dest = nextMarkets.btc;
      else if (strcmp(symbol, "HYPEUSDT") == 0) dest = nextMarkets.hype;
      else {
        Serial.println("FAIL: 8B-3 unexpected tracked symbol.");
        continue;
      }

      const bool fetched = marketDataService.fetchLatest(symbol);
      if (!fetched) {
        Serial.print("Market fetch failed, retaining last valid: ");
        Serial.print(symbol);
        Serial.print(" ");
        Serial.println(marketDataService.lastError(symbol));
      }

      const MarketPriceValue* value =
          marketDataService.lastValidValue(symbol);
      if (value == nullptr) {
        strcpy(dest, "--");
        continue;
      }

      char formatted[Phase8MarketPrices::DISPLAY_CAPACITY]{};
      if (!Phase8MarketPresentation::formatPrice(
              symbol, value->price, formatted, sizeof(formatted))) {
        // Never render a clipped/misleading value. Full source is retained.
        strcpy(dest, "--");
        Serial.print("Price not representable in 34px column: ");
        Serial.println(symbol);
        continue;
      }

      strcpy(dest, formatted);
      Serial.print(fetched ? "Market fetched " : "Market preserved ");
      Serial.print(symbol);
      Serial.print(" raw=");
      Serial.print(value->price);
      Serial.print(" shown=");
      Serial.println(dest);
    }
  }

#if (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1)) || (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1))
  Phase8CurrentWeather nextWeather = phase8B4DisplayedWeather;
  if (timeService.isSynchronized() && wifiManager.isConnected() &&
      (!phase8B4HasPolled ||
       static_cast<uint32_t>(millis() - phase8B4LastPollMs) >=
           WEATHER_POLL_INTERVAL_MS)) {
    phase8B4HasPolled = true;
    phase8B4LastPollMs = millis();
    Serial.println("8B-4: polling existing Open-Meteo current weather (30 min).");

    const bool fetched = weatherService.fetchLatest(
        static_cast<double>(WEATHER_LATITUDE),
        static_cast<double>(WEATHER_LONGITUDE));
    if (!fetched) {
      Serial.print("Weather fetch failed; retaining last valid: ");
      Serial.println(weatherService.lastError());
    }
    const WeatherSnapshot* snapshot = weatherService.lastValidSnapshot();
    if (snapshot != nullptr) {
      Phase8CurrentWeather mapped{};
      if (Phase8WeatherPresentation::fromCurrent(snapshot->current, mapped)) {
        nextWeather = mapped;
        Serial.print(fetched ? "Weather fetched: " : "Weather preserved: ");
        Serial.print(weatherConditionName(snapshot->current.condition));
        Serial.print(snapshot->current.isDay ? " DAY " : " NIGHT ");
        Serial.print(nextWeather.label);
        Serial.print(" ");
        Serial.println(nextWeather.temperature);
      } else {
        Serial.println("FAIL: last-valid weather not displayable; previous UI retained.");
      }
    } else {
      Serial.println("Weather last-valid unavailable; keep NO DATA placeholder.");
    }
  }

  // Compare selected artwork pointer too: unchanged temp/label must not
  // suppress a clear day-to-night bitmap change.
  const bool weatherChanged =
      !Phase8WeatherPresentation::equal(phase8B4DisplayedWeather, nextWeather);
#endif

#if (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1))
  // Sample RSSI once per minute, but observe Wi-Fi connection transitions
  // immediately. Keep sampled candidate separate from physically committed
  // artwork so failed refresh is retried without losing the changed icon.
  const uint32_t wifiNow = millis();
  const WiFiConnectionState currentConnection = wifiManager.state();
  if (!phase8B5HasSampled ||
      currentConnection != phase8B5LastConnection ||
      static_cast<uint32_t>(wifiNow - phase8B5LastSampleMs) >=
          PHASE8B5_WIFI_SAMPLE_MS) {
    phase8B5HasSampled = true;
    phase8B5LastSampleMs = wifiNow;
    phase8B5LastConnection = currentConnection;
    phase8B5SampledWifi = wifiManager.isConnected()
        ? wifiIconForRssi(wifiManager.rssi())
        : &Icons::WIFI_DISCONNECTED;

    Serial.print("8B-5A Wi-Fi icon sampled: ");
    if (wifiManager.isConnected()) {
      Serial.print(wifiManager.rssi());
      Serial.print(" dBm -> ");
      Serial.println(wifiSignalLevelName(wifiManager.rssi()));
    } else {
      Serial.println("DISCONNECTED / CONNECTING");
    }
  }
  const Bitmap1bpp* nextWifi = phase8B5SampledWifi;
  const bool wifiChanged = (phase8B5DisplayedWifi != nextWifi);
#endif

  const bool calendarChanged =
      !Phase8CalendarPresentation::equal(
          phase8B3DisplayedCalendar, nextCalendar);
  const bool marketChanged =
      !Phase8MarketPresentation::equal(
          phase8B3DisplayedMarkets, nextMarkets);
  if (calendarChanged || marketChanged
#if (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1)) || (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1))
      || weatherChanged
#endif
#if (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1))
      || wifiChanged
#endif
  ) {
    const bool composed =
#if (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1))
        Phase8A1Preview::renderWithCalendarMarketsWeatherWifi(
            graphics, nextCalendar, nextMarkets, nextWeather, *nextWifi);
#elif (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1))
        Phase8A1Preview::renderWithCalendarMarketsWeather(
            graphics, nextCalendar, nextMarkets, nextWeather);
#else
        Phase8A1Preview::renderWithCalendarAndMarkets(
            graphics, nextCalendar, nextMarkets);
#endif
    if (!composed) {
      Serial.println("FAIL: Phase 8B composed frame (no physical change).");
      delay(1000);
      return;
    }
    if (!display.begin()) {
      Serial.println("FAIL: 8B-3 display wake.");
      delay(1000);
      return;
    }
    if (!display.restoreFrameStateForPartial(previousFrameBuffer) ||
        !display.displayPartialFrame(frameBuffer)) {
      display.sleep();
      Serial.println("FAIL: 8B-3 partial refresh; displayed state not committed.");
      delay(1000);
      return;
    }

    memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
    display.sleep();
    phase8B3DisplayedCalendar = nextCalendar;
    phase8B3DisplayedMarkets = nextMarkets;
#if (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1))
    phase8B4DisplayedWeather = nextWeather;
    phase8B5DisplayedWifi = nextWifi;
    Serial.print("PASS: 8B-5A coalesced partial refresh (clock=");
#elif (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1))
    phase8B4DisplayedWeather = nextWeather;
    Serial.print("PASS: 8B-4 coalesced partial refresh (clock=");
#else
    Serial.print("PASS: 8B-3 coalesced partial refresh (clock=");
#endif
    Serial.print(calendarChanged ? "changed" : "same");
    Serial.print(", prices=");
    Serial.print(marketChanged ? "changed" : "same");
#if (defined(EDP_PHASE8B4_WEATHER_TEST) && (EDP_PHASE8B4_WEATHER_TEST == 1)) || (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1))
    Serial.print(", weather=");
    Serial.print(weatherChanged ? "changed" : "same");
#endif
#if (defined(EDP_PHASE8B5A_WIFI_TEST) && (EDP_PHASE8B5A_WIFI_TEST == 1))
    Serial.print(", wifi=");
    Serial.print(wifiChanged ? "changed" : "same");
#endif
    Serial.println(").");
  }

  delay(100);
  return;
#endif
#if defined(EDP_PHASE8B2_CLOCK_TEST) && (EDP_PHASE8B2_CLOCK_TEST == 1)
  if (!phase8B2Ready) {
    delay(1000);
    return;
  }

  // Network is used solely for existing TimeService NTP synchronization.
  // Do not fetch crypto/weather, run Phase 7 navigation, or refresh periodically.
  wifiManager.tick();
  if (!startTimeServiceWhenConnected()) {
    Serial.println("FAIL: 8B-2 NTP service start.");
    delay(1000);
    return;
  }
  timeService.tick();

  if (timeService.isSynchronized()) {
    struct tm localTime{};
    Phase8CalendarHeader next{};
    if (!timeService.getLocalTime(localTime) ||
        !Phase8CalendarPresentation::fromLocalTm(localTime, next)) {
      Serial.println("FAIL: 8B-2 local calendar formatting.");
      delay(1000);
      return;
    }

    // Compare all three fields, not just minute: midnight/NTP corrections
    // must refresh date and weekday even if HH:MM happens to match.
    if (!Phase8CalendarPresentation::equal(phase8B2Displayed, next)) {
      if (!Phase8A1Preview::renderWithCalendar(graphics, next)) {
        Serial.println("FAIL: 8B-2 calendar frame composition.");
        delay(1000);
        return;
      }

      // Preserve the validated application-owned partial refresh sequence.
      // Nothing is committed as displayed until physical refresh succeeds.
      if (!display.begin()) {
        Serial.println("FAIL: 8B-2 E-paper wake.");
        delay(1000);
        return;
      }
      if (!display.restoreFrameStateForPartial(previousFrameBuffer) ||
          !display.displayPartialFrame(frameBuffer)) {
        display.sleep();
        Serial.println("FAIL: 8B-2 E-paper partial refresh.");
        delay(1000);
        return;
      }

      memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
      display.sleep();
      phase8B2Displayed = next;

      Serial.print("PASS: 8B-2 calendar refreshed: ");
      Serial.print(next.date);
      Serial.print(" ");
      Serial.print(next.weekday);
      Serial.print(" ");
      Serial.println(next.time);
    }
  }

  delay(100);
  return;
#endif
#if defined(EDP_PHASE8_WEATHER_ICON_TEST) && (EDP_PHASE8_WEATHER_ICON_TEST == 1)
  // A single static 1-bit diagnostic frame; no background work or refresh.
  delay(1000);
  return;
#endif
#if defined(EDP_PHASE8_FONT_TEST) && (EDP_PHASE8_FONT_TEST == 1)
  // USB Serial commands are exclusively enabled in this local test mode.
  // Unknown bytes (including newline) are ignored; no automatic refresh.
  while (Serial.available() > 0) {
    const char command = static_cast<char>(Serial.read());
    if (command < '1' || command > '5') {
      continue;
    }
    const uint8_t requestedPage = static_cast<uint8_t>(command - '0');
    if (requestedPage == diagnosticFontPage) {
      continue;
    }

    Serial.printf("Rev-C3-JB2 rendering font sheet %u/5...\n", requestedPage);
    if (!DashboardFontTest::render(graphics, requestedPage)) {
      Serial.println("FAIL: font sheet composition failed.");
      continue;
    }
    if (!display.maintenanceRefresh(frameBuffer)) {
      Serial.println("FAIL: font sheet E-paper refresh failed.");
      continue;
    }
    memcpy(previousFrameBuffer, frameBuffer, CrowEPD579::FRAMEBUFFER_BYTES);
    display.sleep();
    diagnosticFontPage = requestedPage;
    Serial.printf("PASS: font sheet %u/5 displayed.\n", diagnosticFontPage);
  }
  delay(30);
  return;
#endif
#if defined(EDP_PHASE8A1_PREVIEW) && (EDP_PHASE8A1_PREVIEW == 1)
  // Deliberately static. No buttons, polling, Wi-Fi, or background refresh.
  delay(1000);
  return;
#endif
  handlePanelInputIfAny();

  if (!processPendingSystemAction()) {
    Serial.println("FAIL: selected DETAIL action did not complete.");
    delay(500);
    return;
  }

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

#if (defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1))
  // 8B-5B coalesces fresh service snapshots BEFORE deciding whether to
  // render the currently visible page; no second poll/refresh owner.
  runPhase5B3MarketPollIfDue();
  runProductionWeatherPollIfDue();
  if (!stagePhase8LiveDashboardFromServices()) {
    delay(1000);
    return;
  }
#endif

  if (!flushVisiblePageChangeIfNeeded()) {
    Serial.println("FAIL: visible page refresh failed.");
    delay(1000);
    return;
  }

  if (!flushLiveDashboardIfNeeded()) {
    Serial.println("FAIL: live dashboard refresh failed.");
  }

#if !(defined(EDP_PHASE8B5B_APP_TEST) && (EDP_PHASE8B5B_APP_TEST == 1))
  runPhase5B3MarketPollIfDue();
  runProductionWeatherPollIfDue();
#endif

  delay(20);
}
