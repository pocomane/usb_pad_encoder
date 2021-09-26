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
#define AUTOFIRE_SELECTOR  select   // start, select; it is used in the toggle mode; it must be one of the field of the gamepad status struct.

// This will make the dpad looks like a pair of "Digital axis"
#define USE_HAT_FOR_DPAD

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

//
// These must match the field of gamepad_status_t than want to be reported, i.e.
//
//  used in: F  FH A  S  SH
//  ------------------------
//  fire1  : x  x  x  x  x
//  fire2  : x  x  x  x  x
//  fire3  : x  x  .  x  x
//  fire4  : x  x  .  x  x
//  fire5  : x  x  .  x  x
//  fire6  : x  x  .  x  x
//  select : x  x  .  x  x
//  start  : x  x  .  x  x
//  ------------------------
//  up     : x  .  .  x  .
//  down   : x  .  .  x  .
//  left   : x  .  .  x  .
//  right  : x  .  .  x  .
//  EMPTYx4: .  .  .  .  .
//
#if INPUT_PROTOCOL == FULLSWITCH && defined( USE_HAT_FOR_DPAD)
#define HID_BUTTONS 8
#define HID_BUTTON_MASKS 8
#endif
//
#if INPUT_PROTOCOL == FULLSWITCH && !defined( USE_HAT_FOR_DPAD)
#define HID_BUTTONS 12
#define HID_BUTTON_MASKS 4
#endif
//
#if INPUT_PROTOCOL == ATARI_PADDLE
#define HID_AXIS_ENABLE
#define HID_BUTTONS 2
#define HID_BUTTON_MASKS 14
#ifdef USE_HAT_FOR_DPAD
#undef USE_HAT_FOR_DPAD
#endif
#endif
//
#if INPUT_PROTOCOL == SNES && defined( USE_HAT_FOR_DPAD)
#define HID_BUTTONS 8
#define HID_BUTTON_MASKS 8
#endif
//
#if INPUT_PROTOCOL == SNES && !defined( USE_HAT_FOR_DPAD)
#define HID_BUTTONS 12
#define HID_BUTTON_MASKS 4
#endif
//


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

static unsigned long now_us = 0;
static void next_time_step(){ now_us = micros();}
static unsigned long current_time_step(){ return now_us;}

typedef struct{
  unsigned long time;
  char event;
} timed_t;

typedef struct {

#ifdef HID_AXIS_ENABLE
  int16_t	axisX;
  int16_t	axisY;
  int16_t	unusedA;
  int16_t	unusedB;
#endif

  uint8_t fire1:   1;
  uint8_t fire2:   1;
  uint8_t fire3:   1;
  uint8_t fire4:   1;
  uint8_t fire5:   1;
  uint8_t fire6:   1;
  uint8_t select:  1;
  uint8_t start:   1;

  uint8_t up:      1;
  uint8_t down:    1;
  uint8_t left:    1;
  uint8_t right:   1;
  uint8_t unused0: 1;
  uint8_t unused1: 1;
  uint8_t unused2: 1;
  uint8_t unused3: 1;

#ifdef USE_HAT_FOR_DPAD
  uint8_t	direction: 4;
  uint8_t	unused4: 4;
#endif

} gamepad_status_t;

static void gamepad_log(gamepad_status_t *status){
  LOG(1, "gamepad report state "

      "| %x%x%x%x "
      "| %x%x%x%x "
      "| %x%x%x%x "
#ifdef USE_HAT_FOR_DPAD
      "# %x "
#endif
#ifdef HID_AXIS_ENABLE
      "% %d %d "
#endif
      ": %lu %lu",

      status->fire1, status->fire2, status->fire3, status->fire4,
      status->fire5, status->fire6, status->start, status->select,
      status->up, status->down, status->left, status->right,
#ifdef USE_HAT_FOR_DPAD
      status->direction,
#endif
#ifdef HID_AXIS_ENABLE
      status->axisX, status->axisY,
#endif
      micros() - current_time_step(), current_time_step()
   );
}

// USB HID wrapper ---------------------------------------------------------------------

#ifdef SIMULATION_MODE

void gamepad_init(){ LOG(1, "gamepad simulation initialized", 0); }
void gamepad_send(gamepad_status_t *status){ gamepad_log( status); }

#else // ! SIMULATION_MODE

#include "HID.h"
#if !defined(_USING_HID)
#error "Arduino board does not support PluggubleHID module"
#endif

