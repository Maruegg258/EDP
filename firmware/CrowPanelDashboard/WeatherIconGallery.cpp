#include "WeatherIconGallery.h"

#include "DashboardFont17.h"
#include "WeatherIconAssets.h"

namespace {

static constexpr int16_t kWidth = 792;
static constexpr int16_t kHeight = 272;
static constexpr int16_t kColumns = 4;
static constexpr int16_t kPitch = 192;
static constexpr int16_t kGridLeft = 12;
static constexpr int16_t kIconSize = 32;
static constexpr int16_t kFirstIconY = 47;
static constexpr int16_t kRowPitch = 74;

struct GalleryItem {
  const char* label;
  const Bitmap1bpp* icon;
};

static const GalleryItem ITEMS[] = {
    {"SUN DAY", &WeatherIconAssets::CLEAR_DAY},
    {"MOON NIGHT", &WeatherIconAssets::CLEAR_NIGHT},
    {"PARTLY DAY", &WeatherIconAssets::PARTLY_DAY},
    {"PARTLY NIGHT", &WeatherIconAssets::PARTLY_NIGHT},
    {"CLOUD", &WeatherIconAssets::OVERCAST},
    {"FOG", &WeatherIconAssets::FOG},
    {"DRIZZLE", &WeatherIconAssets::DRIZZLE},
    {"RAIN", &WeatherIconAssets::RAIN},
    {"SNOW", &WeatherIconAssets::SNOW},
    {"THUNDER", &WeatherIconAssets::THUNDER},
    {"UNKNOWN", &WeatherIconAssets::UNKNOWN}
};

bool drawCenteredText(GraphicsBW& g, const char* label,
                      int16_t centerX, int16_t y, uint16_t maximum) {
  const uint16_t width = g.textWidth(DashboardFont17::FONT, label, 1);
  if (width == 0 || width > maximum || y < 0 ||
      y + DashboardFont17::FONT.lineHeight > kHeight) {
    return false;
  }
  return g.drawText(DashboardFont17::FONT, label,
                    static_cast<int16_t>(centerX - width / 2), y, 1, true);
}

}  // namespace

namespace WeatherIconGallery {

bool render(GraphicsBW& g) {
  if (g.width() != kWidth || g.height() != kHeight) {
    return false;
  }
  g.clear(true);

  if (!g.drawText(DashboardFont17::FONT, "WEATHER ICONS", 24, 8, 1, true) ||
      !g.drawText(DashboardFont17::FONT, "11 TYPES / 32 X 32",
                  540, 8, 1, true)) {
    return false;
  }
  g.drawLine(24, 34, 768, 34, true);

  for (uint8_t i = 0; i < sizeof(ITEMS) / sizeof(ITEMS[0]); ++i) {
    const int16_t col = i % kColumns;
    const int16_t row = i / kColumns;
    const int16_t centerX = static_cast<int16_t>(
        kGridLeft + col * kPitch + kPitch / 2);
    const int16_t iconX = static_cast<int16_t>(centerX - kIconSize / 2);
    const int16_t iconY = static_cast<int16_t>(
        kFirstIconY + row * kRowPitch);
    if (ITEMS[i].icon == nullptr ||
        ITEMS[i].icon->width != kIconSize ||
        ITEMS[i].icon->height != kIconSize ||
        iconX < 0 || iconX + kIconSize > kWidth ||
        iconY < 0 || iconY + kIconSize > kHeight ||
        !g.drawBitmap(*ITEMS[i].icon, iconX, iconY, true) ||
        !drawCenteredText(g, ITEMS[i].label, centerX,
                          static_cast<int16_t>(iconY + 36),
                          static_cast<uint16_t>(kPitch - 8))) {
      return false;
    }
  }
  return true;
}

}  // namespace WeatherIconGallery
