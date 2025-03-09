#include <Arduino.h>
#include <Keyboard.h>
#include <constant.h>
#include <keymap.h>

int ca2Val;
int wVal;
int ca2Low;
int wLow;

bool shiftPressed;
bool shiftLockActive;
bool capsLockActive;

bool keyPressed();
int findKey();
void digitalWritePins(int, const int[], int[]);
int wHigh();
int sendKey(int, int);
bool shiftActive();
void toggleShiftLock();

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
  }

  // needed for reading the analogue values of the 'key pressed(CA2)' and
  // 'correct guess(W)' pins. The output of these is somewhere around 2v, so not
  // high enough for a digitalRead()
  ca2Val = 0;
  wVal = 0;

  // init states
  shiftPressed = false;
  shiftLockActive = false;
  capsLockActive = false;

  // KB pinout
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
  // pin setup
  pinMode(USB_ENABLE, INPUT_PULLUP);  // Keyboard Mode / Programming Mode
  // keyboard cable
  pinMode(FREQ_OUTPUT_PIN, OUTPUT);  // KB 3
  pinMode(ENABLE_KB_PIN, OUTPUT);    // KB 4
  // analogue input rather than digial, as 'high' does not equate to a digital
  // high, voltage is too low
  pinMode(W_PIN, INPUT);    // KB 12
  pinMode(CA2_PIN, INPUT);  // KB 14
  // setup pin for 3 LEDs
  pinMode(KEYPRESSED_LED, OUTPUT);  // KB13
  pinMode(SHIFTLOCK_LED, OUTPUT);   // KB16
  pinMode(CAPSLOCK_LED, OUTPUT);    // KB17

  // setup the clock.
  // output: square wave (toggle mode)
  // freq: f_clk / (2 * (OCR1A + 1))
  TCCR1A = ((1 << COM1A0));               // COM1 -> Toggle Mode
  TCCR1B = ((1 << WGM12) | (1 << CS10));  // CTC Mode, No Prescaler
  TIMSK1 = 0;                             // Disable Timer 1 Interrupt
  OCR1A = OCR1A_VAL;                      // 16M / (2 * ('7' + 1)) = 1MHz

  // state setup
  // start the kb scanning
  digitalWrite(ENABLE_KB_PIN, HIGH);
  // turn off leds
  digitalWrite(KEYPRESSED_LED, LED_OFF);
  digitalWrite(CAPSLOCK_LED, LED_OFF);
  digitalWrite(SHIFTLOCK_LED, LED_OFF);

  // reset Row Pins, KB 5 to 7
  for (int row = 0; row < 3; row++) {
    pinMode(ROW_PINS[row], OUTPUT);
    digitalWrite(ROW_PINS[row], LOW);
  }

  // reset Column Pins, KB 8 to 11
  for (int col = 0; col < 4; col++) {
    pinMode(COL_PINS[col], OUTPUT);
    digitalWrite(COL_PINS[col], LOW);
  }

  // get correct voltage of analogue input
  ca2Low = analogRead(CA2_PIN) + CA2_LOW_OFFSET;
  wLow = analogRead(W_PIN) + W_LOW_OFFSET;

  // start the USB keyboard emulation
  Keyboard.begin();
  // reset scrolllock, capslock and numlock
  Keyboard.releaseAll();
}

void loop() {
  // Keyboard Mode
  if (digitalRead(USB_ENABLE)) {
    if (keyPressed()) {
      digitalWrite(ENABLE_KB_PIN, LOW);  // Stop scanning the kb
      findKey();
      digitalWrite(ENABLE_KB_PIN, HIGH);  // Restart scanning
      delay(REPEAT_DELAY);
    }
  } else {
    // Programming Mode
    Keyboard.end();
  }
}

bool keyPressed() {
  CA2Val = analogRead(CA2_PIN);

  if (DEBUG) {
    Serial.print("CA2: ");
    Serial.println(CA2Val);
  }

  if (CA2Val <= CA2Low) {
    CA2Low = CA2Val + CA2_LOW_OFFSET;  // Recalibrate CA2 Low
    digitalWrite(KEYPRESSED_LED, LED_OFF);
    return false;
  }

  digitalWrite(KEYPRESSED_LED, LED_ON);
  return true;
}

