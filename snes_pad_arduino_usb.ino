/***********************************************************************************

This is free and unencumbered work released into the public domain.

Should the previous statement, for any reason, be judged legally invalid or
ineffective under applicable law, the work is made avaiable under the terms of one
of the following licences:

- Apache License 2.0 https://www.apache.org/licenses/LICENSE-2.0
- MIT License https://opensource.org/licenses/MIT
- CC0 https://creativecommons.org/publicdomain/zero/1.0/legalcode
- Unlicense https://unlicense.org/

The "SNES pad Arduino USB"'s Author, 2020

***********************************************************************************/

// Config -------------------------------------------------------------------------

#define AUTOFIRE_MODE      TOGGLE   // NONE, ASSIST, TOGGLE
#define TAP_MAX_PERIOD     (200000) // us // used in any mode except none
#define AUTOFIRE_PERIOD    (100000) // us // used in any mode except none
#define AUTOFIRE_TAP_COUNT (2)      // #  // used in assit mode
#define AUTOFIRE_SELECTOR  SELECT   // button: SELECT, START, etc // used in toggle mode

#define DEBUG

// Generic routines and macros ---------------------------------------------------------------------

#ifdef DEBUG
#define LOG(C, ...) if (C) do { Serial.print("DEBUG "); Serial.print(__FILE__); Serial.print(":"); Serial.print(__LINE__); Serial.print(" "); char __L[256]; snprintf(__L,255,__VA_ARGS__); __L[255] = '\0'; Serial.print(__L); } while(0)
#else
#define LOG(...)
#endif

//#define BITFLIP(B, N)   ((B) ^= (1<<(N)))
#define BITON(B, N)     ((B) |= (1<<(N)))
#define BITOFF(B, N)    ((B) &= ~(1<<(N)))
#define BITGET(B, N)    (!!((B) & (1<<(N))))
#define BITSET(B, N, V) ((V) ? BITON(B,N) : BITOFF(B,N))

#define CAT2(A, B) A ## B
#define CAT(A, B)  CAT2(A, B)

// Common input-related routines ---------------------------------------------------------------------

#define NUMBER_OF_BUTTONS   12

typedef uint16_t button_state_t;

static unsigned long now_us = 0;
static void next_time_step(void) { now_us = micros(); }
static unsigned long current_time_step(void)  { return now_us; }

static int dpad_angle(int up, int down, int left, int right) {
  if (up) {
    if (left) return 7 * 45;
    else if (right) return 1 * 45;
    else return 0 * 45;
  } else if (down) {
    if (left) return 5 * 45;
    else if (right) return 3 * 45;
    else return 4 * 45;
  } else {
    if (left) return 6 * 45;
    else if (right) return 2 * 45;
    else return -1; // -1 signals that the Hat was released
  }
}

static button_state_t button_debounce(button_state_t button) {
  // No debounce needed ? SNES HW should address it !
  return button;
}

// Autofire ---------------------------------------------------------------------

static int autofire_none(int index, int is_pressed, int option) {
  return is_pressed;
}

#define ASSISTED_BUTTON_NUMBER 4
static int autofire_assist(int index, int is_pressed, int option) {
  if (index < 0 || index >= ASSISTED_BUTTON_NUMBER){
    LOG("autofire not supported for button %d\n", index);
    return is_pressed;
  }
  
  static unsigned long last_press[ ASSISTED_BUTTON_NUMBER ] = {0};
  static int tap_count[ ASSISTED_BUTTON_NUMBER ] = {0};
  static int prev_press[ ASSISTED_BUTTON_NUMBER ] = {0};
  
  // store current status for the next iterations
  unsigned long press_time = last_press[index];
  int was_pressed = prev_press[index];
  prev_press[index] = is_pressed;
  if (is_pressed && !was_pressed) {
    last_press[index] = current_time_step();
  }

  // count the number of taps
  if (is_pressed && !was_pressed) {
    if (current_time_step() < press_time + TAP_MAX_PERIOD ) {
      tap_count[index] += 1;
    }
  }
 
  // reset tap count if too much time is elapsed
  if (!is_pressed && current_time_step() >= press_time + TAP_MAX_PERIOD ) {
    tap_count[index] = 0;
  }
  
  // do autofire 
  if (is_pressed && (tap_count[index] >= AUTOFIRE_TAP_COUNT)) {
    is_pressed = !(((current_time_step() - press_time) / AUTOFIRE_PERIOD ) % 2);
  }
 
  LOG(is_pressed != was_pressed, "auto fire status: button/%d count/%d current/%d timing/%ld result/%d\n", index, tap_count[index], is_pressed, current_time_step() - press_time, is_pressed);

  return is_pressed;
}

