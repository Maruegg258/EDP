#pragma once

// Application-only result types live in a header so Arduino's auto-generated
// .ino function declarations can see them before any function prototype.
enum class UpdateResult {
  FAILED,
  SKIPPED,
  REFRESHED
};

enum class WeatherStageResult {
  FAILED,
  UNCHANGED,
  CHANGED
};
