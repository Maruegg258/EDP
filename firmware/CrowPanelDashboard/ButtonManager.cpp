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
  pinMode(PIN_UP, INPUT);
  pinMode(PIN_DOWN, INPUT);

  // GPIO5 is the rotary push switch. It is intentionally not used in
  // Phase 7 because its mechanical reliability has not been accepted
  // for dashboard navigation.
  pinMode(PIN_ROTARY_CONFIRM, INPUT);

  const uint32_t nowMs = millis();

  initializeInput(_menu, nowMs);
  initializeInput(_exit, nowMs);
  initializeInput(_up, nowMs);
  initializeInput(_down, nowMs);
}

InputEvent ButtonManager::updateInput(
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
    return InputEvent::NONE;
  }

  input.stablePressed = pressed;

  if (pressed) {
    input.pressSeen = true;
    return InputEvent::NONE;
  }

  if (!input.pressSeen) {
    return InputEvent::NONE;
  }

  input.pressSeen = false;
  return input.releaseEvent;
}

InputEvent ButtonManager::tick() {
  const uint32_t nowMs = millis();

  // Update every input on every pass so debounce state keeps advancing
  // even if more than one transition happens close together.
  const InputEvent menuEvent = updateInput(_menu, nowMs);
  const InputEvent exitEvent = updateInput(_exit, nowMs);
  const InputEvent upEvent = updateInput(_up, nowMs);
  const InputEvent downEvent = updateInput(_down, nowMs);

  if (menuEvent != InputEvent::NONE) {
    return menuEvent;
  }

  if (exitEvent != InputEvent::NONE) {
    return exitEvent;
  }

  if (upEvent != InputEvent::NONE) {
    return upEvent;
  }

  return downEvent;
}
