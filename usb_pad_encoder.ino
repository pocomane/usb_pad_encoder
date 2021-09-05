/***********************************************************************************

This is free and unencumbered work released into the public domain.

Should the previous statement, for any reason, be judged legally invalid or
eneffective under applicable law, the work is made avaiable under the terms of one
of the following licences:

- Apache License 2.0 https://www.apache.org/licenses/LICENSE-2.0
- MIT License https://opensource.org/licenses/MIT
- CC0 https://creativecommons.org/publicdomain/zero/1.0/legalcode
- Unlicense https://unlicense.org/

The "USB pad encoder with Arduino"'s Author, 2021

***********************************************************************************/

// Config -------------------------------------------------------------------------

#define INPUT_PROTOCOL   SNES // ATARI, ATARI_PADDLE, SNES

#define AUTOFIRE_MODE      ASSIST   // NONE, ASSIST, TOGGLE
#define TAP_MAX_PERIOD     (200000) // us // used in any mode except none
#define AUTOFIRE_PERIOD    (75000)  // us // used in any mode except none
#define AUTOFIRE_TAP_COUNT (2)      // #  // used in assit mode
#define AUTOFIRE_SELECTOR  SELECT   // button: SELECT, START, etc // used in toggle mode

//#define SIMULATION_MODE
//#define DEBUG

// for debug/logging only
#define SERIAL_BPS 9600 // e.g. 32u4
#define SERIAL_EOL "\n"

// Generic routines and macros ---------------------------------------------------------------------

#if defined(DEBUG) || defined(SIMULATION_MODE)
#define USE_SERIAL
#endif

#define ATARI        1
#define ATARI_PADDLE 2
#define SNES         3

#define NONE   1
#define ASSIST 2
#define TOGGLE 3

#if INPUT_PROTOCOL == ATARI_PADDLE && !defined(ENABLE_AXIS)
#define ENABLE_AXIS
#endif // ATARI_PADDLE

#ifdef USE_SERIAL
#define LOG(C, ...) if (C) do { Serial.print(__FILE__); Serial.print(":"); Serial.print(__LINE__); Serial.print(" "); char __L[256]; snprintf(__L,255,__VA_ARGS__); __L[255] = '\0'; Serial.print(__L); Serial.print(SERIAL_EOL); } while(0)
#else
#define LOG(...)
#endif

#define CAT2(A, B) A ## B
#define CAT(A, B)  CAT2(A, B)

// USB HID wrapper ---------------------------------------------------------------------

typedef struct {

  uint8_t button0  : 1;
  uint8_t button1  : 1;
  uint8_t button2  : 1;
  uint8_t button3  : 1;
  uint8_t button4  : 1;
  uint8_t button5  : 1;
  uint8_t button6  : 1;
  uint8_t button7  : 1;
  uint8_t button8  : 1;
  uint8_t button9  : 1;
  uint8_t buttonA  : 1;
  uint8_t buttonB  : 1;
  uint8_t buttonC  : 1;
  uint8_t buttonD  : 1;
  uint8_t buttonE  : 1;
  uint8_t buttonF  : 1;

  uint8_t	pad0 : 4;
  uint8_t	pad1 : 4;

#ifdef ENABLE_AXIS
  // TODO : enable left and right axis with two different compile-time flags
  int16_t	xAxis;
  int16_t	yAxis;
  int16_t	rxAxis;
  int16_t	ryAxis;
#endif // ENABLE_AXIS

/*
  int8_t	zAxis;
  int8_t	rzAxis;
*/

} gamepad_status_t;

static int gamepad_status_change(gamepad_status_t a, gamepad_status_t b){
  if(a.button0 != b.button0) return 1;
  if(a.button1 != b.button1) return 1;
  if(a.button2 != b.button2) return 1;
  if(a.button3 != b.button3) return 1;
  if(a.button4 != b.button4) return 1;
  if(a.button5 != b.button5) return 1;
  if(a.button6 != b.button6) return 1;
  if(a.button7 != b.button7) return 1;
  if(a.button8 != b.button8) return 1;
  if(a.button9 != b.button9) return 1;
  if(a.buttonA != b.buttonA) return 1;
  if(a.buttonB != b.buttonB) return 1;
  if(a.buttonC != b.buttonC) return 1;
  if(a.buttonD != b.buttonD) return 1;
  if(a.buttonE != b.buttonE) return 1;
  if(a.buttonF != b.buttonF) return 1;
  if(a.pad0 != b.pad0) return 1;
  if(a.pad1 != b.pad1) return 1;
#ifdef ENABLE_AXIS
  int16_t dx = a.xAxis - b.xAxis;
  int16_t dy = a.yAxis - b.yAxis;
  dx = dx < 0 ? -dx : dx;
  dy = dy < 0 ? -dy : dy;
  if(dx > 1) return 1;
  if(dy > 1) return 1;
  dx = a.rxAxis - b.rxAxis;
  dy = a.ryAxis - b.ryAxis;
  dx = dx < 0 ? -dx : dx;
  dy = dy < 0 ? -dy : dy;
  if(dx > 1) return 1;
  if(dy > 1) return 1;
#endif // ENABLE_AXIS
  return 0;
}

