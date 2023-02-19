#!/bin/sh

SCRDIR=$(readlink -f $(dirname "$0"))

# ---------------------------------------------------------------------------------
# configuration

BUILDPROP=""
BUILDPROP="$BUILDPROP --build-property build.vid=0x1209"

BUILDPROP="$BUILDPROP --build-property build.pid=0x0001"
BUILDPROP="$BUILDPROP --build-property build.usb_manufacturer=\"Ardupad\""
BUILDPROP="$BUILDPROP --build-property build.usb_product=\"ArdupadOne\""

# ---------------------------------------------------------------------------------

set -x # print commands before execution
set -e # automatic exit on error
rm -fR build/

## Arduino toolchain installation
arduino-cli core install arduino:avr
arduino-cli lib install Keyboard
arduino-cli config init --overwrite
# arduino-cli config set library.enable_unsafe_install false

SKETCH_DIR="$SCRDIR"
SKETCH_NAME=$(basename "$SCRDIR")

# Prepare directory
#rm -fR "$SKETCH_DIR"/build
mkdir -p "$SKETCH_DIR"/build
cat > "$SKETCH_DIR"/"$SKETCH_NAME".ino <<EOF
// This .ino file is just a boilerplate.
// All the code is in the .cpp file.
EOF

# Compile
arduino-cli compile $BUILDPROP -b arduino:avr:micro "$SKETCH_DIR" --build-path="$SKETCH_DIR"/build

# Upload via USB
#arduino-cli upload -p /dev/ttyACM0 -b arduino:avr:micro -i "$SKETCH_DIR"/build/"$SKETCH_NAME".ino.with_bootloader.bin
arduino-cli upload -p /dev/ttyACM0 -b arduino:avr:micro "$SKETCH_DIR" --input-dir "$SKETCH_DIR"/build

# Upload via programmer (usbasp)
# sudo arduino-cli burn-bootloader -P usbasp -b arduino:avr:micro
# sudo arduino-cli upload -P usbasp -b arduino:avr:micro -i "$SKETCH_DIR"/build/"$SKETCH_NAME".ino.with_bootloader.bin

# show logs from arduino
# Note: too much serial message can prevent the rom to be flashed; in such case
#       you can start minicom, stop it with ctrl-a (that stops the serial messages)
#       and then start the flashing from another console.
# minicom -o -b 9600 -D /dev/ttyACM0


