#pragma once

#include <Arduino.h>

#include "InputEvent.h"

class ButtonManager {
public:
  // EDP application mapping. These names describe dashboard semantics,
  // not the labels used by the Elecrow reference example.
  static constexpr uint8_t PIN_MENU = 1;
  static constexpr uint8_t PIN_UP = 4;
  static constexpr uint8_t PIN_DOWN = 6;
  static constexpr uint8_t PIN_EXIT = 2;
  static constexpr uint8_t PIN_ROTARY_CONFIRM = 5;

  void begin();
  InputEvent tick();

private:
  struct InputState {
    uint8_t pin;
    InputEvent releaseEvent;
    bool rawPressed;
    bool stablePressed;
    bool pressSeen;
    uint32_t lastRawChangeMs;
  };

  static constexpr uint32_t DEBOUNCE_MS = 30;

  InputState _menu{
    PIN_MENU,
    InputEvent::MENU,
    false,
    false,
    false,
    0
  };

  InputState _exit{
    PIN_EXIT,
    InputEvent::EXIT,
    false,
    false,
    false,
    0
  };

  InputState _up{
    PIN_UP,
    InputEvent::UP,
    false,
    false,
    false,
    0
  };

  InputState _down{
    PIN_DOWN,
    InputEvent::DOWN,
    false,
    false,
    false,
    0
  };

  static bool readPressed(uint8_t pin);
  static void initializeInput(InputState& input, uint32_t nowMs);
  static InputEvent updateInput(InputState& input, uint32_t nowMs);
};
