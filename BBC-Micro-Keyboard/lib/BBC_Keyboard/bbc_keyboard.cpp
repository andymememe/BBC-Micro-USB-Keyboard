#include <Arduino.h>
#include <Keyboard.h>

#include <bbc_keyboard.hpp>

_BBCKeyboard::_BBCKeyboard(void) {
  // Default KB pinout
  // KB 1: GND
  // KB 2: RST
  // KB 3: 1 MHz, 9
  // KB 4: KB Enable, 10
  // KB 5 to 7: ROW, 2 to 4
  // KB 8 to 11: COL, 5 to 8
  // KB 12: W, A1
  // KB 13: Casstte LED -> Keypressed LED, 11
  // KB 14: CA2, A2
  // KB 15: 5V
  // KB 16: Shift Lock LED, 12
  // KB 17: Caps Lock LED, 13

  _freqPin = 9;
  _kbEnbPin = 10;
  _rowPins[0] = 4;
  _rowPins[1] = 3;
  _rowPins[2] = 4;
  _colPins[0] = 8;
  _colPins[1] = 7;
  _colPins[2] = 6;
  _colPins[3] = 5;
  _wPin = A1;
  _cassetteLEDPin = 11;
  _ca2Pin = A2;
  _shiftLockLEDPin = 12;
  _capsLockLEDPin = 13;
  _ca2Low = 0;
  _wLow = 0;
  _shiftPressed = false;
  _ctrlPressed = false;
  _shiftLockActive = false;
  _capsLockActive = false;
}

void _BBCKeyboard::_init(void) {
  // keyboard cable
  pinMode(_freqPin, OUTPUT);   // KB 3
  pinMode(_kbEnbPin, OUTPUT);  // KB 4
  // analogue input rather than digial, as 'high' does not equate to a digital
  // high, voltage is too low
  pinMode(_wPin, INPUT);    // KB 12
  pinMode(_ca2Pin, INPUT);  // KB 14
  // setup pin for 3 LEDs
  pinMode(_cassetteLEDPin, OUTPUT);   // KB13
  pinMode(_shiftLockLEDPin, OUTPUT);  // KB16
  pinMode(_capsLockLEDPin, OUTPUT);   // KB17

  // state setup
  // start the kb scanning
  digitalWrite(_kbEnbPin, HIGH);
  // turn off leds
  digitalWrite(_cassetteLEDPin, LED_OFF);
  digitalWrite(_capsLockLEDPin, LED_OFF);
  digitalWrite(_shiftLockLEDPin, LED_OFF);

  // reset Row Pins, KB 5 to 7
  for (int row = 0; row < 3; row++) {
    pinMode(_rowPins[row], OUTPUT);
    digitalWrite(_rowPins[row], LOW);
  }

  // reset Column Pins, KB 8 to 11
  for (int col = 0; col < 4; col++) {
    pinMode(_colPins[col], OUTPUT);
    digitalWrite(_colPins[col], LOW);
  }

  calibrateCA2(analogRead(_ca2Pin));
  calibrateW(analogRead(_wPin));

  // start the USB keyboard emulation
  Keyboard.begin();
  // reset scrolllock, capslock and numlock
  Keyboard.releaseAll();
}

void _BBCKeyboard::begin() {
  _init();

  // setup the clock.
  // output: square wave (toggle mode)
  // freq: f_clk / (2 * (OCR1A + 1))
  TCCR1A = ((1 << COM1A0));               // COM1 -> Toggle Mode
  TCCR1B = ((1 << WGM12) | (1 << CS10));  // CTC Mode, No Prescaler
  TIMSK1 = 0;                             // Disable Timer 1 Interrupt
  OCR1A = OCR1A_VAL;                      // 16M / (2 * ('7' + 1)) = 1MHz
}

void _BBCKeyboard::begin(int freqPin, int kbEnbPin, int wPin,
                         int r0, int r1, int r2,
                         int c0, int c1, int c2, int c3,
                         int cassetteLEDPin, int ca2Pin, int shiftLockLEDPin,
                         int capsLockLEDPin) {
  _freqPin = freqPin;
  _kbEnbPin = kbEnbPin;
  _rowPins[0] = r0;
  _rowPins[1] = r1;
  _rowPins[2] = r2;
  _colPins[0] = c0;
  _colPins[1] = c1;
  _colPins[2] = c2;
  _colPins[3] = c3;
  _wPin = wPin;
  _cassetteLEDPin = cassetteLEDPin;
  _ca2Pin = ca2Pin;
  _shiftLockLEDPin = shiftLockLEDPin;
  _capsLockLEDPin = capsLockLEDPin;
  
  _init();

  // setup the clock.
  // output: square wave (toggle mode)
  // freq: f_clk / (2 * (OCR1A + 1))
  TCCR1A = ((1 << COM1A0));               // COM1 -> Toggle Mode
  TCCR1B = ((1 << WGM12) | (1 << CS10));  // CTC Mode, No Prescaler
  TIMSK1 = 0;                             // Disable Timer 1 Interrupt
  OCR1A = OCR1A_VAL;                      // 16M / (2 * ('7' + 1)) = 1MHz
}