int findKey() {
  // the way this works is that it polls the row and cols ICs with binary
  // counting, then checks the 'W' pin. if the W_PIN goes high, we've got the
  // right combination of rows and cols, if not then continue to the next
  // row/col combination. The [row,col] index can be mapped to a matrix that has
  // the values for which key that is. we don't break the loop after we've found
  // a keypress, as we need to detect more than one at once.

  // row BCD to Decimal
  // 000, 011, 101, 001, 110, 010, 100, 111
  //   0,   1,   2,   3,   4,   5,   6,   7
  int numRowStates = 8;
  int rowStates[numRowStates][3] = {{0, 0, 0}, {0, 1, 1}, {1, 0, 1}, {0, 0, 1},
                                    {1, 1, 0}, {0, 1, 0}, {1, 0, 0}, {1, 1, 1}};

  // col BCD to Decimal
  // 0000, 0001, 0010, 0011, 0100, 0101, 0110, 0111, 1000, 1001
  //    0,    1,    2,    3,    4,    5,    6,    7,    8,    9
  int numColStates = 10;
  int colStates[numColStates][4] = {
      {0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {0, 0, 1, 1}, {0, 1, 0, 0},
      {0, 1, 0, 1}, {0, 1, 1, 0}, {0, 1, 1, 1}, {1, 0, 0, 0}, {1, 0, 0, 1}};

  for (int thisRowState = 0; thisRowState < numRowStates; thisRowState++) {
    digitalWritePins(3, ROW_PINS, rowStates[thisRowState]);
    for (int thisColState = 0; thisColState < numColStates; thisColState++) {
      digitalWritePins(4, COL_PINS, colStates[thisColState]);
      if (wHigh()) {
        if (DEBUG) {
          Serial.print("Row: ");
          Serial.print(thisRowState);
          Serial.print("| Col: ");
          Serial.println(thisColState);
        }
        sendKey(thisRowState, thisColState);
      }
    }
  }

  // reset the output pin state back to 0s.
  digitalWritePins(3, ROW_PINS, rowStates[0]);
  digitalWritePins(4, COL_PINS, colStates[0]);
  return -1;
}

void digitalWritePins(int numPins, const int pinNumbers[], int states[]) {
  // takes an array of pins and pin states to bulk write to them
  // could use pin groups and binary mappings, but this seems more readable
  for (int thisPin = 0; thisPin < numPins; thisPin++) {
    // reset all the pins to be low every time
    digitalWrite(pinNumbers[thisPin], LOW);
    if (states[thisPin] > 0) {
      digitalWrite(pinNumbers[thisPin], HIGH);
    }
  }
}

int wHigh() {
  WVal = analogRead(W_PIN);
  if (WVal > WLow) {
    return true;
  } else {
    WLow = WVal + W_LOW_OFFSET;  // Recalibrate W Low
    return false;
  }
}

int sendKey(int row, int col) {
  int key = -1;
  if (shiftActive()) {
    // try and get the key from the shift matrix, if this is -1, then it
    // defaults to the normal kb matrix. this negates the need to rewrite both
    // matricies to remap the kb.
    key = shiftKeyboardMatrix[row][col];
  }

  if (capsLockActive | key == -2) {
    key = upperKeyboardMatrix[row][col];
  }

  if (key == -1) {
    key = lowerKeyboardMatrix[row][col];
  }

  if (key == -99) {
    toggleShiftLock();
  }

  if (key >= 0) {
    if (DEBUG) {
      Serial.print(key);
      Serial.print(" | ");
      Serial.println(char(key));
    }
    Keyboard.write(char(key));
  }
}

bool shiftActive() {
  // two different ways of shift being active on this board, shiftlock or
  // holding shift.
  if (shiftPressed || shiftLockActive) {
    return true;
  } else {
    return false;
  }
}

void toggleShiftLock() {
  if (shiftLockActive) {
    shiftLockActive = false;
    digitalWrite(SHIFTLOCK_LED, LED_OFF);
  } else {
    shiftLockActive = true;
    digitalWrite(SHIFTLOCK_LED, LED_ON);
  }
}