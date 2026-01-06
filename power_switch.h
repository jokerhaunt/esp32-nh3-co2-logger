#pragma once
#include <Arduino.h>
#include "config.h"

class PowerSwitch {
public:
  explicit PowerSwitch(int pin, bool activeHigh = true)
    : _pin(pin), _activeHigh(activeHigh) {}

  void begin() {
    if (_pin < 0) return;
    pinMode(_pin, OUTPUT);
    off();
  }

  void on()  { write(true); }
  void off() { write(false); }

  bool isUsed() const { return _pin >= 0; }

private:
  int _pin;
  bool _activeHigh;

  void write(bool en) {
    if (_pin < 0) return;
    digitalWrite(_pin, (en ^ !_activeHigh) ? HIGH : LOW);
  }
};
