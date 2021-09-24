/***********************************************************************************

This is free and unencumbered work released into the public domain.

Should the previous statement, for any reason, be judged legally invalid or
uneffective under applicable law, the work is made avaiable under the terms of one
of the following licences:

- Apache License 2.0 https://www.apache.org/licenses/LICENSE-2.0
- MIT License https://opensource.org/licenses/MIT
- CC0 https://creativecommons.org/publicdomain/zero/1.0/legalcode
- Unlicense https://unlicense.org/

The "USB pad encoder with Arduino"'s Author, 2021

***********************************************************************************/

#include <Arduino.h>

// Configuration ------------------------------------------------------------------

#define INPUT_PROTOCOL   SNES // FULLSWITCH, ATARI_PADDLE, SNES

#define AUTOFIRE_MODE      ASSIST   // NONE, ASSIST, TOGGLE
#define TAP_MAX_PERIOD     (200000) // us // used in any mode except none
#define AUTOFIRE_PERIOD    (75000)  // us // used in any mode except none
#define AUTOFIRE_TAP_COUNT (2)      // #  // used in assit mode
#define AUTOFIRE_SELECTOR  select   // start, select, coin; it is used in the toggle mode; it must be one of the field of the gamepad status struct.

//#define SIMULATION_MODE
//#define DEBUG

// for debug/logging only
#define SERIAL_BPS 9600 // e.g. 32u4
#define SERIAL_EOL "\n\r"

// Configuration check and expansion ---------------------------------------------------------------

#if defined(DEBUG) || defined(SIMULATION_MODE)
#define USE_SERIAL
#endif

#define FULLSWITCH   1
#define ATARI_PADDLE 2
#define SNES         3

#define NONE   1
#define ASSIST 2
#define TOGGLE 3

#if INPUT_PROTOCOL == ATARI_PADDLE && !defined(ENABLE_AXIS)
#define ENABLE_AXIS
#endif // ATARI_PADDLE

// Generic routines and macros ---------------------------------------------------------------------

#ifndef USE_SERIAL
#define LOG(...)
#else // USE_SERIAL
static void log(const char* file, int line, const char *format, ...){

  char linebuffer[256];
  va_list args;
  va_start(args, format);
  vsnprintf(linebuffer, sizeof(linebuffer), format, args);
  va_end(args);

  Serial.print(file);
  Serial.print(":");
  Serial.print(line);
  Serial.print(" ");
  Serial.print(linebuffer);
  Serial.print(SERIAL_EOL);
}
#define LOG(C, ...) do{ if( C) log(__FILE__, __LINE__, __VA_ARGS__);} while(0)
#endif // USE_SERIAL

static void bitflip(void *target, int idx){           *(unsigned long*) target ^=  (1<<(idx));}
static int  bitget( void *target, int idx){ return !!(*(unsigned long*) target &   (1<<(idx)));}
static void biton(  void *target, int idx){           *(unsigned long*) target |=  (1<<(idx));}
static void bitoff( void *target, int idx){           *(unsigned long*) target &= ~(1<<(idx));}
static void bitset( void *target, int idx, int value){
  value ? biton(target, idx) : bitoff(target, idx);
}

// USB HID wrapper ---------------------------------------------------------------------

