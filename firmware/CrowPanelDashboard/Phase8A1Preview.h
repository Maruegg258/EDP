#pragma once

#include "GraphicsBW.h"
#include "Phase8CalendarPresentation.h"

// Render-only Phase 8A composition. No Wi-Fi, polling, or physical refresh.
// The accepted static fixture remains available as a regression baseline.
namespace Phase8A1Preview {
bool render(GraphicsBW& graphics);
bool renderWithCalendar(GraphicsBW& graphics,
                        const Phase8CalendarHeader& calendar);
}  // namespace Phase8A1Preview
