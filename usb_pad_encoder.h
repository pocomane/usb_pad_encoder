/***********************************************************************************

Hereby the Authors release this software into the public domain, in order to make
it as free and unencumbered as possible.

Should the previous statement be judged legally invalid or uneffective under
applicable law, the work is made avaiable under the terms of one of the
following licences:

- CC0 https://creativecommons.org/publicdomain/zero/1.0/legalcode
- MIT License https://opensource.org/licenses/MIT
- Apache License 2.0 https://www.apache.org/licenses/LICENSE-2.0
- Unlicense https://unlicense.org/

The "USB pad encoder with Arduino"'s Authors, 2023

***********************************************************************************/

// # Description
// This is an usb encoder that lets you to read simple switched plus various
// common pad protocols. You can configure some of its aspects in the first two
// sections (Configuration and Advanced Configuration). Look at the Readme file
// for further infromations.
//
// # Usage
// This is written as a single file library. Include it as a normal header
// where you needed to call its functions, i.e:
//   usb_pad_encoder_init, usb_pad_encoder_step
// Moreover it must be included in a single place after the definition of the
//   INCLUDE_IMPLEMENTION
// macro (it will include the actual code). In such place the following
// functions or macros MUST be visible:
//   LOG, setup_input, setup_output, get_elasped_microsecond, delay_microsecond,
//   read_digital, read_analog, write_digital, use_hid_descriptor, send_hid_report
// Moreover the following macro must be set if some platform need additional
// attributes for the HID descriptor array:
//   HID_DESCRIPTOR_ATTRIBUTE

// Configuration ------------------------------------------------------------------

#define ENABLE_SNES
#define ENABLE_FULLSWITCH
//#define ENABLE_ATARI_PADDLE

#define AUTOFIRE_MODE      ASSIST   // NONE, ASSIST, TOGGLE
#define TAP_MAX_PERIOD     (200000) // us // used in any mode except none
#define AUTOFIRE_PERIOD    (75000)  // us // used in any mode except none
#define AUTOFIRE_TAP_COUNT (2)      // #  // used in assit mode
#define AUTOFIRE_SELECTOR  select   // start, select; it is used in the toggle mode; it must be one of the field of the gamepad status struct.

// This will make the dpad looks like a pair of "Digital axis"
#define USE_HAT_FOR_DPAD

// Advanced Configuration ---------------------------------------------------------

#define FULLSWITCH_UP_PIN      16
#define FULLSWITCH_DOWN_PIN     8
#define FULLSWITCH_LEFT_PIN    14
#define FULLSWITCH_RIGHT_PIN    7
#define FULLSWITCH_SELECT_PIN  19
#define FULLSWITCH_COIN_PIN    20
#define FULLSWITCH_FIRE_1_PIN  15 // This will be used also as: ATARI_PADDLE_FIRST_FIRE_PIN
#define FULLSWITCH_FIRE_2_PIN   6 // This will be used olso as: ATARI_PADDLE_SECOND_FIRE_PIN
#define FULLSWITCH_FIRE_3_PIN   9
#define FULLSWITCH_FIRE_4_PIN   5
#define FULLSWITCH_FIRE_5_PIN  10
#define FULLSWITCH_FIRE_6_PIN   2 // This will be used also as: SNES_DATA_PIN
#define FULLSWITCH_FIRE_7_PIN   3 // This will be used also as: SNES_LATCH_PIN
#define FULLSWITCH_FIRE_8_PIN   4 // This will be used also as: SNES_CLOCK_PIN or SNES_DATA_PIN
#define FULLSWITCH_FIRE_9_PIN  18 // Must be Analog - This will be used also as: ATARI_PADDLE_FIRST_ANGLE_PIN or SNES_DATA_PIN
#define FULLSWITCH_FIRE_10_PIN 21 // Must be Analog - This will be used also as: ATARI_PADDLE_SECOND_ANGLE_PIN or SNES_LATCH_PIN

