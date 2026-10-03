#pragma once

#include <Arduino.h>
#include <stddef.h>
#include <time.h>

enum class TimeSyncState : uint8_t {
  NOT_STARTED,
  WAITING_FOR_SYNC,
  SYNCHRONIZED
};

class TimeService {
public:
  TimeService();

  bool begin(const char* timezone,
             const char* primaryServer,
             const char* secondaryServer = nullptr);

  void tick();

  bool started() const;
  TimeSyncState state() const;
  bool isSynchronized() const;

  time_t epoch() const;
  bool getLocalTime(struct tm& out) const;
  bool formatLocalTime(char* destination,
                       size_t capacity,
                       const char* format) const;

private:
  static bool isPlausibleEpoch(time_t value);

  bool _started;
  TimeSyncState _state;
};