#define TOGGLED_BUTTON_NUMBER 4
static int autofire_toggle(int index, int is_pressed, int is_toggled) {
  static unsigned long last_press[ ASSISTED_BUTTON_NUMBER ] = {0};
  static int prev_toggle[ ASSISTED_BUTTON_NUMBER ] = {0};
  static int autofire_enabled[ ASSISTED_BUTTON_NUMBER ] = {0};

  // store current status for the next iterations
  int was_toggled = prev_toggle[index];
  prev_toggle[index] = is_toggled;
  unsigned long press_time = last_press[index];
  int autofire = autofire_enabled[index];

  // store the press time
  if (!is_pressed) {
    last_press[index] = current_time_step(); // + 1;
  }

  // flip the toggle when the toggle-button is press and released while the target-button is pressed
  if (was_toggled && !is_toggled && is_pressed) {
    autofire_enabled[index] = !autofire_enabled[index];
  }

  // do autofire
  if (autofire && is_pressed) {
    is_pressed = !(((current_time_step() - press_time) / AUTOFIRE_PERIOD ) % 2);
  }

  LOG(is_toggled != was_toggled, "auto fire status: button/%d auto/%d current/%d timing/%ld result/%d\n", index, autofire, is_pressed, current_time_step() - press_time, is_pressed);

  return is_pressed;
}

// autofire mode selection
//
#define AUTOFIRE_MODE_NONE 0
#define AUTOFIRE_MODE_ASSIST 1
#define AUTOFIRE_MODE_TOGGLE 2
#define AUTOFIRE_CHOSEN CAT(AUTOFIRE_MODE_, AUTOFIRE_MODE) // e.g. result: AUTOFIRE_MODE_ASSIST
//
#if   AUTOFIRE_CHOSEN == AUTOFIRE_MODE_NONE
#  define DO_AUTOFIRE autofire_none
//
#elif AUTOFIRE_CHOSEN == AUTOFIRE_MODE_ASSIST
#  define DO_AUTOFIRE autofire_assist
//
#elif AUTOFIRE_CHOSEN == AUTOFIRE_MODE_TOGGLE
#  define DO_AUTOFIRE autofire_toggle
//
#else
  #error "unsupported autofire mode"
#endif

// Common usb-related part ---------------------------------------------------------------------

#include "Joystick.h"

Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,JOYSTICK_TYPE_GAMEPAD,
  16, 1,                 // Button Count, Hat Switch Count
  false, false, false,   // no X, Y and Z Axis
  false, false, false,   // No Rx, Ry, or Rz
  false, false,          // No rudder or throttle
  false, false, false);  // No accelerator, brake, or steering

// SNES pad protocol ---------------------------------------------------------------------

//
//   / """""""""""""""""""""""
//  |   O  O  O | O  O  O  O |
//   \ _______________________
//      G un  un  D  L  C  V
//
// un = unused
// G = Ground (0V) provided by the console (1 on PCB)
// D = DATA  set by the pad (2 on PCB)
// L = LATCH set by the console (3 on PCB)
// C = CLOCK set by the console (4 on PCB)
// V = Vcc (5V) provided by the console (5 on PCB)
//
// LATCH  _|""|____________________________________________________________________
// CLOCK  ______|"|_|"|_|"|_|"|_|"|_|"|_|"|_|"|_|"|_|"|_|"|_|"|_|"|_|"|_|"|_|"|____
// DATA   =x====x===x===x===x===x===x===x===x===x===x===x===x===x===x===x===x======
// Button (16x)  B   Y   Sel St  ^   v   <-  ->  A   X   L   R   un  un  un  un
//
// ____|"|_|"|_____
//     <-->
//     12 us
//

#define SNES_LATCH_PIN  7
#define SNES_CLOCK_PIN  8
#define SNES_DATA_PIN   9
  
#define SNES_BUTTON_B        0
#define SNES_BUTTON_Y        1
#define SNES_BUTTON_SELECT   2
#define SNES_BUTTON_START    3
#define SNES_BUTTON_UP       4
#define SNES_BUTTON_DOWN     5
#define SNES_BUTTON_LEFT     6
#define SNES_BUTTON_RIGHT    7
#define SNES_BUTTON_A        8
#define SNES_BUTTON_X        9
#define SNES_BUTTON_L        10
#define SNES_BUTTON_R        11

static void setup_snes(void){

  pinMode(SNES_CLOCK_PIN, OUTPUT);
  digitalWrite(SNES_CLOCK_PIN, HIGH);

  pinMode(SNES_LATCH_PIN, OUTPUT);
  digitalWrite(SNES_LATCH_PIN, LOW);

  pinMode(SNES_DATA_PIN, OUTPUT);
  digitalWrite(SNES_DATA_PIN, HIGH);
  pinMode(SNES_DATA_PIN, INPUT);
}