/*
// Old Jamma Coin Op Adapter
// NOTE atari and snes must be disabled
#define FULLSWITCH_UP_PIN       4
#define FULLSWITCH_DOWN_PIN     5
#define FULLSWITCH_LEFT_PIN     6
#define FULLSWITCH_RIGHT_PIN    7
#define FULLSWITCH_SELECT_PIN   3
#define FULLSWITCH_COIN_PIN     2
#define FULLSWITCH_FIRE_1_PIN   8
#define FULLSWITCH_FIRE_2_PIN   9
#define FULLSWITCH_FIRE_3_PIN  10
#define FULLSWITCH_FIRE_4_PIN  16
#define FULLSWITCH_FIRE_5_PIN  15
#define FULLSWITCH_FIRE_6_PIN  14
#define FULLSWITCH_FIRE_7_PIN  18
#define FULLSWITCH_FIRE_8_PIN  19
#define FULLSWITCH_FIRE_9_PIN  18 // NOT USED / DISABLE ENABLE_ATARI_PADDLE
#define FULLSWITCH_FIRE_10_PIN 21 // NOT USED / DISABLE ENABLE_ATARI_PADDLE
*/

// Header guard  ----------------------------------------------------------
// TODO : move this at very beginning of this file ?

// This DOES NOT end at the end of the file, but some line below, since the rest
// of the file is protected by the Implementation guard (i.e. it will be included
// in a single place)
#ifndef USB_PAD_ENCODER_H
#define USB_PAD_ENCODER_H

#include <stdint.h>
#include <string.h>

void usb_pad_encoder_init();
void usb_pad_encoder_step();

#endif // USB_PAD_ENCODER_H

// Implementation guard  ----------------------------------------------------------

// This lasts until the end of the file
#ifdef INCLUDE_IMPLEMENTATION

// Configuration check and expansion ----------------------------------------------

#ifndef HID_DESCRIPTOR_ATTRIBUTE
#define HID_DESCRIPTOR_ATTRIBUTE
#endif

#ifdef ENABLE_FULLSWITCH
#else // ENABLE_FULLSWITCH
#error fullswitch can not be turned of currently
#endif // ENABLE_FULLSWITCH

#ifdef USE_HAT_FOR_DPAD
#define HID_BUTTON_OFFSET_DPAD  4
#define HID_BUTTON_PADDING_DPAD 0
#define HID_AXIS_DPAD           0
#else // USE_HAT_FOR_DPAD
#define HID_BUTTON_OFFSET_DPAD  0
#define HID_BUTTON_PADDING_DPAD 0
#define HID_AXIS_DPAD           0
#endif // USE_HAT_FOR_DPAD

#ifdef ENABLE_SNES
#define SNES_DATA_PIN   FULLSWITCH_FIRE_6_PIN
#define SNES_LATCH_PIN  FULLSWITCH_FIRE_7_PIN
#define SNES_CLOCK_PIN  FULLSWITCH_FIRE_8_PIN
#define HID_BUTTON_OFFSET_SNES  0
#define HID_BUTTON_PADDING_SNES 3
#define HID_AXIS_SNES           0
#else // ENABLE_SNES
#define HID_BUTTON_OFFSET_SNES  0
#define HID_BUTTON_PADDING_SNES 0
#define HID_AXIS_SNES           0
#endif // ENABLE_SNES

#ifdef ENABLE_ATARI_PADDLE
#define ATARI_PADDLE_FIRST_FIRE_PIN    FULLSWITCH_FIRE_1_PIN
#define ATARI_PADDLE_FIRST_ANGLE_PIN   FULLSWITCH_FIRE_9_PIN
#define ATARI_PADDLE_SECOND_FIRE_PIN   FULLSWITCH_FIRE_2_PIN
#define ATARI_PADDLE_SECOND_ANGLE_PIN  FULLSWITCH_FIRE_10_PIN
#define HID_BUTTON_OFFSET_ATARI_PADDLE  0
#define HID_BUTTON_PADDING_ATARI_PADDLE 2
#define HID_AXIS_ATARI_PADDLE 2
#else // ENABLE_ATARI_PADDLE
#define HID_BUTTON_OFFSET_ATARI_PADDLE  0
#define HID_BUTTON_PADDING_ATARI_PADDLE 0
#define HID_AXIS_ATARI_PADDLE 0
#endif // ENABLE_ATARI_PADDLE

// These are needed to align the HID report fields to the gamepad_status_t ones
#define HID_BUTTON_OFFSET  ( HID_BUTTON_OFFSET_DPAD + HID_BUTTON_OFFSET_SNES + HID_BUTTON_OFFSET_ATARI_PADDLE)
#define HID_BUTTON_PADDING ( HID_BUTTON_PADDING_DPAD + HID_BUTTON_PADDING_SNES + HID_BUTTON_PADDING_ATARI_PADDLE)
#define HID_AXIS           ( HID_AXIS_DPAD + HID_AXIS_SNES + HID_AXIS_ATARI_PADDLE)
#define HID_BUTTONS        ( 16 - HID_BUTTON_OFFSET - HID_BUTTON_PADDING)
#if HID_BUTTONS < 0
#error wrong button configuration
#endif

