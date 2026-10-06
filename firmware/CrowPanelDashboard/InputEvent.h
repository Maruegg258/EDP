#pragma once

#include <Arduino.h>

// Application-level input vocabulary.
// Hardware drivers produce these events; application/navigation code consumes them.
enum class InputEvent : uint8_t {
  NONE,
  MENU,
  EXIT,
  UP,
  DOWN
};
