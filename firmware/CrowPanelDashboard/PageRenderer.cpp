#include "PageRenderer.h"

#include "Font5x7.h"

namespace {

constexpr int16_t PAGE_BORDER_X = 8;
constexpr int16_t PAGE_BORDER_Y = 8;
constexpr uint16_t PAGE_BORDER_WIDTH = 776;
constexpr uint16_t PAGE_BORDER_HEIGHT = 256;

}  // namespace

PageRenderer::PageRenderer(
    GraphicsBW& graphics,
    Dashboard& dashboard)
    : _graphics(graphics),
      _dashboard(dashboard) {
}

bool PageRenderer::render(
    PageId page,
    const DashboardState& dashboardState) {
  switch (page) {
    case PageId::DASHBOARD:
      return _dashboard.render(dashboardState);

    case PageId::WEATHER:
      return renderPlaceholder("WEATHER");

    case PageId::MARKETS:
      return renderPlaceholder("MARKETS");
  }

  return false;
}

int16_t PageRenderer::centeredTextX(
    const char* text,
    uint8_t scale) const {
  const uint16_t width =
      _graphics.textWidth(Font5x7::FONT, text, scale);

  if (width >= _graphics.width()) {
    return 0;
  }

  return static_cast<int16_t>(
      (_graphics.width() - width) / 2U
  );
}

bool PageRenderer::renderPlaceholder(const char* title) {
  if (title == nullptr) {
    return false;
  }

  _graphics.clear(true);

  _graphics.drawRect(
      PAGE_BORDER_X,
      PAGE_BORDER_Y,
      PAGE_BORDER_WIDTH,
      PAGE_BORDER_HEIGHT,
      true
  );

  _graphics.drawLine(20, 68, 772, 68, true);

  if (!_graphics.drawText(
          Font5x7::FONT,
          title,
          centeredTextX(title, 4),
          92,
          4,
          true)) {
    return false;
  }

  static constexpr char PLACEHOLDER_TEXT[] = "PAGE PLACEHOLDER";
  if (!_graphics.drawText(
          Font5x7::FONT,
          PLACEHOLDER_TEXT,
          centeredTextX(PLACEHOLDER_TEXT, 2),
          154,
          2,
          true)) {
    return false;
  }

  static constexpr char PHASE_TEXT[] = "PHASE 7C-1";
  return _graphics.drawText(
      Font5x7::FONT,
      PHASE_TEXT,
      centeredTextX(PHASE_TEXT, 1),
      205,
      1,
      true
  );
}
