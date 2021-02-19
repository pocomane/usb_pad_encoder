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

#define AUTOFIRE_MODE      ASSIST   // NONE, ASSIST, TOGGLE
#define TAP_MAX_PERIOD     (200000) // us // used in any mode except none
#define AUTOFIRE_PERIOD    (75000) // us // used in any mode except none
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

// USB HID wrapper ---------------------------------------------------------------------

#include "HID.h"
#if !defined(_USING_HID)
#error "Arduino board does not support PluggubleHID module"
#endif

#define REPORTID (0x06)

static const uint8_t gamepad_hid_descriptor[] PROGMEM = {

  // Gamepad
  0x05, 0x01,        //  USAGE_PAGE (Generic Desktop)
  0x09, 0x04,        //  USAGE (Joystick)
  0xa1, 0x01,        //  COLLECTION (Application)

    0x85, REPORTID,  //  REPORT_ID

    // 8 Buttons
    0x05, 0x09,        //  USAGE_PAGE (Button)
      0x19, 0x01,        //  USAGE_MINIMUM (Button 1)
      0x29, 0x08,        //  USAGE_MAXIMUM (Button 8)
    0x15, 0x00,        //  LOGICAL_MINIMUM (0)
    0x25, 0x01,        //  LOGICAL_MAXIMUM (1)
    0x75, 0x01,        //  REPORT_SIZE (1)
    0x95, 0x08,        //  REPORT_COUNT (8)
    0x81, 0x02,        //  INPUT (Data,Var,Abs)

    // 2 Hat Switches
    0x05, 0x01,        //  USAGE_PAGE (Generic Desktop)
    0x09, 0x39,        //  USAGE (Hat switch)
    0x09, 0x39,        //  USAGE (Hat switch)
      0x15, 0x01,        //  LOGICAL_MINIMUM (1)
      0x25, 0x08,        //  LOGICAL_MAXIMUM (8)
    0x95, 0x02,        //  REPORT_COUNT (2)
    0x75, 0x04,        //  REPORT_SIZE (4)
    0x81, 0x02,        //  INPUT (Data,Var,Abs)

/*
    // 4 16bit Axis
    0x05, 0x01,        //   USAGE_PAGE (Generic Desktop)
    0xa1, 0x00,        //   COLLECTION (Physical)
    0x09, 0x30,        //   USAGE (X)
    0x09, 0x31,        //   USAGE (Y)
    0x09, 0x33,        //   USAGE (Rx)
    0x09, 0x34,        //   USAGE (Ry)
      0x16, 0x00, 0x80,  //   LOGICAL_MINIMUM (-32768)
      0x26, 0xFF, 0x7F,  //   LOGICAL_MAXIMUM (32767)
    0x75, 0x10,        //   REPORT_SIZE (16)
    0x95, 0x04,        //   REPORT_COUNT (4)
    0x81, 0x02,        //   INPUT (Data,Var,Abs)

    // 2 8bit Axis
    0x09, 0x32,        //  USAGE (Z)
    0x09, 0x35,        //  USAGE (Rz)
      0x15, 0x80,        //  LOGICAL_MINIMUM (-128)
      0x25, 0x7F,        //  LOGICAL_MAXIMUM (127)
    0x75, 0x08,        //  REPORT_SIZE (8)
    0x95, 0x02,        //  REPORT_COUNT (2)
    0x81, 0x02,        //  INPUT (Data,Var,Abs)
    0xc0,              //  END_COLLECTION
*/

  0xc0               //  END_COLLECTION
};

typedef struct {

  uint8_t button1  : 1;
  uint8_t button2  : 1;
  uint8_t button3  : 1;
  uint8_t button4  : 1;
  uint8_t button5  : 1;
  uint8_t button6  : 1;
  uint8_t button7  : 1;
  uint8_t button8  : 1;

  uint8_t	dPad1 : 4;
  uint8_t	dPad2 : 4;

/*
  int16_t	xAxis;
  int16_t	yAxis;

  int16_t	rxAxis;
  int16_t	ryAxis;

  int8_t	zAxis;
  int8_t	rzAxis;
*/

} gamepad_status_t;

