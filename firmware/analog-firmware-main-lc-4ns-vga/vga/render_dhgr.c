#include <pico/stdlib.h>
#include "hires_color_patterns.h"
#include "hires_dot_patterns.h"
#include "common/config.h"
#include "vga/vgabuf.h"
#include "vga/render.h"
#include "vga/vgaout.h"
#include "vga/dhgr_patterns.h"

static void render_dhgr_line(bool p2, uint line, bool mono);

uint16_t DELAYED_COPY_DATA(dhgr_palette)[16] = {
    RGB_BLACK,    RGB_DBLUE,    RGB_DGREEN,    RGB_HBLUE,
    RGB_BROWN,    RGB_LGRAY,    RGB_HGREEN,    RGB_AQUA,
    RGB_MAGENTA,  RGB_HVIOLET,  RGB_DGRAY,     RGB_LBLUE,
    RGB_HORANGE,  RGB_PINK,     RGB_YELLOW,    RGB_WHITE
};
uint16_t __attribute__((section(".uninitialized_data."))) half_palette[16];

//#define PAGE2SEL (!(soft_switches & SOFTSW_80STORE) && (soft_switches & SOFTSW_PAGE_2))
#define PAGE2SEL ((soft_switches & (SOFTSW_80STORE | SOFTSW_PAGE_2)) == SOFTSW_PAGE_2)


static inline uint dhgr_line_to_mem_offset(uint line) {
    return ((line & 0x07) << 10) | ((line & 0x38) << 4) | (((line & 0xc0) >> 6) * 40);
}


void DELAYED_COPY_CODE(render_dhgr)() {
    bool mono = mono_rendering;
    if((internal_flags & IFLAGS_VIDEO7) && ((internal_flags & IFLAGS_V7_MODE3) == IFLAGS_V7_MODE0)) {
        mono = true;
    }
    for(uint line=0; line < 192; line++) {
        render_dhgr_line(PAGE2SEL, line, mono);
    }
}

void DELAYED_COPY_CODE(render_mixed_dhgr)() {
    bool mono = mono_rendering;
    if((internal_flags & IFLAGS_VIDEO7) && ((internal_flags & IFLAGS_V7_MODE3) == IFLAGS_V7_MODE0)) {
        mono = true;
    }
    for(uint line=0; line < 160; line++) {
        render_dhgr_line(PAGE2SEL, line, mono);
    }

    render_mixed_text();
}

