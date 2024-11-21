#include <pico/stdlib.h>
#include "common/config.h"
#include "vga/hires_color_patterns.h"
#include "vga/hires_dot_patterns.h"
#include "vga/vgabuf.h"
#include "vga/render.h"
#include "vga/vgaout.h"

//#define PAGE2SEL (!(soft_switches & SOFTSW_80STORE) && (soft_switches & SOFTSW_PAGE_2))
#define PAGE2SEL ((soft_switches & (SOFTSW_80STORE | SOFTSW_PAGE_2)) == SOFTSW_PAGE_2)
uint16_t __attribute__((section(".uninitialized_data."))) lhalf_palette[16];

static void render_hires_line(bool p2, uint line);

static inline uint hires_line_to_mem_offset(uint line) {
    return ((line & 0x07) << 10) | ((line & 0x38) << 4) | (((line & 0xc0) >> 6) * 40);
}


void DELAYED_COPY_CODE(render_hires)() {
    for(uint line=0; line < 192; line++) {
        render_hires_line(PAGE2SEL, line);
    }
}


void DELAYED_COPY_CODE(render_mixed_hires)() {
    for(uint line=0; line < 160; line++) {
        render_hires_line(PAGE2SEL, line);
    }

    render_mixed_text();
}

static void DELAYED_COPY_CODE(render_hires_line)(bool p2, uint line) {
    struct vga_scanline *sl = vga_prepare_scanline();
    uint sl_pos = 0;

    const uint8_t *line_mem = (const uint8_t *)((p2 ? hgr_p2 : hgr_p1) + hires_line_to_mem_offset(line));

    // Pad 40 pixels on the left to center horizontally
    sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
    sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
    sl->data[sl_pos++] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 16 pixels per word

    uint32_t lastmsb = 0;
    uint32_t dots = 0;
    uint_fast8_t dotc = 0;
    uint32_t pixeldata;
    uint i;

    if(mono_rendering) {
        while(i < 40) {
            // Load in as many subpixels as possible
            dots |= (hires_dot_patterns2[lastmsb | line_mem[i]]) << dotc;
            lastmsb = (dotc>0) ? ((line_mem[i] & 0x40)<<2) : 0;
            i++;
            dotc += 14;

            // Consume pixels
            while(dotc) {
                pixeldata = ((dots & 1) ? (text_fore) : (text_back));
                dots >>= 1;
                pixeldata |= (((dots & 1) ? (text_fore) : (text_back))) << 16;
                dots >>= 1;
                sl->data[sl_pos++] = pixeldata;
                dotc -= 2;
            }
        }
    } else {
        // Each hires byte contains 7 pixels which may be shifted right 1/2 a pixel. That is
        // represented here by 14 'dots' to precisely describe the half-pixel positioning.
        //
        // For each pixel, inspect a window of 8 dots around the pixel to determine the
        // precise dot locations and colors.
        //
        // Dots would be scanned out to the CRT from MSB to LSB (left to right here):
        //
        //            previous   |        next
        //              dots     |        dots
        //        +-------------------+--------------------------------------------------+
        // dots:  | 31 | 30 | 29 | 28 | 27 | 26 | 25 | 24 | 23 | ... | 14 | 13 | 12 | ...
        //        |              |         |              |
        //        \______________|_________|______________/
        //                       |         |
        //                       \_________/
        //                         current
        //                          pixel
        uint oddness = 0;
        uint j;
            
        // Load in the first 14 dots
        dots |= (uint32_t)hires_dot_patterns[line_mem[0]] << 15;

        for(i=1; i < 41; i++) {
            // Load in the next 14 dots
            uint b = (i < 40) ? line_mem[i] : 0;
            if(b & 0x80) {
                // Extend the last bit from the previous byte
                dots |= (dots & (1u << 15)) >> 1;
            }
            dots |= (uint32_t)hires_dot_patterns[b] << 1;

            // Consume 14 dots
            for(uint j=0; j < 7; j++) {
                uint dot_pattern = oddness | ((dots >> 24) & 0xff);
                sl->data[sl_pos] = hires_color_patterns[dot_pattern];
                sl_pos++;
                dots <<= 2;
                oddness ^= 0x100;
            }
        }
    }

    // Pad 40 pixels on the right to center horizontally
    sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
    sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
    sl->data[sl_pos++] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 16 pixels per word

    sl->length = sl_pos;
    sl->repeat_count = 1;
    vga_submit_scanline(sl);
}
