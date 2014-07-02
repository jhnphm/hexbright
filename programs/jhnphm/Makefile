ARDUINO_DIR = /usr/share/arduino

ALTERNATE_CORE_PATH = $(ARDUINO_SKETCHBOOK)/hardware/hexbright
ARDUINO_VAR_PATH = $(ARDUINO_DIR)/hardware/arduino/variants

ARDUINO_PORT = /dev/ttyUSB0
ARDUINO_LIBS = hb_utilities EEPROM hexbright
BOARD_TAG = hexbright
ARDUINO_SKETCHBOOK = ../../
include ../../Arduino.mk
#$(shell mkdir -p ../../hardware/hexbright/variants; if [ ! -e "../../hardware/hexbright/variants/arduino:standard" ]; then ln -sf $(ARDUINO_DIR)/hardware/arduino/variants/standard/ "../../hardware/hexbright/variants/arduino:standard"; fi)

