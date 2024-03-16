// This .ino file is just a boilerplate.
// All the code is in the .h files.

#include "usb_pad_encoder.h"

#include <Arduino.h>

#include "HID.h"
#if !defined(_USING_HID)
#error "Arduino board does not support PluggubleHID module"
#endif

//#define DEBUG

// for debug/logging only
#define SERIAL_BPS 9600 // e.g. 32u4
#define SERIAL_EOL "\n\r"

#if defined(DEBUG) || defined(SIMULATION_MODE)
#define USE_SERIAL
#endif

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

static unsigned long get_elasped_microsecond(){
  return micros();
}

static void delay_microsecond(unsigned long us){
  delayMicroseconds(us);
}

static void setup_input( uint8_t p, uint8_t d){
  switch(d){
    case 0: pinMode( p, INPUT);
    case 1: pinMode( p, INPUT_PULLUP);
  }
}

static void setup_output( uint8_t p){
  pinMode( p, OUTPUT);
}

static int read_digital( uint8_t p){
  return digitalRead( p);
}

static int read_analog( uint8_t p){
  return analogRead( p);
}

static void write_digital( uint8_t p, uint8_t v){
  return digitalWrite( p, v);
}

static void use_hid_descriptor( uint8_t* desc, size_t len){
  static HIDSubDescriptor node( desc, len);
  HID().AppendDescriptor(&node);
}

static void send_hid_report( int id, void* data, size_t len){
  HID().SendReport( id, data, len);
}

static void setup_first() {

#ifdef USE_SERIAL
  Serial.begin(SERIAL_BPS);
#endif
}

static void setup_last() {

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

#ifdef LED_BUILTIN_TX
  // the bootloader continously turn on the annoying leds, shutdown
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(LED_BUILTIN_RX, HIGH);
  digitalWrite(LED_BUILTIN_TX, HIGH);
#endif // LED_BUILTIN_TX
}

void setup() {
  setup_first();
  usb_pad_encoder_init();
  setup_last();
}

void loop(){
  loop_first();
  usb_pad_encoder_step();
}

// PROGMEM = store the data in the flash/program memory instead of SRAM; this is
// needed by the HID library.
#define HID_DESCRIPTOR_ATTRIBUTE PROGMEM

#define INCLUDE_IMPLEMENTATION
#include "usb_pad_encoder.h"

