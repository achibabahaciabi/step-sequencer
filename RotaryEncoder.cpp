#include "RotaryEncoder.h"

RotaryEncoder::RotaryEncoder(uint8_t clkPin, uint8_t dtPin, uint8_t swPin) :
  _clkPin(clkPin), _dtPin(dtPin), _swPin(swPin), _delta(0),
  _lastClk(false), _buttonState(false), _lastButtonState(true),
  _lastDebounceTime(0), _buttonPressedFlag(false) {}

void RotaryEncoder::begin() {
  pinMode(_clkPin, INPUT_PULLUP);
  pinMode(_dtPin, INPUT_PULLUP);
  pinMode(_swPin, INPUT_PULLUP);

  _lastClk = digitalRead(_clkPin);
  _lastButtonState = digitalRead(_swPin);
}

void RotaryEncoder::loop() {
  // Handle rotation
  bool clk = digitalRead(_clkPin);
  bool dt = digitalRead(_dtPin);

  if (clk != _lastClk) {
    if (clk == HIGH) {
      // Determine direction
      _delta += (dt != clk) ? 1 : -1;
    }
  }
  _lastClk = clk;

  // Handle button with debounce
  bool reading = digitalRead(_swPin);

  if (reading != _lastButtonState) {
    _lastDebounceTime = millis();
  }

  if ((millis() - _lastDebounceTime) > _debounceDelay) {
    if (reading != _buttonState) {
      _buttonState = reading;

      // Button pressed (active low)
      if (_buttonState == LOW) {
        _buttonPressedFlag = true;
      }
    }
  }

  _lastButtonState = reading;
}

int RotaryEncoder::getDelta() {
  int val = _delta;
  _delta = 0;
  return val;
}

bool RotaryEncoder::wasButtonPressed() {
  if (_buttonPressedFlag) {
    _buttonPressedFlag = false;
    return true;
  }
  return false;
}

