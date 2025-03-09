#include <Arduino.h>

// define all the pins
const int USB_ENABLE = A0;  // To switch the USB KB functionality of the machine
                            // off, put this pin to ground.
const int W_PIN = A1;  // Goes high when the right Row/Col combination is found
const int CA2_PIN = A2;  // Goes high if there's a keypress
const int ROW_PINS[3] = {4, 3, 2};
const int COL_PINS[4] = {8, 7, 6, 5};
const int FREQ_OUTPUT_PIN =
    9;                      // OC1A output pin for ATmega32u4 (Arduino Leonardo)
const int ENABLE_KB_PIN =
    10;  // Enable KB Scan/Query : used to stop the scanning of the keyboard.
         // Default state High, Low enables querying of the matrix.
const int KEYPRESSED_LED = 11;
const int SHIFTLOCK_LED = 12;
const int CAPSLOCK_LED = 13;

// define frquency
const int OCR1A_VAL = 7;  // 0 = 8MHz, 1 = 4MHz,  3 = 2MHz, 7 = 1MHz

// LED states
// These are reversed as we need to ground the LEDs to turn them on.
const int LED_ON = LOW;
const int LED_OFF = HIGH;

const int REPEAT_DELAY = 100;    // Defined in ms
const int CA2_LOW_OFFSET = 100;  // TODO: experimental value
const int W_LOW_OFFSET = 200;    // TODO: experimental value

// Debug mode
const bool DEBUG = false;