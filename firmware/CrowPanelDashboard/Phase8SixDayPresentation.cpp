#include "Phase8SixDayPresentation.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "WeatherIconSelector.h"

namespace {

int twoDigits(const char* s) {
  return (s[0] - '0') * 10 + (s[1] - '0');
}
int fourDigits(const char* s) {
  return (s[0] - '0') * 1000 + (s[1] - '0') * 100 +
         (s[2] - '0') * 10 + (s[3] - '0');
}
bool digits(const char* s, size_t a, size_t b) {
  for (size_t i=a; i<b; ++i)
    if (s[i] < '0' || s[i] > '9') return false;
  return true;
}
bool leap(int y) {
  return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}
bool parseDate(const char* s, int& y, int& m, int& d) {
  if (s == nullptr || strlen(s) != 10 || s[4] != '-' || s[7] != '-' ||
      !digits(s,0,4) || !digits(s,5,7) || !digits(s,8,10)) return false;
  y=fourDigits(s); m=twoDigits(s+5); d=twoDigits(s+8);
  static const int DAYS[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (y < 2000 || y > 9999 || m < 1 || m > 12) return false;
  const int maxDay = (m==2 && leap(y)) ? 29 : DAYS[m-1];
  return d >= 1 && d <= maxDay;
}
bool nextDate(const char* a, const char* b) {
  int y1,m1,d1,y2,m2,d2;
  if (!parseDate(a,y1,m1,d1)||!parseDate(b,y2,m2,d2)) return false;
  static const int DAYS[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  const int maxDay = (m1==2 && leap(y1)) ? 29 : DAYS[m1-1];
  if (++d1>maxDay) {d1=1; if (++m1>12) {m1=1; ++y1;}}
  return y1==y2 && m1==m2 && d1==d2;
}
bool weekdayName(const char* iso, char* result) {
  int y,m,d;
  if (!parseDate(iso,y,m,d)) return false;
  // Sakamoto Gregorian weekday: Sunday = 0. Local DAILY dates are already
  // in Asia/Taipei; no UTC conversion or time-zone lookup is needed here.
  static const int OFFSETS[]={0,3,2,5,0,3,5,1,4,6,2,4};
  static const char* const DAYS[]={"SUN","MON","TUE","WED","THU","FRI","SAT"};
  if (m<3) --y;
  const int index=(y+y/4-y/100+y/400+OFFSETS[m-1]+d)%7;
  strcpy(result,DAYS[index]);
  return true;
}

bool formatSummary(const WeatherDailyValue& day, char* dest, size_t n) {
  if (dest==nullptr || n==0 ||
      !std::isfinite(day.temperatureMaxC) ||
      !std::isfinite(day.temperatureMinC) ||
      day.temperatureMaxC < day.temperatureMinC ||
      day.temperatureMaxC > 100.f || day.temperatureMaxC < -100.f ||
      day.temperatureMinC > 100.f || day.temperatureMinC < -100.f ||
      day.precipitationProbabilityMax>100) return false;

  const int maxC=static_cast<int>(std::lround(day.temperatureMaxC));
  const int minC=static_cast<int>(std::lround(day.temperatureMinC));
  const int written=snprintf(dest,n,"%d/%dC %u%%",maxC,minC,
      static_cast<unsigned>(day.precipitationProbabilityMax));
  // 14px glyph cell is exactly 9px wide. 12 glyphs => 108px and
  // 10px margins in each 128px column (upper bound: 116px).
  return written>0 && static_cast<size_t>(written)<n &&
         written <= 12;
}

}  // namespace

namespace Phase8SixDayPresentation {

void setUnavailable(Phase8SixDayForecast& out) {
  out=Phase8SixDayForecast{};
}

bool fromSnapshot(const WeatherSnapshot& snapshot,
                  Phase8SixDayForecast& out) {
  if (!snapshot.hasSixDayForecast ||
      strncmp(snapshot.today.date, snapshot.current.time, 10)!=0 ||
      !nextDate(snapshot.today.date, snapshot.futureDays[0].date) ||
      strcmp(snapshot.tomorrow.date, snapshot.futureDays[0].date)!=0) {
    return false;
  }
  Phase8SixDayForecast candidate{};
  for (size_t i=0;i<Phase8SixDayForecast::COUNT;++i) {
    const WeatherDailyValue& input=snapshot.futureDays[i];
    if (i>0 && !nextDate(snapshot.futureDays[i-1].date,input.date)) {
      return false;
    }
    Phase8SixDayColumn& col=candidate.days[i];
    if (!weekdayName(input.date,col.weekday) ||
        !formatSummary(input,col.summary,sizeof(col.summary))) {
      return false;
    }
    col.icon=&WeatherIconSelector::select(input.condition,true);
    if (col.icon->width!=32 || col.icon->height!=32 ||
        col.icon->data==nullptr) return false;
  }
  candidate.available=true;
  out=candidate;
  return true;
}

bool equal(const Phase8SixDayForecast& a,const Phase8SixDayForecast& b) {
  if (a.available!=b.available) return false;
  if (!a.available) return true;
  for (size_t i=0;i<Phase8SixDayForecast::COUNT;++i) {
    if (a.days[i].icon!=b.days[i].icon ||
        strcmp(a.days[i].weekday,b.days[i].weekday)!=0 ||
        strcmp(a.days[i].summary,b.days[i].summary)!=0) return false;
  }
  return true;
}

bool selfCheck() {
  Phase8SixDayForecast blank{};
  setUnavailable(blank);
  if (blank.available) return false;
  WeatherSnapshot src{};
  strcpy(src.current.time,"2026-10-11T12:00");
  strcpy(src.today.date,"2026-10-11");
  strcpy(src.tomorrow.date,"2026-10-12");
  const char* dates[]={"2026-10-12","2026-10-13","2026-10-14",
                       "2026-10-15","2026-10-16","2026-10-17"};
  for (size_t i=0;i<WeatherSnapshot::FUTURE_DAY_COUNT;++i) {
    WeatherDailyValue& day=src.futureDays[i];
    strcpy(day.date,dates[i]);
    day.condition=WeatherCondition::CLEAR;
    day.temperatureMaxC=29.2f;
    day.temperatureMinC=24.8f;
    day.precipitationProbabilityMax=80;
  }
  if (fromSnapshot(src,blank)) return false; // hasSixDayForecast=false
  src.hasSixDayForecast=true;
  Phase8SixDayForecast valid{};
  if (!fromSnapshot(src,valid) || !valid.available ||
      strcmp(valid.days[0].weekday,"MON")!=0 ||
      strcmp(valid.days[5].weekday,"SAT")!=0 ||
      strcmp(valid.days[0].summary,"29/25C 80%")!=0) return false;
  Phase8SixDayForecast backup=valid;
  src.futureDays[2].temperatureMaxC=-101.f;
  if (fromSnapshot(src,valid) || !equal(valid,backup)) return false;
  src.futureDays[2].temperatureMaxC=29.2f;
  strcpy(src.futureDays[2].date,"2026-10-15"); // duplicate/skipped
  if (fromSnapshot(src,valid) || !equal(valid,backup)) return false;
  if (!nextDate("2028-02-28","2028-02-29") ||
      !nextDate("2028-02-29","2028-03-01") ||
      !nextDate("2026-12-31","2027-01-01") ||
      nextDate("2027-02-28","2027-02-29")) return false;
  return true;
}

} // namespace Phase8SixDayPresentation
