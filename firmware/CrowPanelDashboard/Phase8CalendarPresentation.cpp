#include "Phase8CalendarPresentation.h"

#include <cstdio>
#include <cstring>

namespace {

static const char* const MONTHS[12] = {
    "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
    "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

static const char* const DAYS[7] = {
    "SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY",
    "THURSDAY", "FRIDAY", "SATURDAY"
};

}  // namespace

namespace Phase8CalendarPresentation {

bool fromLocalTm(const struct tm& local, Phase8CalendarHeader& out) {
  if (local.tm_mon < 0 || local.tm_mon >= 12 ||
      local.tm_wday < 0 || local.tm_wday >= 7 ||
      local.tm_mday < 1 || local.tm_mday > 31 ||
      local.tm_hour < 0 || local.tm_hour > 23 ||
      local.tm_min < 0 || local.tm_min > 59) {
    return false;
  }

  Phase8CalendarHeader candidate{};
  const int timeLength = snprintf(
      candidate.time, sizeof(candidate.time),
      "%02d:%02d", local.tm_hour, local.tm_min);
  const int dateLength = snprintf(
      candidate.date, sizeof(candidate.date),
      "%02d %s", local.tm_mday, MONTHS[local.tm_mon]);
  const int dayLength = snprintf(
      candidate.weekday, sizeof(candidate.weekday),
      "%s", DAYS[local.tm_wday]);

  if (timeLength != 5 || dateLength != 6 ||
      dayLength < 6 || dayLength > 9) {
    return false;
  }

  out = candidate;  // No dangling pointers to local/temporary text.
  return true;
}

bool equal(const Phase8CalendarHeader& a,
           const Phase8CalendarHeader& b) {
  return strcmp(a.time, b.time) == 0 &&
         strcmp(a.date, b.date) == 0 &&
         strcmp(a.weekday, b.weekday) == 0;
}

bool selfCheck() {
  struct tm sunday{};
  sunday.tm_year = 126;  // 2026
  sunday.tm_mon = 9;    // October
  sunday.tm_mday = 11;
  sunday.tm_wday = 0;
  sunday.tm_hour = 14;
  sunday.tm_min = 2;

  Phase8CalendarHeader base{};
  if (!fromLocalTm(sunday, base) ||
      strcmp(base.time, "14:02") != 0 ||
      strcmp(base.date, "11 OCT") != 0 ||
      strcmp(base.weekday, "SUNDAY") != 0) {
    return false;
  }

  Phase8CalendarHeader same = base;
  if (!equal(base, same)) return false;

  struct tm nextMinute = sunday;
  nextMinute.tm_min = 3;
  Phase8CalendarHeader minuteValue{};
  if (!fromLocalTm(nextMinute, minuteValue) ||
      equal(base, minuteValue) ||
      strcmp(minuteValue.time, "14:03") != 0) {
    return false;
  }

  struct tm yearEnd{};
  yearEnd.tm_year = 126;
  yearEnd.tm_mon = 11;
  yearEnd.tm_mday = 31;
  yearEnd.tm_wday = 4;  // Thursday
  yearEnd.tm_hour = 23;
  yearEnd.tm_min = 59;
  Phase8CalendarHeader before{};
  if (!fromLocalTm(yearEnd, before) ||
      strcmp(before.date, "31 DEC") != 0 ||
      strcmp(before.weekday, "THURSDAY") != 0) {
    return false;
  }

  struct tm yearStart{};
  yearStart.tm_year = 127;
  yearStart.tm_mon = 0;
  yearStart.tm_mday = 1;
  yearStart.tm_wday = 5;  // Friday
  yearStart.tm_hour = 0;
  yearStart.tm_min = 0;
  Phase8CalendarHeader after{};
  if (!fromLocalTm(yearStart, after) ||
      equal(before, after) ||
      strcmp(after.time, "00:00") != 0 ||
      strcmp(after.date, "01 JAN") != 0 ||
      strcmp(after.weekday, "FRIDAY") != 0) {
    return false;
  }

  struct tm wednesday = sunday;
  wednesday.tm_wday = 3;
  Phase8CalendarHeader longest{};
  if (!fromLocalTm(wednesday, longest) ||
      strcmp(longest.weekday, "WEDNESDAY") != 0) {
    return false;
  }

  struct tm bad = sunday;
  bad.tm_mon = 12;
  Phase8CalendarHeader unchanged = base;
  return !fromLocalTm(bad, unchanged) && equal(base, unchanged);
}

}  // namespace Phase8CalendarPresentation
