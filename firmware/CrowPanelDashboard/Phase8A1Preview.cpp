#include "Phase8A1Preview.h"

#include <Arduino.h>
#include "Icons.h"
#include "WeatherIconSelector.h"
#include "DashboardFont34.h"
#include "DashboardFont17.h"
#include "DashboardFont14.h"

namespace {

// Phase 8A-2D: approved weather icon family in the isolated Rev-D2 Dashboard.
// Preserve JetBrains Mono typography, market row and fixed layout anchors.
// 34px for time/prices, 17px for date/weekday/English labels,
// 14px for compact weather temperature and precipitation.
// Draw at native scale=1, with fixed-cell glyph metrics.
// The former Rev-B font assets and optional 13px diagnostic remain
// available only for historical/reference testing, not this preview.
// Fixed mock values only; no live weather or dynamic update path in preview.
// Physical pixel dimensions: 792x272; opt-in static-only composition.
static constexpr int16_t WIDTH = 792;
static constexpr int16_t HEADER_DIVIDER_Y = 77;
static constexpr int16_t FORECAST_DIVIDER_Y = 181;
static constexpr int16_t MARKET_COLUMN_COUNT = 3;
static constexpr int16_t MARKET_COLUMN_PITCH = 248;
static constexpr int16_t MARKET_GRID_LEFT = 24;
static constexpr int16_t MARKET_SYMBOL_Y = 101;
static constexpr int16_t MARKET_PRICE_Y = 126;
static constexpr int16_t FORECAST_COLUMN_PITCH = 128;
static constexpr int16_t FORECAST_COLUMN_COUNT = 6;
static constexpr int16_t FORECAST_GRID_LEFT = static_cast<int16_t>(
    (WIDTH - FORECAST_COLUMN_COUNT * FORECAST_COLUMN_PITCH) / 2);
static constexpr int16_t FORECAST_ICON_SIZE = 32;
static constexpr int16_t FORECAST_ICON_LABEL_GAP = 8;

struct PreviewForecast {
  const char* day;
  const char* summary;
  WeatherCondition condition;  // Static, illustrative daily icon category.
};

// Intentionally mock values matching the approved Phase 8A-1 preview.
// These are NOT six live daily forecasts; production WeatherService still
// owns only today and tomorrow.
static const PreviewForecast FORECAST[6] = {
    {"MON", "25-29/ 10%", WeatherCondition::CLEAR},
    {"TUE", "25-29/ 10%", WeatherCondition::PARTLY_CLOUDY},
    {"WED", "25-29/ 10%", WeatherCondition::OVERCAST},
    {"THU", "25-29/ 10%", WeatherCondition::DRIZZLE},
    {"FRI", "25-29/ 10%", WeatherCondition::RAIN},
    {"SAT", "25-29/ 10%", WeatherCondition::THUNDERSTORM}
};

void drawDiamond(GraphicsBW& g, int16_t cx, int16_t cy, int16_t size = 3) {
  for (int16_t dy = -size; dy <= size; ++dy) {
    const int16_t halfWidth = static_cast<int16_t>(
        size - (dy < 0 ? -dy : dy));
    g.drawLine(cx - halfWidth, cy + dy,
               cx + halfWidth, cy + dy, true);
  }
}

void drawSectionDivider(GraphicsBW& g, int16_t y) {
  g.drawLine(16, y, 776, y, true);
  drawDiamond(g, 16, y);
  drawDiamond(g, 776, y);
}

bool drawHeader(GraphicsBW& g, const Phase8CalendarHeader& calendar) {
  // Static current-weather example: sunny daytime. Same 32x32 slot as Rev-D2.
  if (!g.drawBitmap(
          WeatherIconSelector::select(WeatherCondition::CLEAR, true),
          22, 25, true)) {
    return false;
  }

  if (!g.drawText(DashboardFont17::FONT, "SUNNY", 69, 27, 1, true) ||
      !g.drawText(DashboardFont14::FONT,
                  "26.9 C", 69, 48, 1, true)) {
    return false;
  }

  // Compose date / diamond / time / diamond / weekday as a centered group.
  // Date/weekday are 17px tall, precisely 50% of the 34px clock height.
  const char* date = calendar.date;
  const char* time = calendar.time;
  const char* weekday = calendar.weekday;
  const uint16_t dateWidth = g.textWidth(DashboardFont17::FONT, date, 1);
  const uint16_t timeWidth = g.textWidth(DashboardFont34::FONT, time, 1);
  const uint16_t dayWidth = g.textWidth(DashboardFont17::FONT, weekday, 1);
  if (dateWidth == 0 || timeWidth == 0 || dayWidth == 0) {
    return false;
  }

  // Each diamond has 6px of width and 18px of whitespace on either side.
  static constexpr int16_t DIAMOND_SECTION_WIDTH = 42;
  const int16_t totalWidth = static_cast<int16_t>(
      dateWidth + timeWidth + dayWidth + 2 * DIAMOND_SECTION_WIDTH);
  if (totalWidth > 520) {
    return false;
  }

  const int16_t startX = static_cast<int16_t>((WIDTH - totalWidth) / 2);
  const int16_t timeX = static_cast<int16_t>(
      startX + dateWidth + DIAMOND_SECTION_WIDTH);
  const int16_t dayX = static_cast<int16_t>(
      timeX + timeWidth + DIAMOND_SECTION_WIDTH);
  const int16_t firstDiamondX = static_cast<int16_t>(
      startX + dateWidth + DIAMOND_SECTION_WIDTH / 2);
  const int16_t secondDiamondX = static_cast<int16_t>(
      timeX + timeWidth + DIAMOND_SECTION_WIDTH / 2);

  // All three have a common vertical center at y=42.
  if (!g.drawText(DashboardFont17::FONT, date, startX, 34, 1, true) ||
      !g.drawText(DashboardFont34::FONT, time,
                  timeX, 25, 1, true) ||
      !g.drawText(DashboardFont17::FONT, weekday,
                  dayX, 34, 1, true) ||
      !g.drawBitmap(Icons::WIFI_STRONG, 738, 25, true)) {
    return false;
  }

  drawDiamond(g, firstDiamondX, 42);
  drawDiamond(g, secondDiamondX, 42);
  return true;
}

struct MarketFixture {
  const char* symbol;
  const char* price;
};

static const MarketFixture MARKETS[MARKET_COLUMN_COUNT] = {
    {"ETH", "2493.56"},
    {"BTC", "82750.1"},
    {"HYPE", "84.154"}
};

bool drawMarkets(GraphicsBW& g) {
  // Rev-D2: text-only labels above prices, each centered within its column.
  for (uint8_t i = 0; i < MARKET_COLUMN_COUNT; ++i) {
    const uint16_t labelWidth =
        g.textWidth(DashboardFont17::FONT, MARKETS[i].symbol, 1);
    const uint16_t priceWidth =
        g.textWidth(DashboardFont34::FONT, MARKETS[i].price, 1);
    if (labelWidth == 0 || priceWidth == 0 ||
        labelWidth > MARKET_COLUMN_PITCH - 24 ||
        priceWidth > MARKET_COLUMN_PITCH - 24) {
      return false;
    }

    const int16_t columnX = static_cast<int16_t>(
        MARKET_GRID_LEFT + i * MARKET_COLUMN_PITCH);
    const int16_t labelX = static_cast<int16_t>(
        columnX + (MARKET_COLUMN_PITCH - labelWidth) / 2);
    const int16_t priceX = static_cast<int16_t>(
        columnX + (MARKET_COLUMN_PITCH - priceWidth) / 2);
    if (!g.drawText(DashboardFont17::FONT, MARKETS[i].symbol,
                    labelX, MARKET_SYMBOL_Y, 1, true) ||
        !g.drawText(DashboardFont34::FONT, MARKETS[i].price,
                    priceX, MARKET_PRICE_Y, 1, true)) {
      return false;
    }
  }
  return true;
}

bool drawSixDayForecast(GraphicsBW& g) {
  // Six equal columns centered across the 792px screen.
  // Center the 32px icon + weekday as one unit and the 14px summary
  // independently; never approximate text widths or stretch the glyphs.
  for (uint8_t index = 0; index < FORECAST_COLUMN_COUNT; ++index) {
    const int16_t columnLeft = static_cast<int16_t>(
        FORECAST_GRID_LEFT + index * FORECAST_COLUMN_PITCH);
    const uint16_t weekdayWidth = g.textWidth(
        DashboardFont17::FONT, FORECAST[index].day, 1);
    const uint16_t summaryWidth = g.textWidth(
        DashboardFont14::FONT, FORECAST[index].summary, 1);
    const uint16_t iconAndWeekdayWidth = static_cast<uint16_t>(
        FORECAST_ICON_SIZE + FORECAST_ICON_LABEL_GAP + weekdayWidth);

    // Leave a minimum 6px inset on either side of the summary.
    if (weekdayWidth == 0 || summaryWidth == 0 ||
        iconAndWeekdayWidth > FORECAST_COLUMN_PITCH ||
        summaryWidth > FORECAST_COLUMN_PITCH - 12) {
      return false;
    }

    const int16_t iconX = static_cast<int16_t>(
        columnLeft + (FORECAST_COLUMN_PITCH - iconAndWeekdayWidth) / 2);
    const int16_t weekdayX = static_cast<int16_t>(
        iconX + FORECAST_ICON_SIZE + FORECAST_ICON_LABEL_GAP);
    const int16_t summaryX = static_cast<int16_t>(
        columnLeft + (FORECAST_COLUMN_PITCH - summaryWidth) / 2);

    // Daily representatives use daytime artwork. These are mock conditions,
    // not hourly isDay values or live six-day WeatherService predictions.
    const Bitmap1bpp& icon =
        WeatherIconSelector::select(FORECAST[index].condition, true);
    if (icon.width != FORECAST_ICON_SIZE ||
        icon.height != FORECAST_ICON_SIZE ||
        !g.drawBitmap(icon, iconX, 200, true) ||
        !g.drawText(DashboardFont17::FONT, FORECAST[index].day,
                    weekdayX, 209, 1, true) ||
        !g.drawText(DashboardFont14::FONT,
                    FORECAST[index].summary,
                    summaryX, 243, 1, true)) {
      return false;
    }
  }
  return true;
}

}  // namespace

