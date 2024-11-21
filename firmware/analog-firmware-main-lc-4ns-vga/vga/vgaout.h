#pragma once

#include <stdint.h>
#include "vga9.pio.h"

#define VGA_WIDTH 640
#define VGA_HEIGHT 480

#ifdef ANALOG_GS
#define THEN_WAIT_VSYNC (2 << 12)
#define THEN_WAIT_HSYNC (3 << 12)
#define THEN_EXTEND_7   (4 << 12)
#define THEN_EXTEND_6   (5 << 12)
#define THEN_EXTEND_3   (6 << 12)
#define THEN_EXTEND_1   (7 << 12)
#else
#define THEN_WAIT_VSYNC (vga_data_offset_wait_vsync << 9)
#define THEN_WAIT_HSYNC (vga_data_offset_wait_hsync << 9)
#define THEN_EXTEND_7   (vga_data_offset_extend_7 << 9)
#define THEN_EXTEND_6   (vga_data_offset_extend_6 << 9)
#define THEN_EXTEND_3   (vga_data_offset_extend_3 << 9)
#define THEN_EXTEND_1   (vga_data_offset_extend_1 << 9)
#endif

struct vga_scanline {
    // number of 32-bit words in the data array
    uint_fast16_t length;

    // number of times to repeat the scanline
    uint_fast16_t repeat_count;

    volatile uint_fast8_t _flags;

    uint32_t _sync;
    uint32_t data[(VGA_WIDTH/2)+8];
};

extern void vga_prepare_frame();
extern struct vga_scanline *vga_prepare_scanline();
extern struct vga_scanline *vga_prepare_scanline_quick();
extern void vga_submit_scanline(struct vga_scanline *scanline);

extern void vga_stop();
extern void vga_dpms_sleep();
extern void vga_dpms_wake();

extern void terminal_process_input();
