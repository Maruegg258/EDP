#include "Phase8A1Preview.h"

#include <Arduino.h>
#include "Font5x7.h"
#include "Icons.h"
#include "Phase8A1Digits.h"
#include "Phase8A1Logos.h"

namespace {

struct PreviewForecast {
  const char* day;
  const char* range;
  const char* rain;
  const Bitmap1bpp* icon;
};

// Static samples ONLY. The production WeatherSnapshot currently contains
// today and tomorrow, not the six independent forecast days shown here.
static const PreviewForecast FORECAST[6] = {
    {"MON", "25-29 C", "10%", &Icons::WEATHER_SUN},
    {"TUE", "24-28 C", "15%", &Icons::WEATHER_SUN},
    {"WED", "23-27 C", "30%", &Icons::WEATHER_CLOUD},
    {"THU", "24-29 C", "60%", &Icons::WEATHER_RAIN},
    {"FRI", "23-26 C", "45%", &Icons::WEATHER_CLOUD},
    {"SAT", "24-28 C", "20%", &Icons::WEATHER_SUN},
};

void diamond(GraphicsBW& g, int16_t cx, int16_t cy) {
  for (int16_t dy = -4; dy <= 4; ++dy) {
    const int16_t radius = static_cast<int16_t>(4 - abs(dy));
    g.drawLine(cx - radius, cy + dy, cx + radius, cy + dy, true);
  }
}

void sparkle(GraphicsBW& g, int16_t x, int16_t y) {
  // Slender four-point star from the PDF, independent of font glyphs.
  for (int16_t dy = -10; dy <= 10; ++dy) {
    const int16_t width = (abs(dy) < 3) ? 2 : (abs(dy) < 6 ? 1 : 0);
    g.drawLine(x - width, y + dy, x + width, y + dy, true);
  }
  g.drawLine(x - 5, y, x + 5, y, true);
}

void horizontalRule(GraphicsBW& g, int16_t y) {
  g.drawLine(19, y, 773, y, true);
  diamond(g, 18, y);
  diamond(g, 774, y);
}

void sun(GraphicsBW& g, int16_t cx, int16_t cy) {
  // Bold 1-bit icon in the upper-left, similar in weight to PDF artwork.
  for (int16_t y = -12; y <= 12; ++y) {
    for (int16_t x = -12; x <= 12; ++x) {
      if (x * x + y * y <= 12 * 12) {
        g.setPixel(cx + x, cy + y, true);
      }
    }
  }
  const int16_t directions[8][4] = {
    {0,-17,0,-23}, {0,17,0,23}, {-17,0,-23,0}, {17,0,23,0},
    {-13,-13,-18,-18}, {13,-13,18,-18},
    {-13,13,-18,18}, {13,13,18,18}
  };
  for (const auto& d : directions) {
    g.drawLine(cx + d[0], cy + d[1], cx + d[2], cy + d[3], true);
  }
}

bool drawMarket(GraphicsBW& g,
                const Bitmap1bpp& logo,
                int16_t logoX,
                int16_t priceX,
                const char* samplePrice) {
  return g.drawBitmap(logo, logoX, 94, true) &&
         g.drawText(Phase8A1Digits::FONT,
                    samplePrice, priceX, 101, 2, true) &&
         g.drawText(Font5x7::FONT,
                    "USDT PERP", priceX, 148, 2, true);
}

bool drawForecast(GraphicsBW& g) {
  for (uint8_t index = 0; index < 6; ++index) {
    const int16_t offset = static_cast<int16_t>(index * 128);
    const PreviewForecast& row = FORECAST[index];
    if (!g.drawBitmap(*row.icon, 14 + offset, 208, true) ||
        !g.drawText(Font5x7::FONT, row.day, 55 + offset, 201, 2, true) ||
        !g.drawText(Phase8A1Digits::FONT, row.range,
                    55 + offset, 221, 1, true) ||
        !g.drawText(Phase8A1Digits::FONT, row.rain,
                    55 + offset, 246, 1, true)) {
      return false;
    }
  }
  return true;
}

}  // namespace

namespace Phase8A1Preview {

bool render(GraphicsBW& g) {
  // Explicitly reject accidental use with a different panel geometry.
  if (g.width() != 792 || g.height() != 272) {
    return false;
  }

  g.clear(true);

  // Row 1: current weather | date * time * weekday | Wi-Fi.
  sun(g, 37, 30);
  if (!g.drawText(Font5x7::FONT, "SUNNY", 75, 15, 2, true) ||
      !g.drawText(Phase8A1Digits::FONT, "26.9 C", 75, 37, 1, true) ||
      !g.drawText(Font5x7::FONT, "10 OCT", 216, 27, 3, true) ||
      !g.drawText(Phase8A1Digits::FONT, "12:59", 354, 13, 2, true) ||
      !g.drawText(Font5x7::FONT, "SUNDAY", 511, 27, 3, true) ||
      !g.drawBitmap(Icons::WIFI_STRONG, 745, 15, true)) {
    return false;
  }
  sparkle(g, 337, 38);
  sparkle(g, 493, 38);
  horizontalRule(g, 67);

  // Row 2: ETH / BTC / HYPE, with PDF-style sparkle dividers.
  if (!drawMarket(g, Phase8A1Logos::ETH, 28, 91, "2493.56") ||
      !drawMarket(g, Phase8A1Logos::BTC, 291, 353, "82750.1") ||
      !drawMarket(g, Phase8A1Logos::HYPE, 553, 616, "84.154")) {
    return false;
  }
  sparkle(g, 265, 124);
  sparkle(g, 530, 124);
  horizontalRule(g, 181);

  // Row 3: tomorrow through next six days (deliberately fake samples).
  return drawForecast(g);
}

}  // namespace Phase8A1Preview
