#ifndef TUS_H
#define TUS_H

#include <Keypad.h>

class Tus {
public:
    static const byte ROWS = 4;
    static const byte COLS = 4;

    char keys[ROWS][COLS];
    byte rowPins[ROWS];
    byte colPins[COLS];
    Keypad keypad;

    Tus();  // constructor

    int mapKeyToStep(char key);
};

#endif






