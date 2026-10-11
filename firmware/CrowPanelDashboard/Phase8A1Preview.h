#pragma once

#include "GraphicsBW.h"
#include "Phase8CalendarPresentation.h"
#include "Phase8MarketPresentation.h"
#include "Phase8WeatherPresentation.h"
#include "Phase8DashboardFrame.h"
#include "Phase8SixDayPresentation.h"

// Render-only Phase 8A composition. No Wi-Fi, polling, or physical refresh.
// The accepted static fixture remains available as a regression baseline.
namespace Phase8A1Preview {
bool render(GraphicsBW& graphics);
// Final live Dashboard always uses truthful unavailable daily forecast text.
bool renderLive(GraphicsBW& graphics, const Phase8DashboardFrame& frame);
// Phase 8C-2: same approved compositor, six REAL bounded future days.
// Phase 8B live route remains UNAVAILABLE until 8C-3 connects state/dirty.
bool renderWithSixDayForecast(
    GraphicsBW& graphics, const Phase8DashboardFrame& frame,
    const Phase8SixDayForecast& forecast);
bool renderWithCalendar(GraphicsBW& graphics,
                        const Phase8CalendarHeader& calendar);
bool renderWithCalendarAndMarkets(GraphicsBW& graphics,
                                  const Phase8CalendarHeader& calendar,
                                  const Phase8MarketPrices& prices);
bool renderWithCalendarMarketsWeather(GraphicsBW& graphics,
                                      const Phase8CalendarHeader& calendar,
                                      const Phase8MarketPrices& prices,
                                      const Phase8CurrentWeather& weather);
bool renderWithCalendarMarketsWeatherWifi(
    GraphicsBW& graphics, const Phase8CalendarHeader& calendar,
    const Phase8MarketPrices& prices, const Phase8CurrentWeather& weather,
    const Bitmap1bpp& wifi);
}  // namespace Phase8A1Preview
