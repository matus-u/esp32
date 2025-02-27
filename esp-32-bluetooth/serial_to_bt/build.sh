#!/bin/bash -
#===============================================================================
#
#          FILE: build.sh
#
#         USAGE: ./build.sh
#
#   DESCRIPTION: 
#
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: YOUR NAME (), 
#  ORGANIZATION: 
#       CREATED: 05.03.2024 08:50:20
#      REVISION:  ---
#===============================================================================

set -o nounset                                  # Treat unset variables as an error


mkdir -p /tmp/arduino_build

/home/matus/programovanie/szilard/camera/arduino-1.8.13/arduino-builder -compile -logger=machine -hardware /home/matus/programovanie/szilard/camera/arduino-1.8.13/hardware -hardware /home/matus/.arduino15/packages -tools /home/matus/programovanie/szilard/camera/arduino-1.8.13/tools-builder -tools /home/matus/programovanie/szilard/camera/arduino-1.8.13/hardware/tools/avr -tools /home/matus/.arduino15/packages -built-in-libraries /home/matus/programovanie/szilard/camera/arduino-1.8.13/libraries -libraries /home/matus/Arduino/libraries -fqbn=esp32:esp32:esp32cam:CPUFreq=240,FlashMode=qio,FlashFreq=80 -ide-version=10813 -build-path /tmp/arduino_build -warnings=none -build-cache /tmp/arduino_cache_148384 -prefs=build.warn_data_percentage=75 -prefs=runtime.tools.esptool_py.path=/home/matus/.arduino15/packages/esp32/tools/esptool_py/4.9.dev3 -prefs=runtime.tools.esptool_py-4.9.dev3.path=/home/matus/.arduino15/packages/esp32/tools/esptool_py/4.9.dev3 -prefs=runtime.tools.xtensa-esp32-elf-gcc.path=/home/matus/.arduino15/packages/esp32/tools/xtensa-esp32-elf-gcc/1.22.0-97-gc752ad5-5.2.0 -prefs=runtime.tools.xtensa-esp32-elf-gcc-1.22.0-97-gc752ad5-5.2.0.path=/home/matus/.arduino15/packages/esp32/tools/xtensa-esp32-elf-gcc/1.22.0-97-gc752ad5-5.2.0 -prefs=runtime.tools.mkspiffs.path=/home/matus/.arduino15/packages/esp32/tools/mkspiffs/0.2.3 -prefs=runtime.tools.mkspiffs-0.2.3.path=/home/matus/.arduino15/packages/esp32/tools/mkspiffs/0.2.3 -verbose /home/matus/programovanie/szilard/camera/esp32/esp-32-bluetooth/serial_to_bt/serial_to_bt.ino
