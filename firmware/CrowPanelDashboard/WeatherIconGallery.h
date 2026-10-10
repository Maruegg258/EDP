#pragma once

#include "GraphicsBW.h"

// Opt-in Phase 8A-2 art gallery: renders one 792x272 static frame.
// It never initiates refresh, Wi-Fi, or any polling.
namespace WeatherIconGallery {
bool render(GraphicsBW& graphics);
}  // namespace WeatherIconGallery
