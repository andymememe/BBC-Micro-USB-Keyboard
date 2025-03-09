#include <Arduino.h>
#include <Keyboard.h>
#include <bbc_keyboard.hpp>
#include <constant.hpp>

void setup() {
  if (DEBUG) {
    Serial.begin(9600);
  }

  // pin setup
  pinMode(USB_ENABLE, INPUT_PULLUP);  // Keyboard Mode / Programming Mode

  // get correct voltage of analogue input
  BBCKeyboard.begin();
}

void loop() {
  // Keyboard Mode
  if (digitalRead(USB_ENABLE)) {
    BBCKeyboard.loop();
    delay(REPEAT_DELAY);
  } else {
    // Programming Mode
    BBCKeyboard.end();
  }
}