#define HID_REPORT_ID (0x06)

// The content of this array must match the definition of gamepad_status_t.
// PROGMEM = store the data in the flash/program memory instead of SRAM; this is
// needed by the HID library.
static const uint8_t gamepad_hid_descriptor[] PROGMEM = {

  // Gamepad
  0x05, 0x01,               //  USAGE_PAGE (Generic Desktop)
  0x09, 0x04,               //  USAGE (Joystick)
  0xa1, 0x01,               //  COLLECTION (Application)
    0x85, HID_REPORT_ID,    //    REPORT_ID

#ifdef HID_AXIS_ENABLE
    // 16bit Axis
    0x05, 0x01,             //    USAGE_PAGE (Generic Desktop)
    0xa1, 0x00,             //    COLLECTION (Physical)
    0x09, 0x30,             //    USAGE (X)
    0x09, 0x31,             //    USAGE (Y)
    0x09, 0x33,             //    USAGE (Rx)
    0x09, 0x34,             //    USAGE (Ry)
      0x16, 0x00, 0x80,     //      LOGICAL_MINIMUM (-32768)
      0x26, 0xFF, 0x7F,     //      LOGICAL_MAXIMUM (32767)
    0x75, 0x10,             //    REPORT_SIZE (16)
    0x95, 0x4,              //    REPORT_COUNT (4)
    0x81, 0x02,             //    INPUT (Data,Var,Abs)
#endif

    // Buttons
    0x05, 0x09,             //    USAGE_PAGE (Button)
      0x19, 0x01,           //      USAGE_MINIMUM (Button 1)
      0x29, HID_BUTTONS,    //      USAGE_MAXIMUM (Buttons N = HID_BUTTONS)
    0x15, 0x00,             //    LOGICAL_MINIMUM (0)
    0x25, 0x01,             //    LOGICAL_MAXIMUM (1)
    0x75, 0x01,             //    REPORT_SIZE (1)
    0x95, HID_BUTTONS,      //    REPORT_COUNT (N = HID_BUTTONS)
    0x81, 0x02,             //    INPUT (Data,Var,Abs)

#if HID_BUTTON_MASKS > 0
    // Padding
    0x75, 0x01,             //    REPORT_SIZE (1)
    0x95, HID_BUTTON_MASKS, //    REPORT_COUNT (N = HID_BUTTON_MASKS)
    0x81, 0x03,             //    INPUT (Cnst,Var,Abs)
#endif

#ifdef USE_HAT_FOR_DPAD
    // Hat Switches
    0x05, 0x01,             //    USAGE_PAGE (Generic Desktop)
    0x09, 0x39,             //    USAGE (Hat switch)
    0x09, 0x39,             //    USAGE (Hat switch)
      0x15, 0x01,           //      LOGICAL_MINIMUM (1)
      0x25, 0x08,           //      LOGICAL_MAXIMUM (8)
    0x95, 0x02,             //    REPORT_COUNT (2)
    0x75, 0x04,             //    REPORT_SIZE (4)
    0x81, 0x02,             //    INPUT (Data,Var,Abs)
#endif

/*
    // 2 8bit Axis
    0x09, 0x32,             //    USAGE (Z)
    0x09, 0x35,             //    USAGE (Rz)
      0x15, 0x80,           //      LOGICAL_MINIMUM (-128)
      0x25, 0x7F,           //      LOGICAL_MAXIMUM (127)
    0x75, 0x08,             //    REPORT_SIZE (8)
    0x95, 0x02,             //    REPORT_COUNT (2)
    0x81, 0x02,             //    INPUT (Data,Var,Abs)
    0xc0,                   //    END_COLLECTION
*/

  0xc0                      //  END_COLLECTION
};

void gamepad_init(){

  static HIDSubDescriptor node(gamepad_hid_descriptor, sizeof(gamepad_hid_descriptor));
  HID().AppendDescriptor(&node);

  LOG(1, "gamepad initialized", 0);
}

void gamepad_send(gamepad_status_t *status){

  HID().SendReport( HID_REPORT_ID, status, sizeof(*status));
  gamepad_log(status);
}

#endif // ! SIMULATION_MODE

// Common input-related routines ---------------------------------------------------------------------

