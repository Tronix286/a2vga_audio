#pragma once

#include <stdint.h>
#include "vga.pio.h"


#define VGA_WIDTH 640
#define VGA_HEIGHT 480


/*#define THEN_WAIT_VSYNC (2 << 9)
#define THEN_WAIT_HSYNC (3 << 9)
#define THEN_EXTEND_1 (8 << 9)
#define THEN_EXTEND_2 (7 << 9)
#define THEN_EXTEND_3 (6 << 9)
#define THEN_EXTEND_6 (5 << 9)
#define THEN_EXTEND_7 (4 << 9)
*/
#define THEN_WAIT_VSYNC (vga_data_offset_wait_vsync << 9)
#define THEN_WAIT_HSYNC (vga_data_offset_wait_hsync << 9)
#define THEN_EXTEND_7   (vga_data_offset_extend_7 << 9)
#define THEN_EXTEND_6   (vga_data_offset_extend_6 << 9)
#define THEN_EXTEND_3   (vga_data_offset_extend_3 << 9)
#define THEN_EXTEND_1   (vga_data_offset_extend_1 << 9)


struct vga_scanline {
    // number of 32-bit words in the data array
    uint_fast16_t length;

    // number of times to repeat the scanline
    uint_fast16_t repeat_count;

    volatile uint_fast8_t _flags;

    uint32_t _sync;
    uint32_t data[(VGA_WIDTH / 2) + 8];
};


void vga_init();

void vga_prepare_frame();
struct vga_scanline *vga_prepare_scanline();
void vga_submit_scanline(struct vga_scanline *scanline);
void vga_skip_lines(unsigned int n);
