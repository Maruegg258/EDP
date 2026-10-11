#pragma once

#include <time.h>

// Phase 8B-2, owned/pointer-free header typography state.
// The renderer never reads time, Wi-Fi, NTP or any service.
struct Phase8CalendarHeader {
  char time[6];       // HH:MM
  char date[7];       // DD MMM (English, uppercase)
  char weekday[10];   // MONDAY..WEDNESDAY
};

namespace Phase8CalendarPresentation {
bool fromLocalTm(const struct tm& local, Phase8CalendarHeader& out);
bool equal(const Phase8CalendarHeader& a,
           const Phase8CalendarHeader& b);
bool selfCheck();
}  // namespace Phase8CalendarPresentation
