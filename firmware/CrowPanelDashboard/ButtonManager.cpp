#include "ButtonManager.h"

bool ButtonManager::readPressed(uint8_t pin) {
  return digitalRead(pin) == LOW;
}

void ButtonManager::initializeInput(
    InputState& input,
    uint32_t nowMs) {
  const bool pressed = readPressed(input.pin);

  input.rawPressed = pressed;
  input.stablePressed = pressed;
  input.pressSeen = pressed;
  input.lastRawChangeMs = nowMs;
}

void ButtonManager::begin() {
  // The CrowPanel board provides pull-up resistors on these inputs.
  // Match the Elecrow reference example and treat LOW as active.
  pinMode(PIN_MENU, INPUT);
  pinMode(PIN_EXIT, INPUT);
  pinMode(PIN_ROTARY_UP, INPUT);
  pinMode(PIN_ROTARY_DOWN, INPUT);

  // GPIO5 is the rotary push switch. It is intentionally not used in
  // Phase 7A-1 because its mechanical reliability has not been accepted
  // for dashboard navigation.
  pinMode(PIN_ROTARY_CONFIRM, INPUT);

  const uint32_t nowMs = millis();

  initializeInput(_menu, nowMs);
  initializeInput(_exit, nowMs);
  initializeInput(_rotaryUp, nowMs);
  initializeInput(_rotaryDown, nowMs);
}

ButtonEvent ButtonManager::updateInput(
    InputState& input,
    uint32_t nowMs) {
  const bool pressed = readPressed(input.pin);

  if (pressed != input.rawPressed) {
    input.rawPressed = pressed;
    input.lastRawChangeMs = nowMs;
  }

  if (pressed == input.stablePressed ||
      static_cast<uint32_t>(nowMs - input.lastRawChangeMs) <
          DEBOUNCE_MS) {
    return ButtonEvent::NONE;
  }

  input.stablePressed = pressed;

  if (pressed) {
    input.pressSeen = true;
    return ButtonEvent::NONE;
  }

  if (!input.pressSeen) {
    return ButtonEvent::NONE;
  }

  input.pressSeen = false;
  return input.releaseEvent;
}

ButtonEvent ButtonManager::tick() {
  const uint32_t nowMs = millis();

  // Update every input on every pass so debounce state keeps advancing
  // even if more than one transition happens close together.
  const ButtonEvent menuEvent = updateInput(_menu, nowMs);
  const ButtonEvent exitEvent = updateInput(_exit, nowMs);
  const ButtonEvent upEvent = updateInput(_rotaryUp, nowMs);
  const ButtonEvent downEvent = updateInput(_rotaryDown, nowMs);

  if (menuEvent != ButtonEvent::NONE) {
    return menuEvent;
  }

  if (exitEvent != ButtonEvent::NONE) {
    return exitEvent;
  }

  if (upEvent != ButtonEvent::NONE) {
    return upEvent;
  }

  return downEvent;
}