#define NONE   1
#define ASSIST 2
#define TOGGLE 3

// Generic routines and macros ----------------------------------------------------

static unsigned long now_us = 0;
static void next_time_step(){ now_us = get_elasped_microsecond();}
static unsigned long current_time_step(){ return now_us;}

typedef struct{
  unsigned long time;
  char event;
} timed_t;

typedef struct {

  uint8_t up:      1;
  uint8_t down:    1;
  uint8_t left:    1;
  uint8_t right:   1;
  uint8_t select:  1;
  uint8_t start:   1;
  uint8_t fire1:   1;
  uint8_t fire2:   1;

  uint8_t fire3:   1;
  uint8_t fire4:   1;
  uint8_t fire5:   1;

  // The buttons that are not used must always be at the end, so they can be
  // masked through padding in the HID descriptot
#if !defined( ENABLE_SNES)
  uint8_t fire6:   1;
  uint8_t fire7:   1;
  uint8_t fire8:   1;
#endif
#if !defined( ENABLE_ATARI_PADDLE)
  uint8_t fire9:   1;
  uint8_t fire10:  1;
#endif
#if defined( ENABLE_SNES)
  uint8_t fire6:   1;
  uint8_t fire7:   1;
  uint8_t fire8:   1;
#endif
#if defined( ENABLE_ATARI_PADDLE)
  uint8_t fire9:   1;
  uint8_t fire10:  1;
#endif

#ifdef USE_HAT_FOR_DPAD
  uint8_t	direction: 4;
  uint8_t	unused1: 4;
#endif

#if HID_AXIS > 0
  int16_t	axis[HID_AXIS];
#endif // HID_AXIS

} gamepad_status_t;

// USB HID wrapper ----------------------------------------------------------------

#define HID_REPORT_ID (0x06)