static int gamepad_button_set(gamepad_status_t*data, int idx, int val){
  switch (idx){
    default: ;
    break; case 0x0: data->button0 = val;
    break; case 0x1: data->button1 = val;
    break; case 0x2: data->button2 = val;
    break; case 0x3: data->button3 = val;
    break; case 0x4: data->button4 = val;
    break; case 0x5: data->button5 = val;
    break; case 0x6: data->button6 = val;
    break; case 0x7: data->button7 = val;
    break; case 0x8: data->button8 = val;
    break; case 0x9: data->button9 = val;
    break; case 0xA: data->buttonA = val;
    break; case 0xB: data->buttonB = val;
    break; case 0xC: data->buttonC = val;
    break; case 0xD: data->buttonD = val;
    break; case 0xE: data->buttonE = val;
    break; case 0xF: data->buttonF = val;
  }
}

static int gamepad_log(gamepad_status_t *status){
  LOG(1, "gamepad report state "
      "| %x%x%x%x "
      "| %x%x%x%x "
      "| %x%x%x%x "
      "| %x%x%x%x "
      "- %x %x "
#ifdef ENABLE_AXIS
      "\\ %d %d %d %d "
#endif // ENABLE_AXIS
      "/ %lu %lu",
      status->button0, status->button1, status->button2, status->button3,
      status->button4, status->button5, status->button6, status->button7,
      status->button8, status->button9, status->buttonA, status->buttonB,
      status->buttonC, status->buttonD, status->buttonE, status->buttonF,
      status->pad0, status->pad1,
#ifdef ENABLE_AXIS
      status->xAxis, status->yAxis, status->rxAxis, status->ryAxis,
#endif // ENABLE_AXIS
      micros() - current_time_step(), current_time_step()
   );
}

#ifdef SIMULATION_MODE

void gamepad_init(){ LOG(1, "gamepad simulation initialized", 0); }
int gamepad_send(gamepad_status_t *status){ return gamepad_log( status); }

#else // ! SIMULATION_MODE

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
      0x29, 0x10,        //  USAGE_MAXIMUM (Button 16)
    0x15, 0x00,        //  LOGICAL_MINIMUM (0)
    0x25, 0x01,        //  LOGICAL_MAXIMUM (1)
    0x75, 0x01,        //  REPORT_SIZE (1)
    0x95, 0x10,        //  REPORT_COUNT (16)
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

#ifdef ENABLE_AXIS
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
#endif

