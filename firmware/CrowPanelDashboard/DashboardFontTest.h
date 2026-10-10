#pragma once

#include <Arduino.h>
#include "GraphicsBW.h"

// Phase 8A-1 Rev-C3. Pure, font-only renderer; it owns neither E-paper
// refresh nor Wi-Fi / state services. Page numbers are 1..4.
namespace DashboardFontTest {
static constexpr uint8_t PAGE_COUNT = 4;
bool render(GraphicsBW& graphics, uint8_t page);
}
