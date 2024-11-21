#!/bin/bash

if [ -z ${PICO_SDK_PATH:+x} ]; then
  echo You must set PICO_SDK_PATH to the path where you downloaded https://github.com/raspberrypi/pico-sdk.git
  exit 1
fi

build_firmware() {
  if [ ! -d build/$1 ]; then
    ( mkdir -p build/$1 && cd build/$1 && cmake ../.. )
  fi
  make -C build/$1
  #tools/mksparse build/$1/$1.bin build/$1/$1.ota
}

build_firmware v2-analog-lc-4ns-z80
build_firmware v2-analog-lc-8ns-z80
build_firmware v2-analog-lc-4ns-vga
build_firmware v2-analog-lc-8ns-vga
build_firmware v2-analog-wifi-4ns-z80
build_firmware v2-analog-wifi-8ns-z80
build_firmware v2-analog-wifi-4ns-vga
build_firmware v2-analog-wifi-8ns-vga
build_firmware v2-analog-gs-4ns-z80
build_firmware v2-analog-gs-8ns-z80
build_firmware v2-analog-gs-4ns-vga