static void DELAYED_COPY_CODE(render_dhgr_line)(bool p2, uint line, bool mono) {
    struct vga_scanline *sl = vga_prepare_scanline();
    uint sl_pos = 0;
    uint i;

    const uint8_t *line_mema = (const uint8_t *)((p2 ? hgr_p2 : hgr_p1) + dhgr_line_to_mem_offset(line));
    const uint8_t *line_memb = (const uint8_t *)((p2 ? hgr_p4 : hgr_p3) + dhgr_line_to_mem_offset(line));

    if((internal_flags & IFLAGS_VIDEO7) && ((internal_flags & IFLAGS_V7_MODE3) == IFLAGS_V7_MODE2)) {
        // Pad 0 pixels on the left to center horizontally
    } else {
        // Pad 40 pixels on the left to center horizontally
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 16 pixels per word
    }

    // DHGR is weird. Video-7 just makes it weirder. Nuff said.
    uint32_t dots = 0;
    uint_fast8_t dotc = 0;
    uint32_t pixeldata;
    uint32_t pixelmode = 0;
    uint16_t white_pixel_count = 0;
    uint32_t color1, color2, color3, color4;

    i = 0;
    if(mono) {
        while(i < 40) {
            // Load in as many subpixels as possible
            while((dotc < 28) && (i < 40)) {
                dots |= (line_memb[i] & 0x7f) << dotc;
                dotc += 7;
                dots |= (line_mema[i] & 0x7f) << dotc;
                dotc += 7;
                i++;
            }

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
    } else if((internal_flags & IFLAGS_VIDEO7) && ((soft_switches & (SOFTSW_80STORE | SOFTSW_80COL)) == (SOFTSW_80STORE))) {
        int j;

        // Video 7 F/B HiRes
        while(i < 40) {
            dots = (line_mema[i] & 0x7f);
            color1 = lores_palette[(line_memb[i] >> 4) & 0xF];
            color2 = lores_palette[(line_memb[i] >> 0) & 0xF];
            i++;

            dots |= (line_mema[i] & 0x7f) << 7;
            color3 = lores_palette[(line_memb[i] >> 4) & 0xF];
            color4 = lores_palette[(line_memb[i] >> 0) & 0xF];
            i++;

            for(j = 0; j < 3; j++) {
                pixeldata = ((dots & 1) ? (color1) : (color2)) | THEN_EXTEND_1;
                dots >>= 1;
                pixeldata |= (((dots & 1) ? (color1) : (color2)) | THEN_EXTEND_1) << 16;
                dots >>= 1;
                sl->data[sl_pos++] = pixeldata;
            }

            pixeldata = ((dots & 1) ? (color1) : (color2)) | THEN_EXTEND_1;
            dots >>= 1;
            pixeldata |= (((dots & 1) ? (color3) : (color4)) | THEN_EXTEND_1) << 16;
            dots >>= 1;
            sl->data[sl_pos++] = pixeldata;

            for(j = 0; j < 3; j++) {
                pixeldata = ((dots & 1) ? (color3) : (color4)) | THEN_EXTEND_1;
                dots >>= 1;
                pixeldata |= (((dots & 1) ? (color3) : (color4)) | THEN_EXTEND_1) << 16;
                dots >>= 1;
                sl->data[sl_pos++] = pixeldata;
            }
        }
    } else if((internal_flags & IFLAGS_VIDEO7) && ((internal_flags & IFLAGS_V7_MODE3) == IFLAGS_V7_MODE2)) {
        // 160x192 Video-7 
        while(i < 40) {
            // Load in as many subpixels as possible
            while((dotc <= 18) && (i < 40)) {
                dots |= (line_memb[i] & 0xff) << dotc;
                dotc += 8;
                dots |= (line_mema[i] & 0xff) << dotc;
                dotc += 8;
                i++;
            }

            // Consume pixels
            while(dotc >= 8) {
                pixeldata = (lores_palette[dots & 0xf] | THEN_EXTEND_3);
                dots >>= 4;
                pixeldata |= (lores_palette[dots & 0xf] | THEN_EXTEND_3) << 16;
                dots >>= 4;
                sl->data[sl_pos++] = pixeldata;
                dotc -= 8;
            }
        }
    } else if((internal_flags & (IFLAGS_VIDEO7 | IFLAGS_V7_MODE3)) == (IFLAGS_VIDEO7 | IFLAGS_V7_MODE1)) {
        // Video-7 Mixed B&W/RGB
        while(i < 40) {
            // Load in as many subpixels as possible
            while((dotc <= 18) && (i < 40)) {
                dots |= (line_memb[i] & 0x7f) << dotc;
                pixelmode |= ((line_memb[i] & 0x80) ? 0x7f : 0x00) << dotc;
                dotc += 7;
                dots |= (line_mema[i] & 0x7f) << dotc;
                pixelmode |= ((line_mema[i] & 0x80) ? 0x7f : 0x00) << dotc;
                dotc += 7;
                i++;
            }

            // Consume pixels
            while(dotc >= 4) {
                if(pixelmode) {
                    pixeldata = (dhgr_palette[dots & 0xf] | THEN_EXTEND_1);
                    pixeldata |= pixeldata << 16;
                    dots >>= 4;
                    pixelmode >>= 4;
                    sl->data[sl_pos++] = pixeldata;
                    dotc -= 4;
                } else {
                    pixeldata = ((dots & 1) ? (text_fore) : (text_back));
                    dots >>= 1;
                    pixelmode >>= 1;
                    pixeldata |= (((dots & 1) ? (text_fore) : (text_back))) << 16;
                    dots >>= 1;
                    pixelmode >>= 1;
                    sl->data[sl_pos++] = pixeldata;
                    dotc -= 2;
                    pixeldata = ((dots & 1) ? (text_fore) : (text_back));
                    dots >>= 1;
                    pixelmode >>= 1;
                    pixeldata |= (((dots & 1) ? (text_fore) : (text_back))) << 16;
                    dots >>= 1;
                    pixelmode >>= 1;
                    sl->data[sl_pos++] = pixeldata;
                    dotc -= 2;
                }
            }
        }
    } else if((internal_flags & (IFLAGS_INTERP | IFLAGS_GRILL)) == (IFLAGS_INTERP | IFLAGS_GRILL)) {
        // Preload black into the sliding window
        dots = 0;
        dotc = 4;

        while(i < 40) {
            // Load in as many subpixels as possible
            while((dotc <= 18) && (i < 40)) {
                dots |= (line_memb[i] & 0x7f) << dotc;
                dotc += 7;
                dots |= (line_mema[i] & 0x7f) << dotc;
                dotc += 7;
                i++;
            }

            while((dotc >= 8) || ((dotc > 0) && (i == 40))) {
                dots &= 0xfffffffe;
                dots |= (dots >> 4) & 1;
                pixeldata = half_palette[dots & 0xf];
                dots &= 0xfffffffc;
                dots |= (dots >> 4) & 3;
                pixeldata |= dhgr_palette[dots & 0xf] << 16;
                sl->data[sl_pos++] = pixeldata;

                dots &= 0xfffffff8;
                dots |= (dots >> 4) & 7;
                pixeldata = half_palette[dots & 0xf];
                dots >>= 4;
                pixeldata |= dhgr_palette[dots & 0xf] << 16;
                sl->data[sl_pos++] = pixeldata;

                dotc -= 4;
            }
        }
    } else if(internal_flags & IFLAGS_INTERP) {
        // Preload black into the sliding window
        dots = 0;
        dotc = 4;

        while(i < 40) {
            // Load in as many subpixels as possible
            while((dotc <= 18) && (i < 40)) {
                dots |= (line_memb[i] & 0x7f) << dotc;
                dotc += 7;
                dots |= (line_mema[i] & 0x7f) << dotc;
                dotc += 7;
                i++;
            }

            while((dotc >= 8) || ((dotc > 0) && (i == 40))) {
                dots &= 0xfffffffe;
                dots |= (dots >> 4) & 1;
                pixeldata = dhgr_palette[dots & 0xf];
                dots &= 0xfffffffc;
                dots |= (dots >> 4) & 3;
                pixeldata |= dhgr_palette[dots & 0xf] << 16;
                sl->data[sl_pos++] = pixeldata;

                dots &= 0xfffffff8;
                dots |= (dots >> 4) & 7;
                pixeldata = dhgr_palette[dots & 0xf];
                dots >>= 4;
                pixeldata |= dhgr_palette[dots & 0xf] << 16;
                sl->data[sl_pos++] = pixeldata;

                dotc -= 4;
            }
        }
    } else {
        while(i < 40) {
            // Load in as many subpixels as possible
            while((dotc <= 18) && (i < 40)) {
                dots |= (line_memb[i] & 0x7f) << dotc;
                dotc += 7;
                dots |= (line_mema[i] & 0x7f) << dotc;
                dotc += 7;
                i++;
            }

            // Consume pixels
            while(dotc >= 8) {
                pixeldata = (dhgr_palette[dots & 0xf] | THEN_EXTEND_3);
                dots >>= 4;
                pixeldata |= (dhgr_palette[dots & 0xf] | THEN_EXTEND_3) << 16;
                dots >>= 4;
                sl->data[sl_pos++] = pixeldata;
                dotc -= 8;
            }
        }
    }

    if((internal_flags & IFLAGS_VIDEO7) && ((internal_flags & IFLAGS_V7_MODE3) == IFLAGS_V7_MODE2)) {
        // Pad 0 pixels on the right to center horizontally
    } else {
        // Pad 40 pixels on the right to center horizontally
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 16 pixels per word
    }

    sl->length = sl_pos;
    sl->repeat_count = 1;
    vga_submit_scanline(sl);
}