namespace Phase8A1Preview {

bool renderWithCalendar(GraphicsBW& g,
                        const Phase8CalendarHeader& calendar) {
  if (g.width() != WIDTH || g.height() != 272) {
    return false;
  }

  // Validate required glyphs/size before drawing; no silent clipped text.
  if (strlen(calendar.time) != 5 ||
      strlen(calendar.date) != 6 ||
      strlen(calendar.weekday) < 6 ||
      strlen(calendar.weekday) > 9 ||
      g.textWidth(DashboardFont34::FONT, calendar.time, 1) != 5 * 22 ||
      g.textWidth(DashboardFont17::FONT, calendar.date, 1) != 6 * 12 ||
      g.textWidth(DashboardFont17::FONT, calendar.weekday, 1) !=
          strlen(calendar.weekday) * 12) {
    return false;
  }

  g.clear(true);
  if (!drawHeader(g, calendar) ||
      !drawMarkets(g) || !drawSixDayForecast(g)) {
    return false;
  }
  drawSectionDivider(g, HEADER_DIVIDER_Y);
  drawSectionDivider(g, FORECAST_DIVIDER_Y);
  return true;
}

bool render(GraphicsBW& g) {
  // Phase 8A user-accepted immutable one-shot regression fixture.
  static const Phase8CalendarHeader fixture = {
      "12:59", "10 OCT", "SUNDAY"
  };
  return renderWithCalendar(g, fixture);
}

}  // namespace Phase8A1Preview
