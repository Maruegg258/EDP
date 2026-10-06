#pragma once

#include <Arduino.h>

enum class PageId : uint8_t {
  DASHBOARD,
  WEATHER,
  MARKETS
};

namespace PageModel {

static constexpr PageId PAGES[] = {
  PageId::DASHBOARD,
  PageId::WEATHER,
  PageId::MARKETS
};

static constexpr uint8_t PAGE_COUNT =
    static_cast<uint8_t>(sizeof(PAGES) / sizeof(PAGES[0]));

inline PageId pageAt(uint8_t index) {
  return PAGES[index < PAGE_COUNT ? index : 0];
}

inline const char* pageName(PageId page) {
  switch (page) {
    case PageId::DASHBOARD:
      return "DASHBOARD";
    case PageId::WEATHER:
      return "WEATHER";
    case PageId::MARKETS:
      return "MARKETS";
  }

  return "UNKNOWN";
}

}  // namespace PageModel
