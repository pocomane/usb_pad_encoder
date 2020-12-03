
# SNES pad to USB with Arduino

This is a Arduino sketch to read a SNES pad input and convert it to USB HID events.

# Auto-fire

The sketch has an auto-fire assist feature enabled. With a small change you can
turn it off or use a more classic auto-fire mode. The two modes supported are
the following.

In the ASSIST mode to start an autofire sequence you have to tap three time one
of the A/B/X/Y button and then hold it down. The auto-fire will continue until
you release the button.

In the TOGGLE mode, if you press and release SELECT while one of the A/B/X/Y
button is down it will enable and disable the auto-fire for that button.

# Compile and flash

It should be easy to compile and flash the sketch using the Arduino IDE. For
some IDE version you could need to install the the joystick library:
https://github.com/MHeironimus/ArduinoJoystickLibrary

# Build an adaptor

TODO : write instructions !

# Configure auto-fire mode

The sketch can be configured to enable two kind of auto-fire. You have just to
set the `AUTOFIRE_MODE` macro at the beginning of the file to `NONE`, `ASSIST`
or `TOGGLE`.

In the `NONE` mode, no auto-fire is generated, like the original SNES pad.

For all the other modes you can use the `AUTOFIRE_PERIOD` macro to change the
time between two taps generated by the auto-fire system.

If you selected the `ASSIST` mode, you can use the `TAP_MAX_PERIOD` macros to
set the maximum time between real taps to accept. Moreover, changing the
`AUTOFIRE_TAP_COUNT` macro you can set the number of consecutive taps to wait
before start the auto-fire sequence.

If you selected the `TOGGLE` mode, you can use the `AUTOFIRE_SELECTOR` to chage
the button to use as toggle in the `TOGGLE` mode (the default is `SELECT`)

# Configure Arduino USB name

To change the name that arduino uses to identify itself on USB, you have to
change the `avr/boards.txt` file in the IDE setup folder. E.g., if you have an
Arduino Micro board:

```
...

## original ardunio USB VID/PID/Name
#micro.build.vid=0x2341
#micro.build.pid=0x8037
#micro.build.usb_product="Arduino Micro"

# Open source project USB VID - https://pid.codes/1209
# (test USB PID from 0x0000 to 0x010)
micro.build.vid=0x1209
micro.build.pid=0x0001
#micro.build.usb_product="Player 1 Input (arduino)"
micro.build.usb_product="SNES USB pad (arduino)"

...
```

This for example is usefull if you want to connect different kind of
arduino-powered to the same computer and have it to remember setups for each of
them.