// The content of this array must match the definition of gamepad_status_t.
static const uint8_t gamepad_hid_descriptor[] HID_DESCRIPTOR_ATTRIBUTE = {

  // Gamepad
  0x05, 0x01,               //  USAGE_PAGE (Generic Desktop)
  0x09, 0x04,               //  USAGE (Joystick)
  0xa1, 0x01,               //  COLLECTION (Application)
    0x85, HID_REPORT_ID,    //    REPORT_ID

#if HID_BUTTON_OFFSET > 0
    // Mask leading bits
    0x75, 0x01,              //    REPORT_SIZE (1)
    0x95, HID_BUTTON_OFFSET, //    REPORT_COUNT (N = HID_BUTTON_OFFSET)
    0x81, 0x03,              //    INPUT (Cnst,Var,Abs)
#endif

#if HID_BUTTONS > 0
    // Active buttons
    0x05, 0x09,             //    USAGE_PAGE (Button)
      0x19, 0x01,           //      USAGE_MINIMUM (Button 1)
      0x29, HID_BUTTONS,    //      USAGE_MAXIMUM (Buttons N = HID_BUTTONS)
    0x15, 0x00,             //    LOGICAL_MINIMUM (0)
    0x25, 0x01,             //    LOGICAL_MAXIMUM (1)
    0x75, 0x01,             //    REPORT_SIZE (1)
    0x95, HID_BUTTONS,      //    REPORT_COUNT (N = HID_BUTTONS)
    0x81, 0x02,             //    INPUT (Data,Var,Abs)
#endif

#if HID_BUTTON_PADDING > 0
    // Mask trailing bits
    0x75, 0x01,               //    REPORT_SIZE (1)
    0x95, HID_BUTTON_PADDING, //    REPORT_COUNT (N = HID_BUTTON_PADDING)
    0x81, 0x03,               //    INPUT (Cnst,Var,Abs)
#endif

#ifdef USE_HAT_FOR_DPAD
    // Hat Switches
    0x05, 0x01,             //    USAGE_PAGE (Generic Desktop)
    0x09, 0x39,             //    USAGE (Hat switch)
      0x15, 0x01,           //      LOGICAL_MINIMUM (1)
      0x25, 0x08,           //      LOGICAL_MAXIMUM (8)
    // 0x46, 0x3B, 0x01,       //    Physical Maximum  : 315 degrees (Optional)
    0x95, 0x01,             //    REPORT_COUNT (1)
    0x75, 0x04,             //    REPORT_SIZE (4)
    // 0x65, 0x14,             //    Unit: English Rotation/Angular Position 1 degree (Optional)
    // 0x81, 0x42,             //    INPUT (Data, Var, Abs, Null State)
    0x81, 0x02,             //    INPUT (Data,Var,Abs)
#endif

#if HID_AXIS > 0
    // 16bit Axis
    0x05, 0x01,             //    USAGE_PAGE (Generic Desktop)
    // 0xa1, 0x00,             //    COLLECTION (Physical)
    0x09, 0x30,             //    USAGE (X)
#if HID_AXIS > 1
    0x09, 0x31,             //    USAGE (Y)
#endif
#if HID_AXIS > 2
    0x09, 0x33,             //    USAGE (Rx)
#endif
#if HID_AXIS > 3
    0x09, 0x34,             //    USAGE (Ry)
#endif
      0x16, 0x00, 0x80,     //      LOGICAL_MINIMUM (-32768)
      0x26, 0xFF, 0x7F,     //      LOGICAL_MAXIMUM (32767)
    0x75, 0x10,             //    REPORT_SIZE (16)
    0x95, HID_AXIS,         //    REPORT_COUNT (N = HID_AXIS)
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

void config_log(){
  int hat = 0;
#ifdef USE_HAT_FOR_DPAD
  hat = 1;
#endif
  LOG(1, "configuration report id: %d | button: %d # hat: %d > axis: %d @ offset/padding: %d/%d",
      HID_REPORT_ID, HID_BUTTONS, hat, HID_AXIS, HID_BUTTON_OFFSET, HID_BUTTON_PADDING);
}

void gamepad_log(void* data){
  gamepad_status_t* status = (gamepad_status_t*) data;
  config_log();
  LOG(1, "gamepad report state "

      "| %x%x "
      "- %x%x "
      "- %x%x%x%x "
      "- %x%x%x "
      "| %x%x%x%x "
#ifdef USE_HAT_FOR_DPAD
      "# %x "
#endif
#if HID_AXIS > 0
      "> %d %d "
#endif
      ": %lu %lu",

      status->start, status->select,
      status->fire1, status->fire2,
      status->fire3, status->fire4, status->fire5, status->fire6,
      status->fire7, status->fire9, status->fire10,

      status->up, status->down, status->left, status->right,
#ifdef USE_HAT_FOR_DPAD
      status->direction,
#endif
#if HID_AXIS > 0
      status->axis[0], status->axis[1],
#endif
      get_elasped_microsecond() - current_time_step(), current_time_step()
   );
}

void gamepad_init(){

  use_hid_descriptor(gamepad_hid_descriptor, sizeof(gamepad_hid_descriptor));
}

void gamepad_send(gamepad_status_t *status){

  send_hid_report( HID_REPORT_ID, status, sizeof(*status));
  gamepad_log( status);
}

// Common input-related routines --------------------------------------------------

static void process_dpad( gamepad_status_t* gamepad){
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

// Autofire -----------------------------------------------------------------------

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

static void process_autofire( gamepad_status_t* gamepad) {
  static timed_t autofire_slot[ 4] = { 0};

  gamepad->fire1 = do_autofire( autofire_slot + 0, gamepad->fire1, gamepad->AUTOFIRE_SELECTOR);
  gamepad->fire2 = do_autofire( autofire_slot + 1, gamepad->fire2, gamepad->AUTOFIRE_SELECTOR);
  gamepad->fire3 = do_autofire( autofire_slot + 2, gamepad->fire3, gamepad->AUTOFIRE_SELECTOR);
  gamepad->fire4 = do_autofire( autofire_slot + 3, gamepad->fire4, gamepad->AUTOFIRE_SELECTOR);
}

// SwitchFull protocol ------------------------------------------------------------

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

static void setup_fullswitch(void){
#if defined(ENABLE_FULLSWITCH)

  setup_input( FULLSWITCH_UP_PIN, 1);
  setup_input( FULLSWITCH_DOWN_PIN, 1);
  setup_input( FULLSWITCH_LEFT_PIN, 1);
  setup_input( FULLSWITCH_RIGHT_PIN, 1);
  setup_input( FULLSWITCH_SELECT_PIN, 1);
  setup_input( FULLSWITCH_COIN_PIN, 1);
  setup_input( FULLSWITCH_FIRE_1_PIN, 1);
  setup_input( FULLSWITCH_FIRE_2_PIN, 1);
  setup_input( FULLSWITCH_FIRE_3_PIN, 1);
  setup_input( FULLSWITCH_FIRE_4_PIN, 1);
  setup_input( FULLSWITCH_FIRE_5_PIN, 1);
#if !defined( ENABLE_SNES)
  setup_input( FULLSWITCH_FIRE_6_PIN, 1);
  setup_input( FULLSWITCH_FIRE_7_PIN, 1);
  setup_input( FULLSWITCH_FIRE_8_PIN, 1);
#endif // ENABLE_SNES
#if !defined( ENABLE_ATARI_PADDLE)
  setup_input( FULLSWITCH_FIRE_9_PIN,  1);
  setup_input( FULLSWITCH_FIRE_10_PIN, 1);
#endif // ENABLE_ATARI_PADDLE
#endif // ENABLE_FULLSWITCH
}


static void read_fullswitch( gamepad_status_t* gamepad) {
#if defined( ENABLE_FULLSWITCH)
  static timed_t debounce_slot[ 16] = { 0};

#define RDD( I, P) button_debounce( debounce_slot + (I), !read_digital( P ))
  gamepad->up |=    RDD( 0, FULLSWITCH_UP_PIN);
  gamepad->down |=  RDD( 1, FULLSWITCH_DOWN_PIN);
  gamepad->left |=  RDD( 2, FULLSWITCH_LEFT_PIN);
  gamepad->right |= RDD( 3, FULLSWITCH_RIGHT_PIN);
  gamepad->select|= RDD( 4, FULLSWITCH_SELECT_PIN);
  gamepad->start |= RDD( 5, FULLSWITCH_COIN_PIN);
  gamepad->fire1 |= RDD( 6, FULLSWITCH_FIRE_1_PIN);
  gamepad->fire2 |= RDD( 7, FULLSWITCH_FIRE_2_PIN);
  gamepad->fire3 |= RDD( 8, FULLSWITCH_FIRE_3_PIN);
  gamepad->fire4 |= RDD( 9, FULLSWITCH_FIRE_4_PIN);
  gamepad->fire5 |= RDD( 10, FULLSWITCH_FIRE_5_PIN);
#if !defined( ENABLE_SNES)
  gamepad->fire6 |= RDD( 11, FULLSWITCH_FIRE_6_PIN);
  gamepad->fire7 |= RDD( 12, FULLSWITCH_FIRE_7_PIN);
  gamepad->fire8 |= RDD( 13, FULLSWITCH_FIRE_8_PIN);
#endif // ENABLE_SNES
#if !defined( ENABLE_ATARI_PADDLE)
  gamepad->fire9 |= RDD( 14, FULLSWITCH_FIRE_9_PIN);
  gamepad->fire10|= RDD( 15, FULLSWITCH_FIRE_10_PIN);
#endif // ENABLE_ATARI_PADDLE
#undef RDD
#endif // ENABLE_FULLSWITCH
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

static void setup_atari_paddle(void){
#if defined( ENABLE_ATARI_PADDLE)

  setup_input( ATARI_PADDLE_FIRST_FIRE_PIN, 1);
  setup_input( ATARI_PADDLE_FIRST_ANGLE_PIN, 1);
  setup_input( ATARI_PADDLE_SECOND_FIRE_PIN, 1);
  setup_input( ATARI_PADDLE_SECOND_ANGLE_PIN, 1);
#endif // ENABLE_ATARI_PADDLE
}


static void read_atari_paddle( gamepad_status_t* gamepad) {
#if defined( ENABLE_ATARI_PADDLE)
  static timed_t debounce_slot[ 2] = { 0};

#define RDD( I, P) button_debounce( debounce_slot + (I), !read_digital( P ))
  gamepad->fire1 |= RDD( 0, ATARI_PADDLE_FIRST_FIRE_PIN);
  gamepad->fire2 |= RDD( 1, ATARI_PADDLE_SECOND_FIRE_PIN);
#undef RDD
  gamepad->axis[0] = read_analog( ATARI_PADDLE_FIRST_ANGLE_PIN);
  gamepad->axis[1] = read_analog( ATARI_PADDLE_SECOND_ANGLE_PIN);
#endif // ENABLE_ATARI_PADDLE
}

static void process_atari_axis( gamepad_status_t* gamepad) {
#if defined( ENABLE_ATARI_PADDLE)
  static int16_t first_axis_history[10] = {0};
  static int16_t second_axis_history[10] = {0};

  // Moving average to reduce noise on the analog readinng
  gamepad->axis[0] = moving_average(first_axis_history,  10, gamepad->axis[0]);
  gamepad->axis[1] = moving_average(second_axis_history, 10, gamepad->axis[1]);

  // Axis calibration
  gamepad->axis[0] = (gamepad->axis[0] - 500) << 6;
  gamepad->axis[1] = (gamepad->axis[1] - 500) << 5;

  //// Debugging
  //gamepad->axis[0] = gamepad->axis[0] > 256 ? 32000 : -32000;
  //gamepad->axis[1] = gamepad->axis[1] > 256 ? 32000 : -32000;

#endif // ENABLE_ATARI_PADDLE
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

static void setup_snes(void){
#if defined( ENABLE_SNES)

  setup_output( SNES_CLOCK_PIN);
  write_digital(SNES_CLOCK_PIN, 1);

  setup_output( SNES_LATCH_PIN);
  write_digital(SNES_LATCH_PIN, 0);

  setup_output( SNES_DATA_PIN);
  write_digital(SNES_DATA_PIN, 1);
  setup_input( SNES_DATA_PIN, 1);
#endif // ENALBE_SNES
}

#if defined( ENABLE_SNES)
static int read_next_button_snes(int pin, int semiwait) {

  write_digital(pin, 0);
  delay_microsecond(semiwait);
  int result = !read_digital( SNES_DATA_PIN);
  write_digital(pin, 1);
  delay_microsecond(semiwait);
  return result;
}
#endif // ENALBE_SNES

static void read_snes( gamepad_status_t* gamepad) {
#if defined( ENABLE_SNES)

  write_digital(SNES_LATCH_PIN, 1);
  delay_microsecond(12);
  write_digital(SNES_LATCH_PIN, 0);
  delay_microsecond(6);

  gamepad->fire2 |=  read_next_button_snes( SNES_CLOCK_PIN, 6); // B
  gamepad->fire4 |=  read_next_button_snes( SNES_CLOCK_PIN, 6); // Y
  gamepad->select |= read_next_button_snes( SNES_CLOCK_PIN, 6);
  gamepad->start |=  read_next_button_snes( SNES_CLOCK_PIN, 6);
  gamepad->up |=     read_next_button_snes( SNES_CLOCK_PIN, 6);
  gamepad->down |=   read_next_button_snes( SNES_CLOCK_PIN, 6);
  gamepad->left |=   read_next_button_snes( SNES_CLOCK_PIN, 6);
  gamepad->right |=  read_next_button_snes( SNES_CLOCK_PIN, 6);
  gamepad->fire1 |=  read_next_button_snes( SNES_CLOCK_PIN, 6); // A
  gamepad->fire3 |=  read_next_button_snes( SNES_CLOCK_PIN, 6); // X
  gamepad->fire5 |=  read_next_button_snes( SNES_CLOCK_PIN, 6); // L
  gamepad->fire6 |=  read_next_button_snes( SNES_CLOCK_PIN, 6); // R
#endif // ENABLE_SNES
}

// dispatcher ---------------------------------------------------------------------

void usb_pad_encoder_init(){
  
  gamepad_init();
  setup_fullswitch();
  setup_atari_paddle();
  setup_snes();
  next_time_step();
  config_log();
}

void usb_pad_encoder_step(){
  static gamepad_status_t old_status = {0};

  next_time_step();

  gamepad_status_t gamepad = {0};
  // memset( &gamepad, sizeof( gamepad), 0);

  read_fullswitch( &gamepad);
  read_atari_paddle( &gamepad);
  read_snes( &gamepad);

  process_autofire( &gamepad);
  process_atari_axis( &gamepad);
  process_dpad( &gamepad);

  if (memcmp( &old_status, &gamepad, sizeof( gamepad)))
    gamepad_send(&gamepad);
  old_status = gamepad;
}

// --------------------------------------------------------------------------------
// Implementation guard ends
#endif // INCLUDE_IMPLEMENTATION
// --------------------------------------------------------------------------------

