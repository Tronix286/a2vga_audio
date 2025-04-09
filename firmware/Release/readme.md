## Firmwares
There are several firmware options for this board:
| Firmware | Description | Preferred slot number |
| -------- | ----------- | --------------------- |
| v2-analog-preloader.uf2 <br /> v2-analog-lc-4ns-vga.uf2| [A2VGA](https://github.com/V2RetroComputing/analog-firmware) + mockingboard, board work with both features together. First flash *v2-analog-preloader.uf2* it will initialize the fonts and the necessary default values, then after that flash the main *v2-analog-lc-4ns-vga.uf2* firmware.| Slot #4 for MB; <br /> any if software allows you to choose MB-slot №|
| applevga.uf2 | Original [Apple II VGA](https://github.com/markadev/AppleII-VGA) firmware modified for this PCB. Burn applevga.uf2. Its NOT support Mockingboard, VGA output only. But if the Videx 80 column card is installed this firmware can display 80x25 text mode (PR#3) and the previous firmware (a2vga+mockingboard) can't do it.| any|
|A2USB-MOUSE-NTSC-A2VGA-4ns-v1.3.uf2 <br /> A2USB-MOUSE-PAL-A2VGA-4ns-v1.3.uf2| [Apple II USB Interface & Apple II Mouse Interface Card](https://github.com/ThorstenBr/A2USB) modified for this PCB. This firmware support mouse emulation only. No VGA output and no mockingboard emulation.| Slot #4 |
