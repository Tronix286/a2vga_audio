#include <pico/stdlib.h>
#include "common/config.h"
#include "vga/vgabuf.h"
#include "vga/render.h"
#include "vga/vgaout.h"

//#define PAGE2SEL (!(soft_switches & SOFTSW_80STORE) && (soft_switches & SOFTSW_PAGE_2))
#define PAGE2SEL ((soft_switches & (SOFTSW_80STORE | SOFTSW_PAGE_2)) == SOFTSW_PAGE_2)

uint8_t DELAYED_COPY_DATA(dgr_dot_pattern)[32] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x08, 0x19, 0x2A, 0x3B, 0x4C, 0x5D, 0x6E, 0x7F,
    0x00, 0x44, 0x08, 0x4C, 0x11, 0x55, 0x19, 0x5D,
    0x22, 0x66, 0x2A, 0x6E, 0x33, 0x77, 0x3B, 0x7F,
};

extern uint8_t lores_to_dhgr[16];
uint8_t dhgr_to_lores[16] = {
    0,2,4,6,8,10,12,14,1,3,5,7,9,11,13,15
};

static void render_dgr_line(bool p2, uint line);

void DELAYED_COPY_CODE(render_dgr)() {
    for(uint line=0; line < 24; line++) {
        render_dgr_line(PAGE2SEL, line);
    }
}


void DELAYED_COPY_CODE(render_mixed_dgr)() {
    for(uint line=0; line < 20; line++) {
        render_dgr_line(PAGE2SEL, line);
    }

    render_mixed_text();
}


static void DELAYED_COPY_CODE(render_dgr_line)(bool p2, uint line) {
    // Construct two scanlines for the two different colored cells at the same time
    struct vga_scanline *sl1 = vga_prepare_scanline();
    struct vga_scanline *sl2 = vga_prepare_scanline();
    uint sl_pos = 0;
    uint i, j;
    uint32_t color1, color2;
    uint_fast8_t dotc = 0;
    uint32_t pixeldata;

    const uint8_t *line_bufa = (const uint8_t *)((p2 ? text_p2 : text_p1) + ((line & 0x7) << 7) + (((line >> 3) & 0x3) * 40));
    const uint8_t *line_bufb = (const uint8_t *)((p2 ? text_p4 : text_p3) + ((line & 0x7) << 7) + (((line >> 3) & 0x3) * 40));

    // Pad 40 pixels on the left to center horizontally
    sl1->data[sl_pos] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
    sl2->data[sl_pos] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
    sl_pos++;
    sl1->data[sl_pos] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
    sl2->data[sl_pos] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
    sl_pos++;
    sl1->data[sl_pos] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 8 pixels per word
    sl2->data[sl_pos] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 8 pixels per word
    sl_pos++;

    i = 0;
    color1 = 0;
    color2 = 0;
    if(mono_rendering) {
        while(i < 40) {
            while((dotc <= 14) && (i < 40)) {
                color1 |= dgr_dot_pattern[((i & 1) << 4) | (line_bufb[i] & 0xf)] << dotc;
                color2 |= dgr_dot_pattern[((i & 1) << 4) | ((line_bufb[i] >> 4) & 0xf)] << dotc;
                dotc += 7;
                color1 |= dgr_dot_pattern[((i & 1) << 4) | (line_bufa[i] & 0xf)] << dotc;
                color2 |= dgr_dot_pattern[((i & 1) << 4) | ((line_bufa[i] >> 4) & 0xf)] << dotc;
                dotc += 7;
                i++;
            }

            // Consume pixels
            while(dotc >= 2) {
                pixeldata = ((color1 & 1) ? (text_fore) : (text_back));
                pixeldata |= (((color1 & 2) ? (text_fore) : (text_back))) << 16;
                sl1->data[sl_pos] = pixeldata;

                pixeldata = ((color2 & 1) ? (text_fore) : (text_back));
                pixeldata |= (((color2 & 2) ? (text_fore) : (text_back))) << 16;
                sl2->data[sl_pos] = pixeldata;

                color1 >>= 2;
                color2 >>= 2;
                sl_pos++;
                dotc -= 2;
            }
        }
    } else {
        while(i < 40) {
            while((dotc <= 14) && (i < 40)) {
                color1 |= dgr_dot_pattern[((i & 1) << 4) | (line_bufb[i] & 0xf)] << dotc;
                color2 |= dgr_dot_pattern[((i & 1) << 4) | ((line_bufb[i] >> 4) & 0xf)] << dotc;
                dotc += 7;
                color1 |= dgr_dot_pattern[((i & 1) << 4) | (line_bufa[i] & 0xf)] << dotc;
                color2 |= dgr_dot_pattern[((i & 1) << 4) | ((line_bufa[i] >> 4) & 0xf)] << dotc;
                dotc += 7;
                i++;
            }

            // Consume pixels
            while((dotc >= 8) || ((dotc > 0) && (i == 40))) {
                color1 &= 0xfffffffe;
                color1 |= (color1 >> 4) & 1;
                pixeldata = dhgr_palette[color1 & 0xf];
                color1 &= 0xfffffffc;
                color1 |= (color1 >> 4) & 3;
                pixeldata |= dhgr_palette[color1 & 0xf] << 16;
                sl1->data[sl_pos] = pixeldata;

                color2 &= 0xfffffffe;
                color2 |= (color2 >> 4) & 1;
                pixeldata = dhgr_palette[color2 & 0xf];
                color2 &= 0xfffffffc;
                color2 |= (color2 >> 4) & 3;
                pixeldata |= dhgr_palette[color2 & 0xf] << 16;
                sl2->data[sl_pos] = pixeldata;
                
                sl_pos++;

                color1 &= 0xfffffff8;
                color1 |= (color1 >> 4) & 7;
                pixeldata = dhgr_palette[color1 & 0xf];
                color1 >>= 4;
                pixeldata |= dhgr_palette[color1 & 0xf] << 16;
                sl1->data[sl_pos] = pixeldata;

                color2 &= 0xfffffff8;
                color2 |= (color2 >> 4) & 7;
                pixeldata = dhgr_palette[color2 & 0xf];
                color2 >>= 4;
                pixeldata |= dhgr_palette[color2 & 0xf] << 16;
                sl2->data[sl_pos] = pixeldata;

                sl_pos++;
                dotc -= 4;
            }
        }
    }

    // Pad 40 pixels on the right to center horizontally
    sl1->data[sl_pos] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
    sl2->data[sl_pos] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
    sl_pos++;
    sl1->data[sl_pos] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
    sl2->data[sl_pos] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
    sl_pos++;
    sl1->data[sl_pos] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 8 pixels per word
    sl2->data[sl_pos] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 8 pixels per word
    sl_pos++;


    sl1->length = sl_pos;
    sl1->repeat_count = 7;
    vga_submit_scanline(sl1);

    sl2->length = sl_pos;
    sl2->repeat_count = 7;
    vga_submit_scanline(sl2);
}
