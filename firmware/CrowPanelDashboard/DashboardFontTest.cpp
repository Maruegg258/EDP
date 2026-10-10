#include "DashboardFontTest.h"

#include "DashboardFont34.h"
#include "DashboardFont17.h"
#include "DashboardFont13.h"
#include "DashboardFont14.h"

namespace {

// 792x272 visible pixels; a diagnostic frame, NOT the final dashboard layout.
static constexpr int16_t kPanelWidth = 792;
static constexpr int16_t kPanelHeight = 272;
static constexpr int16_t kContentLeft = 24;
static constexpr int16_t kContentRight = 768;

bool text(GraphicsBW& g, const BitmapFont& font,
          const char* value, int16_t x, int16_t y) {
  if (value == nullptr || x < 0 || y < 0 ||
      y + font.lineHeight > kPanelHeight) {
    return false;
  }
  const uint16_t width = g.textWidth(font, value, 1);
  // A zero measurement means unsupported glyph, empty string, or overflow.
  if (width == 0 || x + width > kPanelWidth) {
    return false;
  }
  return g.drawText(font, value, x, y, 1, true);
}

bool textCentered(GraphicsBW& g, const BitmapFont& font,
                  const char* value, int16_t centerX, int16_t y) {
  const uint16_t width = g.textWidth(font, value, 1);
  if (width == 0 || width > kPanelWidth) {
    return false;
  }
  return text(g, font, value, static_cast<int16_t>(
      centerX - static_cast<int16_t>(width) / 2), y);
}

void rule(GraphicsBW& g, int16_t y) {
  g.drawLine(kContentLeft, y, kContentRight, y, true);
}

bool heading(GraphicsBW& g, const char* label, uint8_t page) {
  char count[] = "PAGE 1 OF 5";
  count[5] = static_cast<char>('0' + page);
  return text(g, DashboardFont17::FONT, label, 24, 8) &&
         text(g, DashboardFont17::FONT, count, 610, 8);
}

bool page34(GraphicsBW& g) {
  if (!heading(g, "JB MONO 34", 1)) return false;
  rule(g, 36);

  return text(g, DashboardFont34::FONT, "0123456789", 38, 48) &&
         text(g, DashboardFont34::FONT, "12:59", 38, 96) &&
         text(g, DashboardFont34::FONT, "2493.56", 300, 96) &&
         text(g, DashboardFont34::FONT, "82750.1", 38, 144) &&
         text(g, DashboardFont34::FONT, "84.154", 300, 144) &&
         text(g, DashboardFont34::FONT, "+-.: 012345", 38, 192) &&
         text(g, DashboardFont17::FONT, "FIXED 22 X 34", 38, 241);
}

bool page17(GraphicsBW& g) {
  if (!heading(g, "JB MONO 17", 2)) return false;
  rule(g, 36);

  return text(g, DashboardFont17::FONT, "ABCDEFGHIJKLM", 38, 49) &&
         text(g, DashboardFont17::FONT, "NOPQRSTUVWXYZ", 38, 83) &&
         text(g, DashboardFont17::FONT, "0123456789", 38, 117) &&
         text(g, DashboardFont17::FONT, "10 OCT SUNDAY", 38, 151) &&
         text(g, DashboardFont17::FONT, "SUNNY USDT PERP", 270, 151) &&
         text(g, DashboardFont17::FONT, "MON TUE WED THU FRI SAT", 38, 189) &&
         text(g, DashboardFont17::FONT, "0123456789", 366, 226);
}

bool page13(GraphicsBW& g) {
  if (!heading(g, "JB MONO 13", 3)) return false;
  rule(g, 36);

  return text(g, DashboardFont13::FONT, "0123456789", 38, 53) &&
         text(g, DashboardFont13::FONT, "26.9 C  25-29 / 10 %", 38, 88) &&
         text(g, DashboardFont13::FONT, "-12.5 C  25-29 / 100 %", 38, 123) &&
         text(g, DashboardFont13::FONT, "+3.2 C  -30-40 / 95 %", 38, 158) &&
         text(g, DashboardFont13::FONT, "0123456789", 370, 198) &&
         text(g, DashboardFont13::FONT, "100 %  -12.5 C", 370, 231);
}

bool page14(GraphicsBW& g) {
  if (!heading(g, "JB MONO 14", 4)) return false;
  rule(g, 36);

  return text(g, DashboardFont14::FONT, "0123456789", 38, 53) &&
         text(g, DashboardFont14::FONT, "26.9 C  25-29 / 10 %", 38, 88) &&
         text(g, DashboardFont14::FONT, "-12.5 C  25-29 / 100 %", 38, 123) &&
         text(g, DashboardFont14::FONT, "+3.2 C  -30-40 / 95 %", 38, 158) &&
         text(g, DashboardFont14::FONT, "0123456789", 370, 198) &&
         text(g, DashboardFont14::FONT, "100 %  -12.5 C", 370, 231);
}

bool comparison13vs14(GraphicsBW& g) {
  if (!heading(g, "JB MONO 13 VS 14", 5)) return false;
  rule(g, 36);
  g.drawLine(396, 43, 396, 259, true);
  return text(g, DashboardFont17::FONT, "13 PX", 34, 49) &&
         text(g, DashboardFont17::FONT, "14 PX", 414, 49) &&
         text(g, DashboardFont13::FONT, "0123456789", 34, 83) &&
         text(g, DashboardFont14::FONT, "0123456789", 414, 83) &&
         text(g, DashboardFont13::FONT, "26.9 C", 34, 112) &&
         text(g, DashboardFont14::FONT, "26.9 C", 414, 112) &&
         text(g, DashboardFont13::FONT, "25-29 / 10 %", 34, 141) &&
         text(g, DashboardFont14::FONT, "25-29 / 10 %", 414, 141) &&
         text(g, DashboardFont13::FONT, "-12.5 C  25-29 / 100 %", 34, 170) &&
         text(g, DashboardFont14::FONT, "-12.5 C  25-29 / 100 %", 414, 170) &&
         text(g, DashboardFont13::FONT, "+3.2 C  -30-40 / 95 %", 34, 199) &&
         text(g, DashboardFont14::FONT, "+3.2 C  -30-40 / 95 %", 414, 199) &&
         text(g, DashboardFont13::FONT, "100 %  -12.5 C", 34, 228) &&
         text(g, DashboardFont14::FONT, "100 %  -12.5 C", 414, 228);
}

}  // namespace

namespace DashboardFontTest {

bool render(GraphicsBW& g, uint8_t page) {
  if (g.width() != kPanelWidth || g.height() != kPanelHeight ||
      page < 1 || page > PAGE_COUNT) {
    return false;
  }

  g.clear(true);
  switch (page) {
    case 1: return page34(g);
    case 2: return page17(g);
    case 3: return page13(g);
    case 4: return page14(g);
    case 5: return comparison13vs14(g);
    default: return false;
  }
}

}  // namespace DashboardFontTest