static void gamepad_process_dpad( gamepad_status_t* gamepad){
#ifdef USE_HAT_FOR_DPAD
  if( gamepad->up){
    if(      gamepad->left)  gamepad->direction = 8;
    else if( gamepad->right) gamepad->direction = 2;
    else                     gamepad->direction = 1;
  } else if( gamepad->down){
    if(      gamepad->left)  gamepad->direction = 6;
    else if( gamepad->right) gamepad->direction = 4;
    else                     gamepad->direction = 5;
  } else{
    if(      gamepad->left)  gamepad->direction = 7;
    else if( gamepad->right) gamepad->direction = 3;
    else                     gamepad->direction = 0;
  }
  gamepad->up    = 0;
  gamepad->down  = 0;
  gamepad->left  = 0;
  gamepad->right = 0;
#endif // USE_HAT_FOR_DPAD
}

static int button_debounce(timed_t* last, int current) {

  const unsigned long now = current_time_step();

  if( last->time == 0){
    // Debounce initialization
    last->time = now;
    last->event = current;

  } else if( now - last->time < 5000 /*us*/){
    // Mask unwanted bounce
    current = last->event;

  } else {
    // Debouncing passed, keep the new value
    if(last->event != current) last->time = now;
    last->event = current;
  }
  return current;
}

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

static int autofire_none( timed_t* last, int is_pressed, int option){
  return is_pressed;
}

static int autofire_assist( timed_t* last, int is_pressed, int option){

  unsigned long last_time = last->time;
  int last_pressed = last->event & 0x1;
  int tap_count = last->event >> 1;

  // store current status for the next iterations
  unsigned long press_time = last_time;
  int was_pressed = last_pressed; // TODO : clean up this
  last_pressed = is_pressed; // TODO : clean up this
  if (is_pressed && !was_pressed) {
    last_time = current_time_step();
  }

  // count the number of taps
  if (is_pressed && !was_pressed) {
    if (current_time_step() < press_time + TAP_MAX_PERIOD ) {
      tap_count += 1;
    }
  }
 
  // reset tap count if too much time is elapsed
  if (!is_pressed && current_time_step() >= press_time + TAP_MAX_PERIOD ) {
    tap_count = 0;
  }
  
  // do autofire 
  if ( is_pressed &&( tap_count >= AUTOFIRE_TAP_COUNT)){
    is_pressed = !((( current_time_step() - press_time) / AUTOFIRE_PERIOD) % 2);
  }
 
  LOG( is_pressed != was_pressed, "auto fire status: count/%d current/%d timing/%ld result/%d", tap_count, is_pressed, current_time_step() - press_time, is_pressed);

  last->time = last_time;
  last->event = (!! last_pressed) +( tap_count << 1);

  return is_pressed;
}

static int autofire_toggle( timed_t* last, int is_pressed, int is_toggled){

  unsigned long last_time = last->time;
  int last_toggle = last->event & 0x1;
  int autofire_enabled = last->event & 0x2;

  // store current status for the next iterations
  int was_toggled = last_toggle; // TODO : clean up
  last_toggle = is_toggled; // TODO : clean up
  unsigned long press_time = last_time; // TODO : clean up
  int autofire = autofire_enabled; // TODO : clean up

  // store the press time
  if (!is_pressed) {
    last_time = current_time_step(); // + 1;
  }

  // flip the toggle when the toggle-button is press and released while the target-button is pressed
  if (was_toggled && !is_toggled && is_pressed) {
    autofire_enabled = !autofire_enabled;
  }

  // do autofire
  if (autofire && is_pressed) {
    is_pressed = !(((current_time_step() - press_time) / AUTOFIRE_PERIOD ) % 2);
  }

  LOG(is_toggled != was_toggled, "auto fire status: auto/%d current/%d timing/%ld result/%d", autofire, is_pressed, current_time_step() - press_time, is_pressed);

  last->time = last_time;
  last->event = (!! last_toggle) +((!! autofire_enabled) << 1);

  return is_pressed;
}

// autofire mode selection
//
static int do_autofire( timed_t* last, int is_pressed, int option){
#if AUTOFIRE_MODE == NONE
  return autofire_none( last, is_pressed, option);
#elif AUTOFIRE_MODE == ASSIST
  return autofire_assist( last, is_pressed, option);
#elif AUTOFIRE_MODE == TOGGLE
  return autofire_toggle( last, is_pressed, option);
#else
  #error "unsupported autofire mode"
  return -1;
#endif
}

// SwitchFull protocol ------------------------------------------------------------
#if INPUT_PROTOCOL == FULLSWITCH