/*
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

void gamepad_init(){

  static HIDSubDescriptor node(gamepad_hid_descriptor, sizeof(gamepad_hid_descriptor));
  HID().AppendDescriptor(&node);

  LOG(1, "gamepad initialized", 0);
}

int gamepad_send(gamepad_status_t *status){

  HID().SendReport(REPORTID, status, sizeof(*status));
  gamepad_log(status);
}

#undef REPORTID

#endif // ! SIMULATION_MODE

// Common input-related routines ---------------------------------------------------------------------

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
static gamepad_status_t button_debounce(gamepad_status_t button);

static gamepad_status_t button_debounce(gamepad_status_t button) {
  // TODO : implement !
  return button;
}

// Autofire ---------------------------------------------------------------------

static int autofire_none(int index, int is_pressed, int option) {
  return is_pressed;
}

#define ASSISTED_BUTTON_NUMBER 4
static int autofire_assist(int index, int is_pressed, int option) {
  if (index < 0 || index >= ASSISTED_BUTTON_NUMBER){
    LOG(1, "autofire not supported for button %d", index);
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
 
  LOG(is_pressed != was_pressed, "auto fire status: button/%d count/%d current/%d timing/%ld result/%d", index, tap_count[index], is_pressed, current_time_step() - press_time, is_pressed);

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

  LOG(is_toggled != was_toggled, "auto fire status: button/%d auto/%d current/%d timing/%ld result/%d", index, autofire, is_pressed, current_time_step() - press_time, is_pressed);

  return is_pressed;
}

// autofire mode selection
//
static int do_autofire(int index, int is_pressed, int option) {
#if AUTOFIRE_MODE == NONE
  return autofire_none(index, is_pressed, option);
#elif AUTOFIRE_MODE == ASSIST
  return autofire_assist(index, is_pressed, option);
#elif AUTOFIRE_MODE == TOGGLE
  return autofire_toggle(index, is_pressed, option);
#else
  #error "unsupported autofire mode"
  return -1;
#endif
}

// Atari protocol (no paddle) -----------------------------------------------------

//
// Front view of famle DB9
// ( the one at end of the joistick cable)
//
//            R   L   D   U
//    \""""""""""""""""""""""/
//     \  O   O   O   O   O / <- DB9-pin-1
//      \   O   O   O   O  /
//       \________________/
//              G       F
//
// G = Ground
// U = Up
// D = Down
// L = Left
// R = Right
// F = Fire
//
// Pin loose = button/direction is released
// Pin at ground = button/direction is pressed
//

#define ATARI_UP_PIN     13
#define ATARI_DOWN_PIN   14
#define ATARI_LEFT_PIN   25
#define ATARI_RIGHT_PIN  26
#define ATARI_FIRE_PIN   27

#define ATARI_BUTTON_FIRE    button0
#define ATARI_BUTTON_UP      button1
#define ATARI_BUTTON_DOWN    button2
#define ATARI_BUTTON_LEFT    button3
#define ATARI_BUTTON_RIGHT   button4

static void setup_atari(void){

  pinMode(ATARI_UP_PIN, INPUT_PULLUP);
  pinMode(ATARI_DOWN_PIN, INPUT_PULLUP);
  pinMode(ATARI_LEFT_PIN, INPUT_PULLUP);
  pinMode(ATARI_RIGHT_PIN, INPUT_PULLUP);
  pinMode(ATARI_FIRE_PIN, INPUT_PULLUP);
}

// If missing, the Arduino IDE will automatically generate protypes ON THE TOP of
// the file, resulting to invalid ones, since user types are not defined yet
static gamepad_status_t read_atari(void);

static gamepad_status_t read_atari(void) {
  gamepad_status_t gamepad = {0};

  gamepad.ATARI_BUTTON_FIRE =  !digitalRead(ATARI_FIRE_PIN);
  gamepad.ATARI_BUTTON_UP =    !digitalRead(ATARI_UP_PIN);
  gamepad.ATARI_BUTTON_DOWN =  !digitalRead(ATARI_DOWN_PIN);
  gamepad.ATARI_BUTTON_LEFT =  !digitalRead(ATARI_LEFT_PIN);
  gamepad.ATARI_BUTTON_RIGHT = !digitalRead(ATARI_RIGHT_PIN);

  return gamepad;
}

static int loop_atari(void) {
  static gamepad_status_t old_status = {0};

  // Read pad status + debounce
  gamepad_status_t gamepad = read_atari();
  gamepad = button_debounce(gamepad);

  // Autofire
  // Last parameter is always zero since atari pad have one fire button so the
  // TOGGLE mode can no be used for autofire
  gamepad.ATARI_BUTTON_FIRE = do_autofire(0, gamepad.ATARI_BUTTON_FIRE, 0);

  // Map dpad to an hat (comment to map the dpad as regular buttons)
  gamepad.pad0 = dpad_value(
    gamepad.ATARI_BUTTON_UP,
    gamepad.ATARI_BUTTON_DOWN,
    gamepad.ATARI_BUTTON_LEFT,
    gamepad.ATARI_BUTTON_RIGHT
  );
  gamepad.ATARI_BUTTON_UP = 0;
  gamepad.ATARI_BUTTON_DOWN = 0;
  gamepad.ATARI_BUTTON_LEFT = 0;
  gamepad.ATARI_BUTTON_RIGHT = 0;

  // Send USB event as needed
  if (gamepad_status_change(old_status, gamepad)) gamepad_send(&gamepad);
  old_status = gamepad;

  return 0;
}

// Atari Paddle protocol ----------------------------------------------------------

//
// Front view of famle DB9
// ( the one at end of the paddle cable)
//
//        SA  FF  SF
//    \""""""""""""""""""""""/
//     \  O   O   O   O   O / <- DB9-pin-1
//      \   O   O   O   O  /
//       \________________/
//         FA   G   R
//
// G  = Ground
// R  = Return for paddle resistence
// FF = First player Fire
// FA = First player Angle
// SF = Second player Fire
// SA = Second player Angle
//
// Fire pin loose = button/direction is released
// Fire pin connected to Ground pin = button/direction is pressed
// Angle pin resistence to the Return pin is proportional to the paddle position (linear 1 Mohm, 270 degree)
//

#define ATARI_PADDLE_FIRST_FIRE_PIN     22
#define ATARI_PADDLE_FIRST_ANGLE_PIN    34
#define ATARI_PADDLE_SECOND_FIRE_PIN    23
#define ATARI_PADDLE_SECOND_ANGLE_PIN   35

#define ATARI_PADDLE_FIRST_BUTTON    button0
#define ATARI_PADDLE_SECOND_BUTTON   button1
#define ATARI_PADDLE_FIRST_AXIS      xAxis
#define ATARI_PADDLE_SECOND_AXIS     yAxis

static void setup_atari_paddle(void){

  pinMode(ATARI_PADDLE_FIRST_FIRE_PIN, INPUT_PULLUP);
  pinMode(ATARI_PADDLE_FIRST_ANGLE_PIN, INPUT_PULLUP);
  pinMode(ATARI_PADDLE_SECOND_FIRE_PIN, INPUT_PULLUP);
  pinMode(ATARI_PADDLE_SECOND_ANGLE_PIN, INPUT_PULLUP);
}

// If missing, the Arduino IDE will automatically generate protypes ON THE TOP of
// the file, resulting to invalid ones, since user types are not defined yet
static gamepad_status_t read_atari_paddle(void);

static gamepad_status_t read_atari_paddle(void) {
  gamepad_status_t gamepad = {0};

  gamepad.ATARI_PADDLE_FIRST_BUTTON =  digitalRead(ATARI_PADDLE_FIRST_FIRE_PIN);
  gamepad.ATARI_PADDLE_SECOND_BUTTON = digitalRead(ATARI_PADDLE_SECOND_FIRE_PIN);
#ifdef ENABLE_AXIS
  gamepad.ATARI_PADDLE_FIRST_AXIS =    analogRead(ATARI_PADDLE_FIRST_ANGLE_PIN);
  gamepad.ATARI_PADDLE_SECOND_AXIS =   analogRead(ATARI_PADDLE_SECOND_ANGLE_PIN);
#endif // ENABLE_AXIS

  return gamepad;
}

static int loop_atari_paddle(void) {
  static gamepad_status_t old_status = {0};

  // Read pad status + debounce
  gamepad_status_t gamepad = read_atari_paddle();
  gamepad = button_debounce(gamepad);

  // Autofire
  // Last parameter is always zero since atari paddle have one fire button so the
  // TOGGLE mode can no be used for autofire
  gamepad.ATARI_PADDLE_FIRST_BUTTON = do_autofire(0, gamepad.ATARI_PADDLE_FIRST_BUTTON, 0);
  gamepad.ATARI_PADDLE_SECOND_BUTTON = do_autofire(1, gamepad.ATARI_PADDLE_SECOND_BUTTON, 0);

#ifdef ENABLE_AXIS
  // Axis calibration
  gamepad.ATARI_PADDLE_FIRST_AXIS  *= 1;
  gamepad.ATARI_PADDLE_SECOND_AXIS *= 1;
#endif // ENABLE_AXIS

  // Send USB event as needed
  if (gamepad_status_change(old_status, gamepad)) gamepad_send(&gamepad);
  old_status = gamepad;

  return 0;
}

// SNES pad protocol --------------------------------------------------------------

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
  
#define SNES_BUTTON_B        button0
#define SNES_BUTTON_Y        button1
#define SNES_BUTTON_SELECT   button2
#define SNES_BUTTON_START    button3
#define SNES_BUTTON_UP       button4
#define SNES_BUTTON_DOWN     button5
#define SNES_BUTTON_LEFT     button6
#define SNES_BUTTON_RIGHT    button7
#define SNES_BUTTON_A        button8
#define SNES_BUTTON_X        button9
#define SNES_BUTTON_L        buttonA
#define SNES_BUTTON_R        buttonB

static void setup_snes(void){

  pinMode(SNES_CLOCK_PIN, OUTPUT);
  digitalWrite(SNES_CLOCK_PIN, HIGH);

  pinMode(SNES_LATCH_PIN, OUTPUT);
  digitalWrite(SNES_LATCH_PIN, LOW);

  pinMode(SNES_DATA_PIN, OUTPUT);
  digitalWrite(SNES_DATA_PIN, HIGH);
  pinMode(SNES_DATA_PIN, INPUT_PULLUP);
}

// If missing, the Arduino IDE will automatically generate protypes ON THE TOP of
// the file, resulting to invalid ones, since user types are not defined yet
static gamepad_status_t read_snes(void);

static gamepad_status_t read_snes(void) {
  gamepad_status_t gamepad = {0};

  digitalWrite(SNES_LATCH_PIN, HIGH);
  delayMicroseconds(12);
  digitalWrite(SNES_LATCH_PIN, LOW);
  delayMicroseconds(6);

  for(int i = 0; i < 12; i++){
      digitalWrite(SNES_CLOCK_PIN, LOW);
      delayMicroseconds(6);
      gamepad_button_set(&gamepad, i, !digitalRead(SNES_DATA_PIN));
      digitalWrite(SNES_CLOCK_PIN, HIGH);
      delayMicroseconds(6);
  }

  return gamepad;
}

static int loop_snes(void) {
  static gamepad_status_t old_status = {0};

  // Read pad status + debounce
  gamepad_status_t gamepad = read_snes();
  // gamepad = button_debounce(gamepad); // No debounce needed ? SNES HW should address it !

  // Autofire
  int opt = gamepad.CAT(SNES_BUTTON_, AUTOFIRE_SELECTOR); // e.g. CAT(...) -> SNES_BUTTON_SELECT // TODO : clean up !
  gamepad.SNES_BUTTON_B = do_autofire(0, gamepad.SNES_BUTTON_B, opt);
  gamepad.SNES_BUTTON_Y = do_autofire(1, gamepad.SNES_BUTTON_Y, opt);
  gamepad.SNES_BUTTON_A = do_autofire(2, gamepad.SNES_BUTTON_A, opt);
  gamepad.SNES_BUTTON_X = do_autofire(3, gamepad.SNES_BUTTON_X, opt);

  // Map dpad to an hat (comment to map the dpad as regular buttons)
  gamepad.pad0 = dpad_value(
    gamepad.SNES_BUTTON_UP,
    gamepad.SNES_BUTTON_DOWN,
    gamepad.SNES_BUTTON_LEFT,
    gamepad.SNES_BUTTON_RIGHT
  );
  gamepad.SNES_BUTTON_UP = 0;
  gamepad.SNES_BUTTON_DOWN = 0;
  gamepad.SNES_BUTTON_LEFT = 0;
  gamepad.SNES_BUTTON_RIGHT = 0;

  // Send USB event as needed
  if (gamepad_status_change(old_status, gamepad)) gamepad_send(&gamepad);
  old_status = gamepad;

  return 0;
}

// dispatcher ---------------------------------------------------------------------

void setup_first() {

#ifdef USE_SERIAL
  Serial.begin(SERIAL_BPS);
#endif
  
  gamepad_init();
}

void setup_last() {

  next_time_step();

#ifdef LED_BUILTIN_TX
  // shutdown annoying leds
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BUILTIN_RX, OUTPUT);
  pinMode(LED_BUILTIN_TX, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_BUILTIN_RX, HIGH);
  digitalWrite(LED_BUILTIN_TX, HIGH);
#endif // LED_BUILTIN_TX
}

static void loop_first(void){

  next_time_step();

#ifdef LED_BUILTIN_TX
  // the bootloader continously turn on the annoying leds, shutdown
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_BUILTIN_RX, HIGH);
  digitalWrite(LED_BUILTIN_TX, HIGH);
#endif // LED_BUILTIN_TX
}

static void loop_last(void){
  // nothing to do
}

#if INPUT_PROTOCOL == ATARI
#elif INPUT_PROTOCOL == ATARI_PADDLE
#elif INPUT_PROTOCOL == SNES
#else
#error "selected INPUT_PROTOCOL is not supported"
#endif

void setup() {
  setup_first();

#if INPUT_PROTOCOL == ATARI
  setup_atari();
#elif INPUT_PROTOCOL == ATARI_PADDLE
  setup_atari_paddle();
#elif INPUT_PROTOCOL == SNES
  setup_snes();
#endif

  setup_last();
}

void loop(){
  loop_first();

#if INPUT_PROTOCOL == ATARI
  loop_atari();
#elif INPUT_PROTOCOL == ATARI_PADDLE
  loop_atari_paddle();
#elif INPUT_PROTOCOL == SNES
  loop_snes();
#endif

  loop_last();
}

