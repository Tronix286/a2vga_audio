# a2vga_audio
 Apple II vga + Mockingboard emulation board at the same time as the vga output.
 This project based on [Mark Aikens Apple II VGA](https://github.com/markadev/AppleII-VGA/) and [A2Analog](https://github.com/V2RetroComputing/analog)

 ![a2vga_card](docs/photo_2024-11-20_12-01-19.jpg)

 ## Hardware differences between existing A2VGA type cards
 
I wanted to add Mockingboard emulation functionality to the A2VGA-like board at the same time as the vga output. The first problem that needed to be solved was to free up several GPIOs for the right and left audio channels as well as for /IRQ control. As you know, in the standard A2VGA circuit all GPIOs are occupied, and I needed at least three free GPIOs. In the standard circuit 9 GPIOs are used to output the color code:

<img width="670" height="431" alt="image" src="https://github.com/user-attachments/assets/96a0e337-3783-45ed-8804-83e1ebc054c0" />

I thought that it would be possible to use two additional latches - one with store on the positive edge, the other on the negative edge clock signal. The result was the following diagram:

<img width="1261" height="705" alt="image" src="https://github.com/user-attachments/assets/67b4e5f9-726e-4fdf-962e-7bdf8d63d635" />

First 4 bits latched in HC373 at positive edge, then second 4 bit (with previos 4 bit from HC373) latched in HC574 at negative edge together with last 9 bit in GPIO18 with help side-set RP2040 PIO. Thus, 3 free GPIOs are released! Naturally, this circuit required changes to the firmware, in particular, doubling the vga-PIOs operating frequency and recalculating some timings in PIO blocks. If previously PIOs worked at a pixel clock frequency of ~25 MHz, then when sequentially loading values ​​into two registers, a frequency twice as high is required - 50 MHz. This is most likely almost the maximum frequency for the 74hc IC series, so it is better to use something faster, for example, the 74AC series. In my case I used what I had on hand, namely 74HC374 and 74AC574:

<img width="1280" height="720" alt="image" src="https://github.com/user-attachments/assets/dcc8f671-07e9-4cb9-b2e9-a1898a1cd462" />

The second problem is the lack of processor time to emulate two AY-chips at 44100Hz stereo PWM simultaneously with vga output, especially in high-resolution mode. If Pi Pico used to work at a frequency of 252Mhz, then I had to overclock the rp2040 to 378mhz. Fortunately, I've done this many times before and everything was fine. For audio output, two GPIOs are used with a low-pass filter according to a typical scheme:

<img width="592" height="464" alt="image" src="https://github.com/user-attachments/assets/c981b3f0-90a5-4328-a095-72374aeb4bda" />

For proper Mockingboard emulation we also need drive/IRQ, so i used  [MacFly's](https://www.applefritter.com/macfly) a2vga mouse circuit (with different GPIO):

<img width="831" height="326" alt="image" src="https://github.com/user-attachments/assets/aa4c3b82-1ead-4ec1-ad1b-5a2a027af1ed" />

## Build notes

- Firmware for GAL16V8 you can download from [∀2 Retro Computing](https://github.com/V2RetroComputing/analog), so direct link for source code is [picopal](https://github.com/V2RetroComputing/picopal-v1/blob/dbde4b1a4bb3d2647ef39bca39bba3fad3cf1425/picopal.pld); direct link to JED file is: [picopal.jed](firmware/PicoPal.jed)
- Do not use firmware that is not specifically designed for this board. It is necessary to make sure that the pipico is flashed with the firmware from this page from the [this section](https://github.com/Tronix286/a2vga_audio/issues/firmware/Release) . First flash [v2-analog-preloader.uf2](https://github.com/Tronix286/a2vga_audio/blob/main/firmware/Release/v2-analog-preloader.uf2) then burn main a2vga+audio firmware [v2-analog-lc-4ns-vga.uf2](https://github.com/Tronix286/a2vga_audio/blob/main/firmware/Release/v2-analog-lc-4ns-vga.uf2). Also you can try [applevga.uf2](https://github.com/Tronix286/a2vga_audio/blob/main/firmware/Release/applevga.uf2) - this firmware dosnt support mockingboard, but compiled for this PCB exactly.
- For proper color decoding very important use "fast" logic IC for U8 (74xx574), U9 (74xx373). Original a2vga PIOs worked at a pixel clock frequency of ~25 MHz, then when sequentially loading values ​​into two registers in my board, a frequency twice as high is required - 50 MHz(!). This is most likely almost the maximum frequency for the 74HC IC series, so it is better to use something faster, for example, the 74AC or 74ACT series. Sometimes the Chinese re-mark the chips, so if you order them from AliExpress, it is not a fact that the marking corresponds to the real chip. They can sell HC as LS marking or vice versa. Therefore, original fast 3.3V chips are needed, HC (not sure) or AC or ACT series. In my case I used what I had on hand, namely IN74HC373 and IN74AC574 combination.
