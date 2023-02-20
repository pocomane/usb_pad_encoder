#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#include "usb_pad_encoder.h"

#define LOG(C, F, ...) do{ if( C) printf( "%s:%d " F "\n", __FILE__, __LINE__, __VA_ARGS__); fflush( stdout);} while(0)

#define PROGMEM

enum {
  LOW = 0,
  HIGH = 1,
  OUTPUT,
  INPUT_PULLUP,

};

static char* input = 0;
unsigned long elapsed_us = 0;

static void setup_input( uint8_t p, uint8_t d){
}

static void setup_output( uint8_t p){
}

static unsigned long get_elasped_microsecond(){
  LOG( 1, "elapsed us %ld", elapsed_us);
  return elapsed_us;
}

static void delay_microsecond(unsigned long us){
  LOG( 1, "delay us %ld", us);
  elapsed_us += us;
}

static int read_digital( uint8_t p){
  int v = 0;
  if( input && *input){
    int r = sscanf( input, "%d", &v);
    if( r >0) input += r;
  }
  LOG( 1, "read %d <- %d", p, v);
  return v;
}

static int read_analog( uint8_t p){
  return read_digital( p);
}

static void write_digital( uint8_t p, uint8_t v){
  LOG( 1, "write %d -> %d", p, v);
}

static void use_hid_descriptor( const uint8_t* desc, size_t len){
  LOG( 1, "usb hid descriptor (size %ld)", len);
}

static void send_hid_report( int id, void* data, size_t len){
  gamepad_log( data);
}

int main(){
  input = "1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1";
  usb_pad_encoder_init();
  while( input && *input){
    elapsed_us += 100;
    usb_pad_encoder_step();
  }
  printf("Test succeeded!\n");
}

#define INCLUDE_IMPLEMENTATION
#include "usb_pad_encoder.h"