void gamepad_init(){

  static HIDSubDescriptor node(gamepad_hid_descriptor, sizeof(gamepad_hid_descriptor));
  HID().AppendDescriptor(&node);

  LOG(1, "gamepad initialized\n", 0);
}

int gamepad_send(gamepad_status_t *status){

  HID().SendReport(REPORTID, status, sizeof(*status));

  LOG(1, "gamepad report state - %x %x %x %x | %x %x %x %x | %x %x \n",
      status->button1, status->button2, status->button3, status->button4,
      status->button5, status->button6, status->button7, status->button8,
      status->dPad1, status->dPad2
   );
}

// Common input-related routines ---------------------------------------------------------------------

#define NUMBER_OF_BUTTONS   12

typedef uint16_t button_state_t;

static unsigned long now_us = 0;
static void next_time_step(void) { now_us = micros(); }
static unsigned long current_time_step(void)  { return now_us; }

static int dpad_value(int up, int down, int left, int right) {
  if (up) {
    if (left) return 8;
    else if (right) return 2;
    else return 1;
  } else if (down) {
    if (left) return 6;
    else if (right) return 4;
    else return 5;
  } else {
    if (left) return 7;
    else if (right) return 3;
    else return 0;
  }
}

// If missing, the Arduino IDE will automatically generate protypes ON THE TOP of
// the file, resulting to invalid ones, since user types are not defined yet
static button_state_t button_debounce(button_state_t button);

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
    LOG(1, "autofire not supported for button %d\n", index);
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
  static unsigned long last_press[ TOGGLED_BUTTON_NUMBER ] = {0};
  static int prev_toggle[ TOGGLED_BUTTON_NUMBER ] = {0};
  static int autofire_enabled[ TOGGLED_BUTTON_NUMBER ] = {0};

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

// If missing, the Arduino IDE will automatically generate protypes ON THE TOP of
// the file, resulting to invalid ones, since user types are not defined yet
static button_state_t read_snes(void);

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
  static button_state_t old_status = 0;

  // Read pad status + debounce
  button_state_t button = read_snes();
  button = button_debounce(button);

  // Autofire
  int opt = BITGET(button, CAT(SNES_BUTTON_, AUTOFIRE_SELECTOR)); // e.g. CAT(...) -> SNES_BUTTON_SELECT
  BITSET(button, SNES_BUTTON_B, DO_AUTOFIRE(0, BITGET(button, SNES_BUTTON_B), opt));
  BITSET(button, SNES_BUTTON_Y, DO_AUTOFIRE(1, BITGET(button, SNES_BUTTON_Y), opt));
  BITSET(button, SNES_BUTTON_A, DO_AUTOFIRE(2, BITGET(button, SNES_BUTTON_A), opt));
  BITSET(button, SNES_BUTTON_X, DO_AUTOFIRE(3, BITGET(button, SNES_BUTTON_X), opt));

  // Map the pad status to the report struct
  // TODO : better mapping betwen button_state_t and gamepad_status_t !
  gamepad_status_t data = {0};
  data.dPad1 = dpad_value(
    BITGET(button, SNES_BUTTON_UP),
    BITGET(button, SNES_BUTTON_DOWN),
    BITGET(button, SNES_BUTTON_LEFT),
    BITGET(button, SNES_BUTTON_RIGHT)
  );
  data.button1 = BITGET(button, SNES_BUTTON_A);
  data.button2 = BITGET(button, SNES_BUTTON_B);
  data.button3 = BITGET(button, SNES_BUTTON_Y);
  data.button4 = BITGET(button, SNES_BUTTON_X);
  data.button5 = BITGET(button, SNES_BUTTON_L);
  data.button6 = BITGET(button, SNES_BUTTON_R);
  data.button7 = BITGET(button, SNES_BUTTON_START);
  data.button8 = BITGET(button, SNES_BUTTON_SELECT);

  // Send USB event as needed
  if (old_status != button) gamepad_send(&data);
  old_status = button;

  return -1; // go to default mode
}

// dispatcher ---------------------------------------------------------------------

void setup_first() {

#ifdef DEBUG
  Serial.begin(9600);
#endif
  
  gamepad_init();
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