static button_state_t read_snes(void) {
  button_state_t button = {0};

  digitalWrite(SNES_LATCH_PIN, HIGH);
  delayMicroseconds(12);
  digitalWrite(SNES_LATCH_PIN, LOW);
  delayMicroseconds(6);

  for(int i = 0; i < NUMBER_OF_BUTTONS; i++){
      digitalWrite(SNES_CLOCK_PIN, LOW);
      delayMicroseconds(6);
      BITSET(button, i, !digitalRead(SNES_DATA_PIN));
      digitalWrite(SNES_CLOCK_PIN, HIGH);
      delayMicroseconds(6);
  }

  return button;
}

static int loop_snes(void) {

  button_state_t button = read_snes();

  button = button_debounce(button);

  int opt = BITGET(button, CAT(SNES_BUTTON_, AUTOFIRE_SELECTOR)); // e.g. CAT(...) -> SNES_BUTTON_SELECT
  BITSET(button, SNES_BUTTON_B, DO_AUTOFIRE(0, BITGET(button, SNES_BUTTON_B), opt));
  BITSET(button, SNES_BUTTON_Y, DO_AUTOFIRE(1, BITGET(button, SNES_BUTTON_Y), opt));
  BITSET(button, SNES_BUTTON_A, DO_AUTOFIRE(2, BITGET(button, SNES_BUTTON_A), opt));
  BITSET(button, SNES_BUTTON_X, DO_AUTOFIRE(3, BITGET(button, SNES_BUTTON_X), opt));

  LOG(0, "button state - %04x - B:%x Y:%x S:%x s:%x u:%x d:%x l:%x r:%x A:%x X:%x L:%x R:%x\n",
      button,
      BITGET(button, SNES_BUTTON_B), BITGET(button, SNES_BUTTON_Y),
      BITGET(button, SNES_BUTTON_START), BITGET(button, SNES_BUTTON_SELECT),
      BITGET(button, SNES_BUTTON_UP), BITGET(button, SNES_BUTTON_DOWN),
      BITGET(button, SNES_BUTTON_LEFT), BITGET(button, SNES_BUTTON_RIGHT),
      BITGET(button, SNES_BUTTON_A), BITGET(button, SNES_BUTTON_X),
      BITGET(button, SNES_BUTTON_L), BITGET(button, SNES_BUTTON_R)
   ); // Note: log is bit-ordered

  // leveraging the internal state handling of the Joystick library (no delta needed)
  Joystick.setButton(0, BITGET(button, SNES_BUTTON_A));
  Joystick.setButton(1, BITGET(button, SNES_BUTTON_B));
  Joystick.setButton(2, BITGET(button, SNES_BUTTON_Y));
  Joystick.setButton(3, BITGET(button, SNES_BUTTON_X));
  Joystick.setButton(4, BITGET(button, SNES_BUTTON_L));
  Joystick.setButton(5, BITGET(button, SNES_BUTTON_R));
  Joystick.setButton(6, BITGET(button, SNES_BUTTON_START));
  Joystick.setButton(7, BITGET(button, SNES_BUTTON_SELECT));
  int angle = dpad_angle(
    BITGET(button, SNES_BUTTON_UP),
    BITGET(button, SNES_BUTTON_DOWN),
    BITGET(button, SNES_BUTTON_LEFT),
    BITGET(button, SNES_BUTTON_RIGHT)
  );
  if (angle < 0) angle = JOYSTICK_HATSWITCH_RELEASE;
  Joystick.setHatSwitch(0, angle);

  return -1; // go to default mode
}

// dispatcher ---------------------------------------------------------------------

void setup_first() {

#ifdef DEBUG
  Serial.begin(9600);
#endif

  //Keyboard.begin();
  //Mouse.begin();

  Joystick.begin();
  Joystick.setXAxisRange(-1, 1);
  Joystick.setYAxisRange(-1, 1);
}

void setup_last() {

  next_time_step();

  // shutdown annoying leds
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BUILTIN_RX, OUTPUT);
  pinMode(LED_BUILTIN_TX, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_BUILTIN_RX, HIGH);
  digitalWrite(LED_BUILTIN_TX, HIGH);
}

static void loop_first(void){

  next_time_step();

  // the bootloader continously turn on the annoying leds, shutdown
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_BUILTIN_RX, HIGH);
  digitalWrite(LED_BUILTIN_TX, HIGH);
}

static void loop_last(void){
  // nothing to do
}

void setup() {
  setup_first();
  setup_snes();
  setup_last();
}

void loop(){
  static int mode = -1;
  loop_first();
  switch (mode) {
  break;default: mode = loop_snes();
  // break;case  0: mode = loop_mode_0();
  // break;case  1: mode = loop_mode_1();
  }
  loop_last();
}
