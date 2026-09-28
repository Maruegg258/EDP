#pragma once

#include <Arduino.h>

class EpaperBus {
public:
  static constexpr uint8_t PIN_SCK   = 12;
  static constexpr uint8_t PIN_MOSI  = 11;
  static constexpr uint8_t PIN_RESET = 47;
  static constexpr uint8_t PIN_DC    = 46;
  static constexpr uint8_t PIN_CS    = 45;
  static constexpr uint8_t PIN_BUSY  = 48;

  void begin();
  void reset();
  void waitUntilIdle();

  void writeCommand(uint8_t command);
  void writeData(uint8_t data);

private:
  void writeByte(uint8_t value);
};