//
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
#define FULLSWITCH_SELECT_PIN   3
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
  pinMode(FULLSWITCH_SELECT_PIN, INPUT_PULLUP);
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

static void read_fullswitch( gamepad_status_t* gamepad) {

  gamepad->start = !digitalRead(FULLSWITCH_COIN_PIN);
  gamepad->select= !digitalRead(FULLSWITCH_SELECT_PIN);
  gamepad->up =    !digitalRead(FULLSWITCH_UP_PIN);
  gamepad->down =  !digitalRead(FULLSWITCH_DOWN_PIN);
  gamepad->left =  !digitalRead(FULLSWITCH_LEFT_PIN);
  gamepad->right = !digitalRead(FULLSWITCH_RIGHT_PIN);
  gamepad->fire1 = !digitalRead(FULLSWITCH_FIRE_1_PIN);
  gamepad->fire2 = !digitalRead(FULLSWITCH_FIRE_2_PIN);
  gamepad->fire3 = !digitalRead(FULLSWITCH_FIRE_3_PIN);
  gamepad->fire4 = !digitalRead(FULLSWITCH_FIRE_4_PIN);
  gamepad->fire5 = !digitalRead(FULLSWITCH_FIRE_5_PIN);
  gamepad->fire6 = !digitalRead(FULLSWITCH_FIRE_6_PIN);
}

static void process_fullswitch( gamepad_status_t* gamepad) {
  static timed_t autofire_slot[ 6] = { 0};
  static timed_t debounce_slot[ 12] = { 0};

  // Debounce
  gamepad->up     =  button_debounce( debounce_slot + 0,  gamepad->up);
  gamepad->down   =  button_debounce( debounce_slot + 1,  gamepad->down);
  gamepad->left   =  button_debounce( debounce_slot + 2,  gamepad->left);
  gamepad->right  =  button_debounce( debounce_slot + 3,  gamepad->right);
  gamepad->select =  button_debounce( debounce_slot + 4,  gamepad->select);
  gamepad->fire1  =  button_debounce( debounce_slot + 5,  gamepad->fire1);
  gamepad->fire2  =  button_debounce( debounce_slot + 6,  gamepad->fire2);
  gamepad->fire3  =  button_debounce( debounce_slot + 7,  gamepad->fire3);
  gamepad->fire4  =  button_debounce( debounce_slot + 8,  gamepad->fire4);
  gamepad->fire5  =  button_debounce( debounce_slot + 9,  gamepad->fire5);
  gamepad->fire6  =  button_debounce( debounce_slot + 10, gamepad->fire6);
  gamepad->start   =  button_debounce( debounce_slot + 11, gamepad->start);

  // Autofire
  gamepad->fire1 = do_autofire( autofire_slot + 0, gamepad->fire1, gamepad->AUTOFIRE_SELECTOR);
  gamepad->fire2 = do_autofire( autofire_slot + 1, gamepad->fire2, gamepad->AUTOFIRE_SELECTOR);
  gamepad->fire3 = do_autofire( autofire_slot + 2, gamepad->fire3, gamepad->AUTOFIRE_SELECTOR);
  gamepad->fire4 = do_autofire( autofire_slot + 3, gamepad->fire4, gamepad->AUTOFIRE_SELECTOR);
  gamepad->fire5 = do_autofire( autofire_slot + 4, gamepad->fire5, gamepad->AUTOFIRE_SELECTOR);
  gamepad->fire6 = do_autofire( autofire_slot + 5, gamepad->fire6, gamepad->AUTOFIRE_SELECTOR);

  gamepad_process_dpad( gamepad);
}
#endif // FULLSWITCH

// Atari Paddle protocol ----------------------------------------------------------
#if INPUT_PROTOCOL == ATARI_PADDLE

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

static void read_atari_paddle( gamepad_status_t* gamepad) {

  gamepad->fire1 = digitalRead( ATARI_PADDLE_FIRST_FIRE_PIN);
  gamepad->fire2 = digitalRead( ATARI_PADDLE_SECOND_FIRE_PIN);
  gamepad->axisX = analogRead( ATARI_PADDLE_FIRST_ANGLE_PIN);
  gamepad->axisY = analogRead( ATARI_PADDLE_SECOND_ANGLE_PIN);
}

