#ifndef HEADER_BBC_KEYBOARD
#define HEADER_BBC_KEYBOARD

// explicitly define the keymappings in integers as ascii
//    -1 = go to lower
//    -50 = keypress not sent
//    -98 = copy
//    -99 = shift lock
#define LC -1
#define NP 50
#define KEY_BBC_COPY -98
#define KEY_BBC_SHIFT_LOCK -99

// define frquency
// 0 = 8MHz, 1 = 4MHz,  3 = 2MHz, 7 = 1MHz
const int OCR1A_VAL = 7;

// LED states
// these are reversed as we need to ground the LEDs to turn them on.

const int LED_ON = LOW;
const int LED_OFF = HIGH;

// row BCD to Decimal
// 000, 011, 101, 001, 110, 010, 100, 111
//   0,   1,   2,   3,   4,   5,   6,   7
const int NUM_ROW_STATES = 8;
const int ROW_STATES[NUM_ROW_STATES][3] = {{0, 0, 0}, {0, 1, 1}, {1, 0, 1},
                                           {0, 0, 1}, {1, 1, 0}, {0, 1, 0},
                                           {1, 0, 0}, {1, 1, 1}};

// col BCD to Decimal
// 0000, 0001, 0010, 0011, 0100, 0101, 0110, 0111, 1000, 1001
//    0,    1,    2,    3,    4,    5,    6,    7,    8,    9
const int NUM_COL_STATES = 10;
const int COL_STATES[NUM_COL_STATES][4] = {
    {0, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {0, 0, 1, 1}, {0, 1, 0, 0},
    {0, 1, 0, 1}, {0, 1, 1, 0}, {0, 1, 1, 1}, {1, 0, 0, 0}, {1, 0, 0, 1}};

// lower case
const int LOWER_KB_MATRIX[8][10] = {
    {KEY_LEFT_SHIFT, KEY_LEFT_CTRL, NP, NP, NP, NP, NP, NP, NP, NP},
    {KEY_TAB, 122, 32, 118, 98, 109, 44, 46, 47, KEY_BBC_COPY},
    {KEY_BBC_SHIFT_LOCK, 115, 99, 103, 104, 110, 108, 59, 93, KEY_DELETE},
    {KEY_CAPS_LOCK, 97, 120, 102, 121, 106, 107, 64, 58, KEY_RETURN},
    {49, 50, 100, 114, 54, 117, 111, 112, 91, KEY_UP_ARROW},
    {KEY_F10, 119, 101, 116, 55, 105, 57, 48, 95, KEY_DOWN_ARROW},
    {113, 51, 52, 53, KEY_F4, 56, KEY_F7, 45, 94, KEY_LEFT_ARROW},
    {KEY_ESC, KEY_F1, KEY_F2, KEY_F3, KEY_F5, KEY_F6, KEY_F8, KEY_F9, 92,
     KEY_RIGHT_ARROW}};

// shift case
const int SHIFT_KEYBOARD_MATRIX[8][10] = {
    {LC, LC, NP, NP, NP, NP, NP, NP, NP, NP},
    {LC, LC, LC, LC, LC, LC, 60, 62, 63, LC},
    {LC, LC, LC, LC, LC, LC, LC, 43, 125, LC},
    {LC, LC, LC, LC, LC, LC, LC, LC, 42, LC},
    {33, 34, LC, LC, 38, LC, LC, LC, 123, LC},
    {LC, LC, LC, LC, 39, 41, LC, LC, 96, LC},
    {LC, 35, 36, 37, LC, 40, LC, 61, 126, LC},
    {LC, LC, LC, LC, LC, LC, LC, LC, 124, LC}};

const int CA2_LOW_OFFSET = 100;  // TODO: experimental value
const int W_LOW_OFFSET = 200;    // TODO: experimental value

class _BBCKeyboard {
 private:
  // frequency wave output pin for ATmega32u4 (Arduino Leonardo)
  int _freqPin;
  // Enable KB Scan/Query : used to stop the scanning of the keyboard. Default
  // state High, Low enables querying of the matrix.
  int _kbEnbPin;
  int _rowPins[3];
  int _colPins[4];
  // goes high when the right Row/Col combination is found
  int _wPin;
  // original cassette motor LED, used as key pressed LED
  int _cassetteLEDPin;
  // goes high if there's a keypress
  int _ca2Pin;
  int _shiftLockLEDPin;
  int _capsLockLEDPin;

  // needed for reading the analogue values of the 'key pressed(CA2)' and
  // 'correct guess(W)' pins. The output of these is somewhere around 2v, so not
  // high enough for a digitalRead()

  int _ca2Low;
  int _wLow;
  bool _shiftPressed;
  bool _ctrlPressed;
  bool _shiftLockActive;
  bool _capsLockActive;

  void _init(void);

 public:
  _BBCKeyboard(void);
  void begin(void);
  void begin(int, int, int, int, int, int, int, int, int, int, int, int, int,
             int);
  void loop(void);
  void end(void);
  void reset(void);
  void calibrateCA2(int);
  void calibrateW(int);
  void digitalWritePins(int, int[], const int[]);
  bool isKeyPressed(void);
  int scanKeyAndSend(void);
  bool isKeyFound(int);
  int getKeySignal(int, int);
  void updateKeyboard(int);
  bool getShiftActive(void);
  void toggleShiftLock(void);
  void toggleCapsLock(void);
};

extern _BBCKeyboard BBCKeyboard;

#endif