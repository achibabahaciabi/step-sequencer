#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <Arduino.h>

class RotaryEncoder {
public:
  RotaryEncoder(uint8_t clkPin = 32, uint8_t dtPin = 33, uint8_t swPin = 25);

  void begin();
  void loop();

  // Returns the incremental change since last call (can be positive or negative)
  int getDelta();

  // Returns true only once per physical button press
  bool wasButtonPressed();

private:
  uint8_t _clkPin;
  uint8_t _dtPin;
  uint8_t _swPin;

  int _delta;
  bool _lastClk;
  
  // Button debouncing variables
  bool _buttonState;
  bool _lastButtonState;
  unsigned long _lastDebounceTime;
  const unsigned long _debounceDelay = 50; // ms
  bool _buttonPressedFlag;
};

#endif

