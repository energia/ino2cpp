#include <Arduino.h>
#line 1 "/Users/robertinant/Documents/Arduino/ButtonEvent/ButtonEvent.ino"
/*
 * This Sketch demononstrates a button library that uses TI-RTOS's event feature.
 * The button library has 3 functions.
 * - begin() will setup the button library.
 * - read() which will read and debounce the button.
 * - waitForPress() which will wait for a button to be pressed.
 * 
 * This library will allows you to read a button in one task and have another task
 * wait for the button to be pressed. The task waiting for the button to be pressed
 * will sleep until a button press event is detected. 
 *
 * In this example Sketch, the ButtonEvent tab is the task reading the button and the Serial task
 * is the task waiting for the button to be presses.
 * 
 * Usage:
 * Upload the Sketch and open the Serial monitor. Press button PUSH1 (S1) and observe 
 */
 
#include "Button.h"

Button button(PUSH1);

// the setup routine runs once when you press reset:
#line 24 "/Users/robertinant/Documents/Arduino/ButtonEvent/ButtonEvent.ino"
void ButtonSetup();
#line 29 "/Users/robertinant/Documents/Arduino/ButtonEvent/ButtonEvent.ino"
void ButtonLoop();
#line 2 "/Users/robertinant/Documents/Arduino/ButtonEvent/Serial.ino"
void serialSetup();
#line 11 "/Users/robertinant/Documents/Arduino/ButtonEvent/Serial.ino"
void serialLoop();
#line 24 "/Users/robertinant/Documents/Arduino/ButtonEvent/ButtonEvent.ino"
void ButtonSetup() {  
  button.begin();
}

// the loop routine runs over and over again forever:
void ButtonLoop() {
  button.read();
}

#line 1 "/Users/robertinant/Documents/Arduino/ButtonEvent/Serial.ino"

void serialSetup()
{
  pinMode(RED_LED, OUTPUT);
  Serial.begin(115200);
  delay(500);
  Serial.println("Press button PUSH1!");
}


void serialLoop()
{
  static uint8_t state = 1;

  button.waitForPress();
  Serial.println("Button was pressed!");

  digitalWrite(RED_LED, state);
  state = !state;
}