typedef struct {

  uint8_t up:      1; // bit 0
  uint8_t down:    1; // bit 1
  uint8_t left:    1; // bit 2
  uint8_t right:   1; // bit 3
  uint8_t start:   1; // bit 4
  uint8_t select:  1; // bit 5
  uint8_t fire1:   1; // bit 6
  uint8_t fire2:   1; // bit 7
  uint8_t fire3:   1; // bit 8
  uint8_t fire4:   1; // bit 9
  uint8_t fire5:   1; // bit 10
  uint8_t fire6:   1; // bit 11
  uint8_t coin:    1; // bit 12
  uint8_t buttonD: 1; // bit 13
  uint8_t buttonE: 1; // bit 14
  uint8_t buttonF: 1; // bit 15

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
  if( a.up != b.up) return 1;
  if( a.down != b.down) return 1;
  if( a.left != b.left) return 1;
  if( a.right != b.right) return 1;
  if( a.fire1 != b.fire1) return 1;
  if( a.fire2 != b.fire2) return 1;
  if( a.fire3 != b.fire3) return 1;
  if( a.fire4 != b.fire4) return 1;
  if( a.fire5 != b.fire5) return 1;
  if( a.fire6 != b.fire6) return 1;
  if( a.start != b.start) return 1;
  if( a.select != b.select) return 1;
  if( a.coin != b.coin) return 1;
  if( a.buttonD != b.buttonD) return 1;
  if( a.buttonE != b.buttonE) return 1;
  if( a.buttonF != b.buttonF) return 1;
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

static unsigned long current_time_step(void);
static void gamepad_log(gamepad_status_t *status){
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
      status->up, status->down, status->left, status->right,
      status->fire1, status->fire2, status->fire3, status->fire4,
      status->fire5, status->fire6, status->start, status->select,
      status->coin, status->buttonD, status->buttonE, status->buttonF,
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

// PROGMEM = store the data in the flash/program memory instead of SRAM; this is
// needed by the HID library.
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

void gamepad_send(gamepad_status_t *status){

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

#define BTNUM 16
static gamepad_status_t button_debounce(gamepad_status_t button) {
  static unsigned char last_value[BTNUM] = {0};
  static unsigned long last_change[BTNUM] = {0};
  const unsigned long now = current_time_step();

  for( int k = 0; k < BTNUM; k += 1){

    if( last_change[k] == 0){
      // Debounce initialization
      int current = bitget( &button, k);
      last_change[k] = now;
      last_value[k] = current;

    } else if( now - last_change[k] < 5000 /*us*/){
      // Mask unwanted bounce
      bitset( &button, k, last_value[k]);

    } else {
      // Debouncing passed, keep the new value
      int current = bitget( &button, k);
      if(last_value[k] != current) last_change[k] = now;
      last_value[k] = current;
    }
  }
  return button;
}
#undef BTNUM

static int16_t moving_average(int16_t* buffer, int16_t size, int16_t newval){

  int16_t* index = buffer;    // The fist item is the index to the oldest inserted value
  int16_t* value = buffer +1; // The other items are the last N read values
  int n = size -1;

  // Update value history
  value[*index] = newval;
  *index += 1;
  if(*index >= n) *index = 0;

  // Calculate the average
  int result = 0;
  for(int k = 0; k < n; k += 1) result += value[k];
  result /= n;
  return result;
}

// Autofire ---------------------------------------------------------------------

static int autofire_none(int index, int is_pressed, int option) {
  return is_pressed;
}

#define ASSISTED_BUTTON_NUMBER 6
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

// SwitchFull protocol ------------------------------------------------------------

// Different controllers with the same logic:
// - One switch for each read pin
// - Pin loose <-> button/direction is released
// - Pin at ground <-> button/direction is pressed
// - Some controllers have power pin
//

// Atari/Commodore/Amiga
//
// Front view of famale DB9
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
// F = Fire (1)
//

// Master system
//
// Front view of famle DB9
// ( the one at end of the joistick cable)
//
//            R   L   D   U
//    \""""""""""""""""""""""/
//     \  O   O   O   O   O / <- DB9-pin-1
//      \   O   O   O   O  /
//       \________________/
//          2   G   V   1
//
// G = Ground
// U = Up
// D = Down
// L = Left
// R = Right
// 1 = Fire 1 ( / Start)
// 2 = Fire 2
// V = +5V (it is really needed ?)
//

// Jamma
//
// Front view of the female jamma connector
//
//                                      +**
//  Player 1 -  G..... ........CSUDLR123456.
//
//    jamma    |UUUUUU UUUUUUUUUUUUUUUUUUUUU|
//  connector  |AAAAAA AAAAAAAAAAAAAAAAAAAAA|
//
//  Player 2 -  G..... ........CSUDLR123456.
//                                      +**
//
// - It need an arduono for each player
// + Not standard but very common extension
// * Not standard and uncommon extension
//
// G = Ground
// C = Coin
// S = Start
// U = Up
// D = Down
// L = Left
// R = Right
// 1-6 = Fire 1-6
// . = not used for player controls
//

#define FULLSWITCH_COIN_PIN     2
#define FULLSWITCH_START_PIN    3
#define FULLSWITCH_UP_PIN       4
#define FULLSWITCH_DOWN_PIN     5
#define FULLSWITCH_LEFT_PIN     6
#define FULLSWITCH_RIGHT_PIN    7
#define FULLSWITCH_FIRE_1_PIN   8
#define FULLSWITCH_FIRE_2_PIN   9
#define FULLSWITCH_FIRE_3_PIN  10
#define FULLSWITCH_FIRE_4_PIN  16
#define FULLSWITCH_FIRE_5_PIN  14
#define FULLSWITCH_FIRE_6_PIN  15

static void setup_fullswitch(void){

  pinMode(FULLSWITCH_COIN_PIN, INPUT_PULLUP);
  pinMode(FULLSWITCH_START_PIN, INPUT_PULLUP);
  pinMode(FULLSWITCH_UP_PIN, INPUT_PULLUP);
  pinMode(FULLSWITCH_DOWN_PIN, INPUT_PULLUP);
  pinMode(FULLSWITCH_LEFT_PIN, INPUT_PULLUP);
  pinMode(FULLSWITCH_RIGHT_PIN, INPUT_PULLUP);
  pinMode(FULLSWITCH_FIRE_1_PIN, INPUT_PULLUP);
  pinMode(FULLSWITCH_FIRE_2_PIN, INPUT_PULLUP);
  pinMode(FULLSWITCH_FIRE_3_PIN, INPUT_PULLUP);
  pinMode(FULLSWITCH_FIRE_4_PIN, INPUT_PULLUP);
  pinMode(FULLSWITCH_FIRE_5_PIN, INPUT_PULLUP);
  pinMode(FULLSWITCH_FIRE_6_PIN, INPUT_PULLUP);
}

static gamepad_status_t read_fullswitch(void) {
  gamepad_status_t gamepad = {0};

  gamepad.coin =  !digitalRead(FULLSWITCH_COIN_PIN);
  gamepad.start = !digitalRead(FULLSWITCH_START_PIN);
  gamepad.up =    !digitalRead(FULLSWITCH_UP_PIN);
  gamepad.down =  !digitalRead(FULLSWITCH_DOWN_PIN);
  gamepad.left =  !digitalRead(FULLSWITCH_LEFT_PIN);
  gamepad.right = !digitalRead(FULLSWITCH_RIGHT_PIN);
  gamepad.fire1 = !digitalRead(FULLSWITCH_FIRE_1_PIN);
  gamepad.fire2 = !digitalRead(FULLSWITCH_FIRE_2_PIN);
  gamepad.fire3 = !digitalRead(FULLSWITCH_FIRE_3_PIN);
  gamepad.fire4 = !digitalRead(FULLSWITCH_FIRE_4_PIN);
  gamepad.fire5 = !digitalRead(FULLSWITCH_FIRE_5_PIN);
  gamepad.fire6 = !digitalRead(FULLSWITCH_FIRE_6_PIN);

  return gamepad;
}

static int loop_fullswitch(void) {
  static gamepad_status_t old_status = {0};

  // Read pad status + debounce
  gamepad_status_t gamepad = read_fullswitch();
  gamepad = button_debounce(gamepad);

  // Autofire
  gamepad.fire1 = do_autofire(0, gamepad.fire1, gamepad.AUTOFIRE_SELECTOR);
  gamepad.fire2 = do_autofire(1, gamepad.fire2, gamepad.AUTOFIRE_SELECTOR);
  gamepad.fire3 = do_autofire(2, gamepad.fire3, gamepad.AUTOFIRE_SELECTOR);
  gamepad.fire4 = do_autofire(3, gamepad.fire4, gamepad.AUTOFIRE_SELECTOR);
  gamepad.fire5 = do_autofire(4, gamepad.fire5, gamepad.AUTOFIRE_SELECTOR);
  gamepad.fire6 = do_autofire(5, gamepad.fire6, gamepad.AUTOFIRE_SELECTOR);

  // Map dpad to an hat (comment to map the dpad as regular buttons)
  gamepad.pad0 = dpad_value(
    gamepad.up,
    gamepad.down,
    gamepad.left,
    gamepad.right
  );
  gamepad.up = 0;
  gamepad.down = 0;
  gamepad.left = 0;
  gamepad.right = 0;

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

static void setup_atari_paddle(void){

  pinMode(ATARI_PADDLE_FIRST_FIRE_PIN, INPUT_PULLUP);
  pinMode(ATARI_PADDLE_FIRST_ANGLE_PIN, INPUT_PULLUP);
  pinMode(ATARI_PADDLE_SECOND_FIRE_PIN, INPUT_PULLUP);
  pinMode(ATARI_PADDLE_SECOND_ANGLE_PIN, INPUT_PULLUP);
}

static gamepad_status_t read_atari_paddle(void) {
  gamepad_status_t gamepad = {0};

  gamepad.fire1 = digitalRead(ATARI_PADDLE_FIRST_FIRE_PIN);
  gamepad.fire2 = digitalRead(ATARI_PADDLE_SECOND_FIRE_PIN);
#ifdef ENABLE_AXIS
  gamepad.xAxis = analogRead(ATARI_PADDLE_FIRST_ANGLE_PIN);
  gamepad.yAxis = analogRead(ATARI_PADDLE_SECOND_ANGLE_PIN);
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
  gamepad.fire1 = do_autofire(0, gamepad.fire1, 0);
  gamepad.fire2 = do_autofire(1, gamepad.fire2, 0);

#ifdef ENABLE_AXIS
  // Moving average to reduce noise on the analog readinng
  static int16_t first_axis_history[10] = {0};
  static int16_t second_axis_history[10] = {0};
  gamepad.xAxis = moving_average(first_axis_history, 10, gamepad.xAxis);
  gamepad.yAxis = moving_average(second_axis_history, 10, gamepad.yAxis);

  // Axis calibration
  gamepad.xAxis *= 1;
  gamepad.yAxis *= 1;
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

static void setup_snes(void){

  pinMode(SNES_CLOCK_PIN, OUTPUT);
  digitalWrite(SNES_CLOCK_PIN, HIGH);

  pinMode(SNES_LATCH_PIN, OUTPUT);
  digitalWrite(SNES_LATCH_PIN, LOW);

  pinMode(SNES_DATA_PIN, OUTPUT);
  digitalWrite(SNES_DATA_PIN, HIGH);
  pinMode(SNES_DATA_PIN, INPUT_PULLUP);
}

static gamepad_status_t read_snes(void) {
  const int button_map[] = { 7,9,5,4, 0,1,2,3, 6,8,10,11, };

  gamepad_status_t gamepad = {0};

  digitalWrite(SNES_LATCH_PIN, HIGH);
  delayMicroseconds(12);
  digitalWrite(SNES_LATCH_PIN, LOW);
  delayMicroseconds(6);

  for(int i = 0; i < 12; i++){
      digitalWrite(SNES_CLOCK_PIN, LOW);
      delayMicroseconds(6);
      bitset( &gamepad, button_map[i], !digitalRead(SNES_DATA_PIN));
      digitalWrite(SNES_CLOCK_PIN, HIGH);
      delayMicroseconds(6);
  }

  return gamepad;
}

static int loop_snes(void) {
  static gamepad_status_t old_status = {0};

  // Read pad status (no debounce needed since it is handled by the SNES HW)
  gamepad_status_t gamepad = read_snes();

  // Autofire
  gamepad.fire1 = do_autofire(0, gamepad.fire1, gamepad.AUTOFIRE_SELECTOR);
  gamepad.fire2 = do_autofire(1, gamepad.fire2, gamepad.AUTOFIRE_SELECTOR);
  gamepad.fire3 = do_autofire(2, gamepad.fire3, gamepad.AUTOFIRE_SELECTOR);
  gamepad.fire4 = do_autofire(3, gamepad.fire4, gamepad.AUTOFIRE_SELECTOR);

  // Map dpad to an hat (comment to map the dpad as regular buttons)
  gamepad.pad0 = dpad_value(
    gamepad.up,
    gamepad.down,
    gamepad.left,
    gamepad.right
  );
  gamepad.up = 0;
  gamepad.down = 0;
  gamepad.left = 0;
  gamepad.right = 0;

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

  LOG(1, "Setup completed");
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

#if INPUT_PROTOCOL == FULLSWITCH
#elif INPUT_PROTOCOL == ATARI_PADDLE
#elif INPUT_PROTOCOL == SNES
#else
#error "selected INPUT_PROTOCOL is not supported"
#endif

void setup() {
  setup_first();

#if INPUT_PROTOCOL == FULLSWITCH
  setup_fullswitch();
#elif INPUT_PROTOCOL == ATARI_PADDLE
  setup_atari_paddle();
#elif INPUT_PROTOCOL == SNES
  setup_snes();
#endif

  setup_last();
}

void loop(){
  loop_first();

#if INPUT_PROTOCOL == FULLSWITCH
  loop_fullswitch();
#elif INPUT_PROTOCOL == ATARI_PADDLE
  loop_atari_paddle();
#elif INPUT_PROTOCOL == SNES
  loop_snes();
#endif

  loop_last();
}

