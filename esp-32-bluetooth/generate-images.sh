#!/bin/bash

#if [ $# != "1" ]; then
#    echo "You need to provide arduino build number"
#    exit 1
#fi

rm images.zip

zip -j images.zip /home/matus/.arduino15/packages/esp32/hardware/esp32/1.0.6/tools/partitions/boot_app0.bin /home/matus/.arduino15/packages/esp32/hardware/esp32/1.0.6/tools/sdk/bin/bootloader_qio_80m.bin /tmp/arduino_build/serial_to_bt.ino.bin /tmp/arduino_build/serial_to_bt.ino.partitions.bin flash-from-cmdline