void _BBCKeyboard::loop(void) {
  if (isKeyPressed()) {
    digitalWrite(_kbEnbPin, LOW);  // Stop scanning the kb
    scanKeyAndSend();
    digitalWrite(_kbEnbPin, HIGH);  // Restart scanning
  }
}

void _BBCKeyboard::end(void) { Keyboard.end(); }

void _BBCKeyboard::reset(void) {
  _shiftPressed = false;
  _ctrlPressed = false;
}

void _BBCKeyboard::calibrateCA2(int val) { _ca2Low = val + CA2_LOW_OFFSET; }

void _BBCKeyboard::calibrateW(int val) { _wLow = val + W_LOW_OFFSET; }

void _BBCKeyboard::digitalWritePins(int numPins, int pinNumbers[],
                                    const int states[]) {
  // takes an array of pins and pin states to bulk write to them
  // could use pin groups and binary mappings, but this seems more readable
  for (int iPin = 0; iPin < numPins; iPin++) {
    // reset all the pins to be low every time
    digitalWrite(pinNumbers[iPin], LOW);
    if (states[iPin] > 0) {
      digitalWrite(pinNumbers[iPin], HIGH);
    }
  }
}

bool _BBCKeyboard::isKeyPressed(void) {
  int ca2Val = analogRead(_ca2Pin);

  if (ca2Val <= _ca2Low) {
    calibrateCA2(ca2Val);
    digitalWrite(_cassetteLEDPin, LED_OFF);
    return false;
  }

  digitalWrite(_cassetteLEDPin, LED_ON);
  return true;
}

int _BBCKeyboard::scanKeyAndSend(void) {
  // the way this works is that it polls the row and cols ICs with binary
  // counting, then checks the 'W' pin. if the W_PIN goes high, we've got the
  // right combination of rows and cols, if not then continue to the next
  // row/col combination. The [row,col] index can be mapped to a matrix that has
  // the values for which key that is. we don't break the loop after we've found
  // a keypress, as we need to detect more than one at once.

  for (int row = 0; row < NUM_ROW_STATES; row++) {
    digitalWritePins(3, _rowPins, ROW_STATES[row]);
    for (int col = 0; col < NUM_COL_STATES; col++) {
      digitalWritePins(4, _colPins, COL_STATES[col]);
      if (isKeyFound(analogRead(_wPin))) {
        updateKeyboard(getKeySignal(row, col));
      }
    }
  }

  // reset the output pin state back to 0s.
  digitalWritePins(3, _rowPins, ROW_STATES[0]);
  digitalWritePins(4, _colPins, COL_STATES[0]);
  reset();

  return -1;
}

bool _BBCKeyboard::isKeyFound(int val) {
  if (val > _wLow) {
    return true;
  } else {
    calibrateW(val);
    return false;
  }
}

int _BBCKeyboard::getKeySignal(int row, int col) {
  int bbcKey = LC;
  if (getShiftActive()) {
    bbcKey = SHIFT_KEYBOARD_MATRIX[row][col];
  }

  if (bbcKey == LC) {
    bbcKey = LOWER_KB_MATRIX[row][col];
  }

  if (bbcKey >= 97 && bbcKey <= 122) {
    if ((!getShiftActive() && _capsLockActive) ||
        ((getShiftActive() && !_capsLockActive))) {
      bbcKey -= 32;
    }
  }

  if (bbcKey == KEY_BBC_SHIFT_LOCK) {
    toggleShiftLock();
  } else if (bbcKey == KEY_CAPS_LOCK) {
    toggleCapsLock();
    bbcKey = NP;
  } else if (bbcKey == KEY_LEFT_SHIFT) {
    _shiftPressed = true;
  }

  return bbcKey;
}

void _BBCKeyboard::updateKeyboard(int key) {
  if (key >= 0) {
    Keyboard.press(char(key));
  } else if (key == KEY_BBC_COPY) {
    Keyboard.press(KEY_LEFT_CTRL);
    Keyboard.press(99);
    Keyboard.release(KEY_LEFT_CTRL);
    Keyboard.release(99);
  }

  if (_shiftLockActive) {
    digitalWrite(_shiftLockLEDPin, LED_ON);
  } else {
    digitalWrite(_shiftLockLEDPin, LED_OFF);
  }

  if (_capsLockActive) {
    digitalWrite(_capsLockLEDPin, LED_ON);
  } else {
    digitalWrite(_capsLockLEDPin, LED_OFF);
  }
}

bool _BBCKeyboard::getShiftActive(void) {
  // two different ways of shift being active on this board, shiftlock or
  // holding shift.
  return (!_shiftLockActive && _shiftPressed) ||
         (_shiftLockActive && !_shiftPressed);
}

void _BBCKeyboard::toggleShiftLock(void) {
  if (_shiftLockActive) {
    _shiftLockActive = false;
  } else {
    _shiftLockActive = true;
    if (_capsLockActive) {
      _capsLockActive = false;
    }
  }
}

void _BBCKeyboard::toggleCapsLock(void) {
  if (_capsLockActive) {
    _capsLockActive = false;
  } else {
    _capsLockActive = true;
    if (_shiftLockActive) {
      _shiftLockActive = false;
    }
  }
}

_BBCKeyboard BBCKeyboard;