#include <Keyboard.h>

// define all the pins
const int FREQ_OUTPUT_PIN =
    9;                      // OC1A output pin for ATmega32u4 (Arduino Micro)
const int OCR1A_VAL = 7;    // 0 = 8MHz, 1 = 4MHz,  3 = 2MHz, 7 = 1MHz
const int CA2_PIN = A10;    // Goes high if there's a keypress
const int USB_ENABLE = A0;  // To switch the USB KB functionality of the machine
                            // off, put this pin to ground.
const int W_PIN = A1;  // Goes high when the right Row/Col combination is found
const int ENABLE_KB_PIN =
    A2;  // Enable KB Scan/Query : used to stop the scanning of the keyboard.
         // Default state High, Low enables querying of the matrix.
const int BREAK_PIN = A3;
const int COL_PINS[4] = {5, 4, 3, 2};
const int ROW_PINS[3] = {6, 7, 8};
const int SHIFTLOCK_LED = 15;
const int CAPSLOCK_LED = 14;
const int KEYPRESSED_LED = 16;

// LED states
// These are reversed as we need to ground the LEDs to turn them on.
const int LED_ON = LOW;
const int LED_OFF = HIGH;

const int REPEAT_DELAY = 100;    // Defined in ms
const int CA2_LOW_OFFSET = 100;  // TODO: experimental value
const int W_LOW_OFFSET = 200;    // TODO: experimental value

const bool debug = true;

// needed for reading the analogue values of the 'key pressed(CA2)' and 'correct
// guess(W)' pins. The output of these is somewhere around 2v, so not high
// enough for a digitalRead()
int CA2Val = 0;
int WVal = 0;
int CA2Low = 0;
int WLow = 0;

bool shiftPressed = false;
bool shiftLockActive = false;
bool capsLockActive = false;

// char keyboardMatrix [8][10] = {
//   {"SHFT"},{"CTRL"},{"*"},{"*"},{"*"},{"*"},{"*"},{"*"},{"*"},{"*"},
//   {"TAB"},{"Z"},{"SPACE"},{"V"},{"B"},{"M"},{","},{"."},{"/"},{"COPY"},
//   {"SHFTLK"},{"S"},{"C"},{"G"},{"H"},{"N"},{"L"},{";"},{"]"},{"DEL"},
//   {"CAPSLK"},{"A"},{"X"},{"F"},{"Y"},{"J"},{"K"},{"@"},{":"},{"RTN"},
//   {"1"},{"2"},{"D"},{"R"},{"6"},{"U"},{"O"},{"P"},{"["},{"UP"},
//   {"F0"},{"W"},{"E"},{"T"},{"7"},{"9"},{"I"},{"0"},{"£"},{"DWN"},
//   {"Q"},{"3"},{"4"},{"5"},{"F4"},{"8"},{"F7"},{"="},{"~"},{"LFT"},
//   {"ESC"},{"F1"},{"F2"},{"F3"},{"F5"},{"F6"},{"F8"},{"F9"},{"\\"},{"RHT"}
// };

// explicitly define the keymappings in integers as ascii
//    -1 = go to lower
//    -2 = go to upper
//    -99 = shift lock
//    any other negative = keypress not sent
int lowerKeyboardMatrix[8][10] = {
    {129, 128, -50, -50, -50, -50, -50, -50, -50, -50},
    {179, 122, 32, 118, 98, 109, 44, 46, 47, -50},
    {-99, 115, 99, 103, 104, 110, 108, 59, 93, 178},
    {193, 97, 120, 102, 121, 106, 107, 34, 58, 176},
    {49, 50, 100, 114, 54, 117, 111, 112, 91, 218},
    {203, 119, 101, 116, 55, 105, 57, 48, 95, 217},
    {113, 51, 52, 53, 197, 56, 200, 45, 94, 216},
    {27, 194, 195, 196, 198, 199, 201, 202, 92, 215}};

