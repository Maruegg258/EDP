#pragma once

#include <Arduino.h>

enum class ButtonEvent : uint8_t {
  NONE,
  MENU_RELEASED,
  EXIT_RELEASED,
  ROTARY_UP_STEP,
  ROTARY_DOWN_STEP
};

class ButtonManager {
public:
  static constexpr uint8_t PIN_MENU = 2;
  static constexpr uint8_t PIN_EXIT = 1;
  static constexpr uint8_t PIN_ROTARY_UP = 6;
  static constexpr uint8_t PIN_ROTARY_DOWN = 4;
  static constexpr uint8_t PIN_ROTARY_CONFIRM = 5;

  void begin();
  ButtonEvent tick();

private:
  struct InputState {
    uint8_t pin;
    ButtonEvent releaseEvent;
    bool rawPressed;
    bool stablePressed;
    bool pressSeen;
    uint32_t lastRawChangeMs;
  };

  static constexpr uint32_t DEBOUNCE_MS = 30;

  InputState _menu{
    PIN_MENU,
    ButtonEvent::MENU_RELEASED,
    false,
    false,
    false,
    0
  };

  InputState _exit{
    PIN_EXIT,
    ButtonEvent::EXIT_RELEASED,
    false,
    false,
    false,
    0
  };

  InputState _rotaryUp{
    PIN_ROTARY_UP,
    ButtonEvent::ROTARY_UP_STEP,
    false,
    false,
    false,
    0
  };

  InputState _rotaryDown{
    PIN_ROTARY_DOWN,
    ButtonEvent::ROTARY_DOWN_STEP,
    false,
    false,
    false,
    0
  };

  static bool readPressed(uint8_t pin);
  static void initializeInput(InputState& input, uint32_t nowMs);
  static ButtonEvent updateInput(InputState& input, uint32_t nowMs);
};
