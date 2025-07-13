#include "Tus.h"

Tus::Tus()
  : keys{
      {'1','2','3','A'},
      {'4','5','6','B'},
      {'7','8','9','C'},
      {'*','0','#','D'}
    },
    rowPins{23, 22, 21, 19},
    colPins{18, 5, 17, 16},
    keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS) // initialize keypad here
{
  // Empty constructor body, everything is initialized in initializer list
}

int Tus::mapKeyToStep(char key) {
  switch (key) {
    case '1': return 0;
    case '2': return 1;
    case '3': return 2;
    case 'A': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case 'B': return 7;
    case '7': return 8;
    case '8': return 9;
    case '9': return 10;
    case 'C': return 11;
    case '*': return 12;
    case '0': return 13;
    case '#': return 14;
    case 'D': return 15;
    default:  return -1;
  }
}