int upperKeyboardMatrix[8][10] = {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                  {-1, 90, -1, 86, 66, 77, -1, -1, -1, -1},
                                  {-1, 83, 67, 71, 72, 78, 76, -1, -1, -1},
                                  {-1, 65, 88, 70, 89, 74, 75, -1, -1, -1},
                                  {-1, -1, 68, 82, -1, 85, 79, 80, -1, -1},
                                  {-1, 87, 69, 84, -1, 73, -1, -1, -1, -1},
                                  {81, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                                  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};

int shiftKeyboardMatrix[8][10] = {{-2, -2, -2, -2, -2, -2, -2, -2, -2, -2},
                                  {-2, -2, -2, -2, -2, -2, 60, 62, 63, -2},
                                  {-2, -2, -2, -2, -2, -2, -2, 43, 125, -2},
                                  {-2, -2, -2, -2, -2, -2, -2, -2, 42, -2},
                                  {33, 64, -2, -2, 38, -2, -2, -2, 123, -2},
                                  {-2, -2, -2, -2, 39, -2, 41, -2, 35, -2},
                                  {-2, 92, 36, 37, -2, 40, -2, 61, 124, -2},
                                  {-2, -2, -2, -2, -2, -2, -2, -2, 126, -2}};

void setup() {
  if (debug) {
    Serial.begin(9600);
  }
  pinMode(FREQ_OUTPUT_PIN, OUTPUT);
  // setup the clock.
  // Output: Square Wave (Toggle)
  // Freq: f_clk / (2 * (OCR1A + 1))
  TCCR1A = ((1 << COM1A0));               // COM1 -> Toggle Mode
  TCCR1B = ((1 << WGM12) | (1 << CS10));  // CTC Mode, No Prescaler
  TIMSK1 = 0;                             // Disable Timer 1 Interrupt
  OCR1A = OCR1A_VAL;                      // 16M / (2 * ('7' + 1)) = 1MHz

  pinMode(USB_ENABLE, INPUT_PULLUP);

  // start the kb scanning
  pinMode(ENABLE_KB_PIN, OUTPUT);
  digitalWrite(ENABLE_KB_PIN, HIGH);

  // analogue input rather than digial, as 'high' does not equate to a digital
  // high, voltage is too low
  pinMode(CA2_PIN, INPUT);
  pinMode(W_PIN, INPUT);
  pinMode(BREAK_PIN, INPUT);

  // setup pin for 3 LEDs
  pinMode(KEYPRESSED_LED, OUTPUT);
  pinMode(CAPSLOCK_LED, OUTPUT);
  pinMode(SHIFTLOCK_LED, OUTPUT);
  digitalWrite(KEYPRESSED_LED, LED_OFF);
  digitalWrite(CAPSLOCK_LED, LED_OFF);
  digitalWrite(SHIFTLOCK_LED, LED_OFF);

  // reset Column Pins
  for (int col = 0; col < 4; col++) {
    pinMode(COL_PINS[col], OUTPUT);
    digitalWrite(COL_PINS[col], LOW);
  }

  // reset Row Pins
  for (int row = 0; row < 3; row++) {
    pinMode(ROW_PINS[row], OUTPUT);
    digitalWrite(ROW_PINS[row], LOW);
  }

  // get correct voltage of analogue input
  CA2Low = analogRead(CA2_PIN) + CA2_LOW_OFFSET;
  WLow = analogRead(W_PIN) + W_LOW_OFFSET;

  // start the USB keyboard emulation
  Keyboard.begin();
  // reset scrolllock, capslock and numlock
  Keyboard.releaseAll();
}

void loop() {
  // Keyboard Mode
  if (digitalRead(USB_ENABLE)) {
    if (keyPressed()) {
      digitalWrite(ENABLE_KB_PIN, LOW);  // stop scanning the kb
      findKey();
      digitalWrite(ENABLE_KB_PIN, HIGH);  // restart scanning
      delay(REPEAT_DELAY);
    }
  } else {
    // Programming Mode
    Keyboard.end();
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
    if (debug) {
      Serial.print(key);
      Serial.print(" | ");
      Serial.println(char(key));
    }
    Keyboard.write(char(key));
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

bool keyPressed() {
  CA2Val = analogRead(CA2_PIN);
  if (debug) {
    Serial.print("CA2: ");
    Serial.println(CA2Val);
  }
  if (CA2Val <= CA2Low) {
    CA2Low = CA2Val + CA2_LOW_OFFSET;  // Recalibrate CA2 Low
    digitalWrite(KEYPRESSED_LED, LED_OFF);
    return false;
  } else {
    digitalWrite(KEYPRESSED_LED, LED_ON);
    return true;
  }
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

int findKey() {
  // the way this works is that it polls the row and cols ICs with binary
  // counting, then checks the 'W' pin. if the W_PIN goes high, we've got the
  // right combination of rows and cols, if not then continue to the next
  // row/col combination. The [row,col] index can be mapped to a matrix that has
  // the values for which key that is. we don't break the loop after we've found
  // a keypress, as we need to detect more than one at once.

  // http://www.skot9000.com/ttl/datasheets/251.pdf
  // row order is not the same as the diagram layout
  // 000, 011, 101, 001, 110, 010, 100, 111
  //   0,   1,   2,   3,   4,   5,   6,   7
  int numRowStates = 8;
  int rowStates[numRowStates][3] = {{0, 0, 0}, {0, 1, 1}, {1, 0, 1}, {0, 0, 1},
                                    {1, 1, 0}, {0, 1, 0}, {1, 0, 0}, {1, 1, 1}};

  // http://www.ti.com/lit/ds/symlink/sn7445.pdf
  // col layout is the same as the diagram layout
  // summary:
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
        if (debug) {
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

int wHigh() {
  WVal = analogRead(W_PIN);
  if (WVal > WLow) {
    return true;
  } else {
    WLow = WVal + W_LOW_OFFSET;  // Recalibrate W Low
    return false;
  }
}
