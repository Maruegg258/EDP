#include "Phase8A1Preview.h"

#include <Arduino.h>
#include "Icons.h"
#include "DashboardFont34.h"
#include "DashboardFont17.h"
#include "Phase8A1Logos.h"
#include "DashboardFont14.h"

namespace {

// Phase 8A-1 Rev-C4: user-approved JetBrains Mono Medium 1-bit family.
// 34px for time/prices, 17px for date/weekday/English labels,
// 14px for compact weather temperature and precipitation.
// Draw at native scale=1, with fixed-cell glyph metrics.
// The former Rev-B font assets and optional 13px diagnostic remain
// available only for historical/reference testing, not this preview.
// Phase 8A-1 layout anchors deliberately unchanged in this font-only pass.
// Physical pixel dimensions: 792x272; opt-in static-only composition.
static constexpr int16_t WIDTH = 792;
static constexpr int16_t HEADER_DIVIDER_Y = 77;
static constexpr int16_t FORECAST_DIVIDER_Y = 181;
static constexpr int16_t MARKET_LOGO_Y = 112;
static constexpr int16_t MARKET_PRICE_Y = 106;
static constexpr int16_t MARKET_LABEL_Y = 149;
static constexpr int16_t FORECAST_COLUMN_PITCH = 128;
static constexpr int16_t FORECAST_COLUMN_COUNT = 6;
static constexpr int16_t FORECAST_GRID_LEFT = static_cast<int16_t>(
    (WIDTH - FORECAST_COLUMN_COUNT * FORECAST_COLUMN_PITCH) / 2);
static constexpr int16_t FORECAST_ICON_SIZE = 32;
static constexpr int16_t FORECAST_ICON_LABEL_GAP = 8;

struct PreviewForecast {
  const char* day;
  const char* summary;
};

// Intentionally mock values matching the approved Phase 8A-1 preview.
// These are NOT six live daily forecasts; production WeatherService still
// owns only today and tomorrow.
static const PreviewForecast FORECAST[6] = {
    {"MON", "25-29 / 10 %"},
    {"TUE", "25-29 / 10 %"},
    {"WED", "25-29 / 10 %"},
    {"THU", "25-29 / 10 %"},
    {"FRI", "25-29 / 10 %"},
    {"SAT", "25-29 / 10 %"}
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

// 32x32 matching outline-sun icon shared by upper current weather and
// the six forecast slots, derived from the user's thin-stroke icon reference.
void drawOutlineSun(GraphicsBW& g, int16_t x, int16_t y) {
  static constexpr int16_t CENTER = 16;
  for (int16_t dy = -10; dy <= 10; ++dy) {
    for (int16_t dx = -10; dx <= 10; ++dx) {
      const int16_t distanceSquared =
          static_cast<int16_t>(dx * dx + dy * dy);
      if (distanceSquared >= 72 && distanceSquared <= 94) {
        g.setPixel(x + CENTER + dx,
                   y + CENTER + dy, true);
      }
    }
  }

  static const int8_t RAYS[8][4] = {
    {0,-12,0,-15}, {0,12,0,15}, {-12,0,-15,0}, {12,0,15,0},
    {-9,-9,-11,-11}, {9,-9,11,-11}, {-9,9,-11,11}, {9,9,11,11}
  };
  for (const auto& ray : RAYS) {
    g.drawLine(x + CENTER + ray[0], y + CENTER + ray[1],
               x + CENTER + ray[2], y + CENTER + ray[3], true);
  }
}

bool drawHeader(GraphicsBW& g) {
  drawOutlineSun(g, 22, 25);

  if (!g.drawText(DashboardFont17::FONT, "SUNNY", 69, 27, 1, true) ||
      !g.drawText(DashboardFont14::FONT,
                  "26.9 C", 69, 48, 1, true)) {
    return false;
  }

  // Compose date / diamond / time / diamond / weekday as a centered group.
  // Date/weekday are 17px tall, precisely 50% of the 34px clock height.
  const char* date = "10 OCT";
  const char* time = "12:59";
  const char* weekday = "SUNDAY";
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

bool drawMarket(GraphicsBW& g,
                const Bitmap1bpp& logo,
                int16_t logoX,
                int16_t priceX,
                const char* price) {
  if (price == nullptr || g.textWidth(
          DashboardFont34::FONT, price, 1) == 0 ||
      priceX + g.textWidth(DashboardFont34::FONT, price, 1) > 782) {
    return false;
  }

  return g.drawBitmap(logo, logoX, MARKET_LOGO_Y, true) &&
         g.drawText(DashboardFont34::FONT,
                    price, priceX, MARKET_PRICE_Y, 1, true) &&
         g.drawText(DashboardFont17::FONT,
                    "USDT PERP", priceX, MARKET_LABEL_Y, 1, true);
}

bool drawMarkets(GraphicsBW& g) {
  // No decorative separators: intentional whitespace between instruments.
  // Each logo is a native 30px outline (previous prototype: 48px filled).
  return drawMarket(g, Phase8A1Logos::ETH, 59, 114, "2493.56") &&
         drawMarket(g, Phase8A1Logos::BTC, 307, 366, "82750.1") &&
         drawMarket(g, Phase8A1Logos::HYPE, 557, 628, "84.154");
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

    drawOutlineSun(g, iconX, 200);
    if (!g.drawText(DashboardFont17::FONT, FORECAST[index].day,
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

bool render(GraphicsBW& g) {
  if (g.width() != WIDTH || g.height() != 272) {
    return false;
  }

  g.clear(true);
  if (!drawHeader(g) || !drawMarkets(g) || !drawSixDayForecast(g)) {
    return false;
  }
  drawSectionDivider(g, HEADER_DIVIDER_Y);
  drawSectionDivider(g, FORECAST_DIVIDER_Y);
  return true;
}

}  // namespace Phase8A1Preview
