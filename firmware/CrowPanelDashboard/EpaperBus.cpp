#include "EpaperBus.h"

void EpaperBus::begin() {
  pinMode(PIN_SCK, OUTPUT);
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_DC, OUTPUT);
  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_BUSY, INPUT);

  digitalWrite(PIN_CS, HIGH);
  digitalWrite(PIN_SCK, HIGH);
  digitalWrite(PIN_RESET, HIGH);
}

void EpaperBus::reset() {
  delay(10);
  digitalWrite(PIN_RESET, LOW);
  delay(10);
  digitalWrite(PIN_RESET, HIGH);
  delay(10);
  waitUntilIdle();
}

void EpaperBus::waitUntilIdle() {
  while (digitalRead(PIN_BUSY) != LOW) {
    delay(1);
  }
}

void EpaperBus::writeCommand(uint8_t command) {
  digitalWrite(PIN_DC, LOW);
  writeByte(command);
  digitalWrite(PIN_DC, HIGH);
}

void EpaperBus::writeData(uint8_t data) {
  digitalWrite(PIN_DC, HIGH);
  writeByte(data);
}

void EpaperBus::writeByte(uint8_t value) {
  digitalWrite(PIN_CS, LOW);

  for (uint8_t bit = 0; bit < 8; ++bit) {
    digitalWrite(PIN_SCK, LOW);
    digitalWrite(PIN_MOSI, (value & 0x80) ? HIGH : LOW);
    digitalWrite(PIN_SCK, HIGH);
    value <<= 1;
  }

  digitalWrite(PIN_CS, HIGH);
}