static void process_atari_paddle( gamepad_status_t* gamepad) {
  static timed_t autofire_slot[ 2] = { 0};
  static timed_t debounce_slot[ 2] = { 0};

  // Debounce
  gamepad->fire1 = button_debounce( debounce_slot + 0, gamepad->fire1);
  gamepad->fire1 = button_debounce( debounce_slot + 1, gamepad->fire1);

  // Autofire
  // Last parameter is always zero since atari paddle have one fire button so the
  // TOGGLE mode can no be used for autofire
  gamepad->fire1 = do_autofire( autofire_slot + 0, gamepad->fire1, 0);
  gamepad->fire2 = do_autofire( autofire_slot + 1, gamepad->fire2, 0);

  // Moving average to reduce noise on the analog readinng
  static int16_t first_axis_history[10] = {0};
  static int16_t second_axis_history[10] = {0};
  gamepad->axisX = moving_average(first_axis_history,  10, gamepad->axisX);
  gamepad->axisY = moving_average(second_axis_history, 10, gamepad->axisY);

  // Axis calibration
  gamepad->axisX *= 1;
  gamepad->axisY *= 1;
}
#endif // ATARI_PADDLE

// SNES pad protocol --------------------------------------------------------------
#if INPUT_PROTOCOL == SNES

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

static int read_next_button_snes(int pin, int semiwait) {
  digitalWrite(pin, LOW);
  delayMicroseconds(semiwait);
  int result = !digitalRead(SNES_DATA_PIN);
  digitalWrite(pin, HIGH);
  delayMicroseconds(semiwait);
  return result;
}

static void read_snes( gamepad_status_t* gamepad) {

  digitalWrite(SNES_LATCH_PIN, HIGH);
  delayMicroseconds(12);
  digitalWrite(SNES_LATCH_PIN, LOW);
  delayMicroseconds(6);

  gamepad->fire2 =  read_next_button_snes( SNES_CLOCK_PIN, 6); // B
  gamepad->fire4 =  read_next_button_snes( SNES_CLOCK_PIN, 6); // Y
  gamepad->select = read_next_button_snes( SNES_CLOCK_PIN, 6);
  gamepad->start =  read_next_button_snes( SNES_CLOCK_PIN, 6);
  gamepad->up =     read_next_button_snes( SNES_CLOCK_PIN, 6);
  gamepad->down =   read_next_button_snes( SNES_CLOCK_PIN, 6);
  gamepad->left =   read_next_button_snes( SNES_CLOCK_PIN, 6);
  gamepad->right =  read_next_button_snes( SNES_CLOCK_PIN, 6);
  gamepad->fire1 =  read_next_button_snes( SNES_CLOCK_PIN, 6); // A
  gamepad->fire3 =  read_next_button_snes( SNES_CLOCK_PIN, 6); // X
  gamepad->fire5 =  read_next_button_snes( SNES_CLOCK_PIN, 6); // L
  gamepad->fire6 =  read_next_button_snes( SNES_CLOCK_PIN, 6); // R
}

static void process_snes( gamepad_status_t* gamepad) {
  static timed_t autofire_slot[ 4] = {0};

  // Autofire
  gamepad->fire1 = do_autofire( autofire_slot + 0, gamepad->fire1, gamepad->AUTOFIRE_SELECTOR);
  gamepad->fire2 = do_autofire( autofire_slot + 1, gamepad->fire2, gamepad->AUTOFIRE_SELECTOR);
  gamepad->fire3 = do_autofire( autofire_slot + 2, gamepad->fire3, gamepad->AUTOFIRE_SELECTOR);
  gamepad->fire4 = do_autofire( autofire_slot + 3, gamepad->fire4, gamepad->AUTOFIRE_SELECTOR);

  gamepad_process_dpad( gamepad);
}
#endif // SNES

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
  static gamepad_status_t old_status = {0};

  loop_first();

  gamepad_status_t gamepad;
  memset( &gamepad, sizeof( gamepad), 0);

#if INPUT_PROTOCOL == FULLSWITCH
  read_fullswitch( &gamepad);
  process_fullswitch( &gamepad);
#elif INPUT_PROTOCOL == ATARI_PADDLE
  read_atari_paddle( &gamepad);
  process_atari_paddle( &gamepad);
#elif INPUT_PROTOCOL == SNES
  read_snes( &gamepad);
  process_snes( &gamepad);
#endif

  if (memcmp( &old_status, &gamepad, sizeof( gamepad)))
    gamepad_send(&gamepad);
  old_status = gamepad;

  loop_last();
}

