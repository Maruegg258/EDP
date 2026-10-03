#include "TimeService.h"

#include <time.h>

namespace {

// Reject an unset/default system clock without tying the service to the
// current calendar year. 2024-01-01 00:00:00 UTC.
constexpr time_t MIN_VALID_EPOCH = 1704067200;

}  // namespace

TimeService::TimeService()
    : _started(false),
      _state(TimeSyncState::NOT_STARTED) {
}

bool TimeService::begin(const char* timezone,
                        const char* primaryServer,
                        const char* secondaryServer) {
  if (timezone == nullptr || timezone[0] == '\0' ||
      primaryServer == nullptr || primaryServer[0] == '\0') {
    return false;
  }

  configTzTime(timezone, primaryServer, secondaryServer);

  _started = true;
  _state = TimeSyncState::WAITING_FOR_SYNC;
  tick();
  return true;
}

void TimeService::tick() {
  if (!_started) {
    return;
  }

  if (isPlausibleEpoch(time(nullptr))) {
    _state = TimeSyncState::SYNCHRONIZED;
  } else {
    _state = TimeSyncState::WAITING_FOR_SYNC;
  }
}

bool TimeService::started() const {
  return _started;
}

TimeSyncState TimeService::state() const {
  return _state;
}

bool TimeService::isSynchronized() const {
  return _state == TimeSyncState::SYNCHRONIZED;
}

time_t TimeService::epoch() const {
  return time(nullptr);
}

bool TimeService::getLocalTime(struct tm& out) const {
  const time_t now = time(nullptr);

  if (!isPlausibleEpoch(now)) {
    return false;
  }

  return localtime_r(&now, &out) != nullptr;
}

bool TimeService::formatLocalTime(char* destination,
                                  size_t capacity,
                                  const char* format) const {
  if (destination == nullptr || capacity == 0 ||
      format == nullptr || format[0] == '\0') {
    return false;
  }

  struct tm localTime;

  if (!getLocalTime(localTime)) {
    destination[0] = '\0';
    return false;
  }

  const size_t written = strftime(
      destination,
      capacity,
      format,
      &localTime
  );

  if (written == 0) {
    destination[0] = '\0';
    return false;
  }

  return true;
}

bool TimeService::isPlausibleEpoch(time_t value) {
  return value >= MIN_VALID_EPOCH;
}
