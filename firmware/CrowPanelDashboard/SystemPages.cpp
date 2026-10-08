#include "SystemPages.h"
#include "Font5x7.h"

namespace {
constexpr int16_t LEFT = 80;
constexpr int16_t MOON_X = 396;
constexpr int16_t MOON_Y = 136;
constexpr int16_t RADIUS = 26;
}  // namespace

bool SystemPages::renderMenu(DashboardAction selection) {
  _graphics.clear(true);
  _graphics.drawRect(8, 8, 776, 256, true);
  _graphics.drawLine(24, 54, 768, 54, true);
  _graphics.drawLine(24, 226, 768, 226, true);

  return _graphics.drawText(
             Font5x7::FONT, "DETAIL", 28, 22, 3, true) &&
         _graphics.drawText(
             Font5x7::FONT,
             selection == DashboardAction::STANDBY ? "+" : " ",
             LEFT - 34, 89, 3, true) &&
         _graphics.drawText(
             Font5x7::FONT, "STANDBY", LEFT, 89, 3, true) &&
         _graphics.drawText(
             Font5x7::FONT,
             selection == DashboardAction::DISPLAY_CLEAN ? "+" : " ",
             LEFT - 34, 153, 3, true) &&
         _graphics.drawText(
             Font5x7::FONT, "DISPLAY CLEAN", LEFT, 153, 3, true) &&
         _graphics.drawText(
             Font5x7::FONT, "MENU SELECT     EXIT BACK",
             LEFT, 237, 1, true);
}

void SystemPages::renderStandby() {
  _graphics.clear(true);

  // Compact black crescent on a white E-paper background.
  // Offset disk subtraction leaves a recognizable moon without assets.
  for (int16_t dy = -RADIUS; dy <= RADIUS; ++dy) {
    for (int16_t dx = -RADIUS; dx <= RADIUS; ++dx) {
      const int32_t outer = static_cast<int32_t>(dx) * dx +
                            static_cast<int32_t>(dy) * dy;
      const int32_t cutDx = dx - 12;
      const int32_t cutDy = dy + 7;
      const int32_t inner = cutDx * cutDx + cutDy * cutDy;
      if (outer <= RADIUS * RADIUS &&
          inner > RADIUS * RADIUS) {
        _graphics.setPixel(MOON_X + dx, MOON_Y + dy, true);
      }
    }
  }
}
