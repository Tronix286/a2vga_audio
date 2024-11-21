#include <pico/stdlib.h>
#include "common/config.h"
#include "vga/vgabuf.h"
#include "vga/render.h"
#include "vga/vgaout.h"

//#define PAGE2SEL (!(soft_switches & SOFTSW_80STORE) && (soft_switches & SOFTSW_PAGE_2))
#define PAGE2SEL ((soft_switches & (SOFTSW_80STORE | SOFTSW_PAGE_2)) == SOFTSW_PAGE_2)

volatile uint_fast32_t text_flasher_mask = 0;
static uint64_t next_flash_tick = 0;

void DELAYED_COPY_CODE(update_text_flasher)() {
    uint64_t now = time_us_64();
    if(now > next_flash_tick) {
        text_flasher_mask ^= 0xff;

        switch(current_machine) {
        default:
        case MACHINE_II:
        case MACHINE_PRAVETZ:
            next_flash_tick = now + 125000u;
            break;
        case MACHINE_IIE:
        case MACHINE_IIGS:
            next_flash_tick = now + 250000u;
            break;
        }
    }
}


static inline uint_fast8_t char_text_bits(uint_fast8_t ch, uint_fast8_t glyph_line) {
    uint_fast8_t bits, invert;

    if((soft_switches & SOFTSW_ALTCHAR) || (ch & 0x80)) {
        // normal / mousetext character
        invert = 0x00;
    } else {
        // flashing character or inverse character
        invert = (ch & 0x40) ? text_flasher_mask : 0x7f;
        ch = (ch & 0x3f) | 0x80;
    }

    bits = character_rom[((uint_fast16_t)ch << 3) | glyph_line];

    return (bits ^ invert) & 0x7f;
}

void DELAYED_COPY_CODE(render_text)() {
    uint line;

    if((internal_flags & IFLAGS_VIDEO7) && ((soft_switches & (SOFTSW_80STORE | SOFTSW_80COL | SOFTSW_DGR)) == (SOFTSW_80STORE | SOFTSW_DGR))) {
        for(line=0; line < 24; line++) {
            render_color_text40_line(line);
        }
    } else {
        if(soft_switches & SOFTSW_80COL) {
            for(line=0; line < 24; line++) {
                render_text80_line(PAGE2SEL, line);
            }
        } else {
            for(line=0; line < 24; line++) {
                render_text40_line(PAGE2SEL, line);
            }
        }
    }
}

void DELAYED_COPY_CODE(render_mixed_text)() {
    uint line;

    if((internal_flags & IFLAGS_VIDEO7) && ((soft_switches & (SOFTSW_80STORE | SOFTSW_80COL | SOFTSW_DGR)) == (SOFTSW_80STORE | SOFTSW_DGR))) {
        for(line=20; line < 24; line++) {
            render_color_text40_line(line);
        }
    } else {
        if(soft_switches & SOFTSW_80COL) {
            for(line=20; line < 24; line++) {
                render_text80_line(PAGE2SEL, line);
            }
        } else {
            for(line=20; line < 24; line++) {
                render_text40_line(PAGE2SEL, line);
            }
        }
    }
}

void DELAYED_COPY_CODE(render_text40_line)(bool p2, unsigned int line) {
    const uint8_t *page = (const uint8_t *)(p2 ? text_p2 : text_p1);
    const uint8_t *line_buf = (const uint8_t *)(page + ((line & 0x7) << 7) + (((line >> 3) & 0x3) * 40));

    for(uint glyph_line=0; glyph_line < 8; glyph_line++) {
        struct vga_scanline *sl = vga_prepare_scanline();
        uint sl_pos = 0;

        // Pad 40 pixels on the left to center horizontally
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 8 pixels per word

        for(uint col=0; col < 40; ) {
            // Grab 14 pixels from the next two characters
            uint32_t bits_a = char_text_bits(line_buf[col], glyph_line);
            col++;
            uint32_t bits_b = char_text_bits(line_buf[col], glyph_line);
            col++;

            uint32_t bits = (bits_a << 7) | bits_b;

            // Translate each pair of bits into a pair of pixels
            for(int i=0; i < 7; i++) {
                uint32_t pixeldata = (bits & 0x2000) ? (text_back|THEN_EXTEND_1) : (text_fore|THEN_EXTEND_1);
                pixeldata |= (bits & 0x1000) ?
                    ((uint32_t)text_back|THEN_EXTEND_1) << 16 :
                    ((uint32_t)text_fore|THEN_EXTEND_1) << 16;
                bits <<= 2;

                sl->data[sl_pos] = pixeldata;
                sl_pos++;
            }
        }

        // Pad 40 pixels on the right to center horizontally
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 8 pixels per word

        sl->length = sl_pos;
        sl->repeat_count = 1;
        vga_submit_scanline(sl);
    }
}

void DELAYED_COPY_CODE(render_text80_line)(bool p2, unsigned int line) {
    const uint8_t *page_a = (const uint8_t *)(p2 ? text_p2 : text_p1);
    const uint8_t *page_b = (const uint8_t *)(p2 ? text_p4 : text_p3);
    const uint8_t *line_buf_a = page_a + ((line & 0x7) << 7) + (((line >> 3) & 0x3) * 40);
    const uint8_t *line_buf_b = page_b + ((line & 0x7) << 7) + (((line >> 3) & 0x3) * 40);

    for(uint glyph_line=0; glyph_line < 8; glyph_line++) {
        struct vga_scanline *sl = vga_prepare_scanline();
        uint sl_pos = 0;

        // Pad 40 pixels on the left to center horizontally
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 8 pixels per word

        for(uint col=0; col < 40; ) {
            // Grab 14 pixels from the next two characters
            uint32_t bits_a = char_text_bits(line_buf_a[col], glyph_line);
            uint32_t bits_b = char_text_bits(line_buf_b[col], glyph_line);
            col++;

            uint32_t bits = (bits_b << 7) | bits_a;

            // Translate each pair of bits into a pair of pixels
            for(int i=0; i < 7; i++) {
                uint32_t pixeldata = (bits & 0x2000) ? (text_back) : (text_fore);
                pixeldata |= (bits & 0x1000) ?
                    ((uint32_t)text_back) << 16 :
                    ((uint32_t)text_fore) << 16;
                bits <<= 2;

                sl->data[sl_pos] = pixeldata;
                sl_pos++;
            }
        }

        // Pad 40 pixels on the right to center horizontally
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 8 pixels per word

        sl->length = sl_pos;
        sl->repeat_count = 1;
        vga_submit_scanline(sl);
    }
}

void DELAYED_COPY_CODE(render_color_text40_line)(unsigned int line) {
    const uint8_t *line_buf = (const uint8_t *)(text_p1 + ((line & 0x7) << 7) + (((line >> 3) & 0x3) * 40));
    const uint8_t *color_buf = (const uint8_t *)(text_p3 + ((line & 0x7) << 7) + (((line >> 3) & 0x3) * 40));

    for(uint glyph_line=0; glyph_line < 8; glyph_line++) {
        struct vga_scanline *sl = vga_prepare_scanline();
        uint sl_pos = 0;

        // Pad 40 pixels on the left to center horizontally
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 8 pixels per word

        for(uint col=0; col < 40; ) {
            // Grab 14 pixels from the next two characters
            uint32_t bits_a = char_text_bits(line_buf[col], glyph_line);
            uint16_t color1 = lores_palette[(color_buf[col] >> 4) & 0xf];
            uint16_t color2 = lores_palette[color_buf[col] & 0xf];
            col++;
            uint32_t bits_b = char_text_bits(line_buf[col], glyph_line);
            uint16_t color3 = lores_palette[(color_buf[col] >> 4) & 0xf];
            uint16_t color4 = lores_palette[color_buf[col] & 0xf];
            col++;

            uint32_t bits = (bits_a << 7) | bits_b;
            uint32_t pixeldata;

            // Translate each pair of bits into a pair of pixels
            for(int i=0; i < 3; i++) {
                pixeldata = (bits & 0x2000) ? (color2|THEN_EXTEND_1) : (color1|THEN_EXTEND_1);
                pixeldata |= (bits & 0x1000) ?
                    (uint32_t)(color2|THEN_EXTEND_1) << 16 :
                    (uint32_t)(color1|THEN_EXTEND_1) << 16;
                bits <<= 2;

                sl->data[sl_pos] = pixeldata;
                sl_pos++;
            }

            pixeldata = (bits & 0x2000) ? (color2|THEN_EXTEND_1) : (color1|THEN_EXTEND_1);
            pixeldata |= (bits & 0x1000) ?
                (uint32_t)(color4|THEN_EXTEND_1) << 16 :
                (uint32_t)(color3|THEN_EXTEND_1) << 16;
            bits <<= 2;

            sl->data[sl_pos] = pixeldata;
            sl_pos++;

            for(int i=4; i < 7; i++) {
                pixeldata = (bits & 0x2000) ? (color4|THEN_EXTEND_1) : (color3|THEN_EXTEND_1);
                pixeldata |= (bits & 0x1000) ?
                    (uint32_t)(color4|THEN_EXTEND_1) << 16 :
                    (uint32_t)(color3|THEN_EXTEND_1) << 16;
                bits <<= 2;

                sl->data[sl_pos] = pixeldata;
                sl_pos++;
            }
        }

        // Pad 40 pixels on the right to center horizontally
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 8 pixels per word

        sl->length = sl_pos;
        sl->repeat_count = 1;
        vga_submit_scanline(sl);
    }
}

void DELAYED_COPY_CODE(render_color_text80_line)(unsigned int line) {
    const uint8_t *line_buf_a = (const uint8_t *)(text_p1 + ((line & 0x7) << 7) + (((line >> 3) & 0x3) * 40));
    const uint8_t *line_buf_b = (const uint8_t *)(text_p3 + ((line & 0x7) << 7) + (((line >> 3) & 0x3) * 40));
    const uint8_t *color_buf_a = (const uint8_t *)(text_p2 + ((line & 0x7) << 7) + (((line >> 3) & 0x3) * 40));
    const uint8_t *color_buf_b = (const uint8_t *)(text_p4 + ((line & 0x7) << 7) + (((line >> 3) & 0x3) * 40));

    for(uint glyph_line=0; glyph_line < 8; glyph_line++) {
        struct vga_scanline *sl = vga_prepare_scanline();
        uint sl_pos = 0;

        // Pad 40 pixels on the left to center horizontally
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 8 pixels per word

        for(uint col=0; col < 40; ) {
            // Grab 14 pixels from the next two characters
            uint32_t bits_a = char_text_bits(line_buf_a[col], glyph_line);
            uint32_t bits_b = char_text_bits(line_buf_b[col], glyph_line);
            uint16_t color1 = lores_palette[color_buf_a[col] >> 4];
            uint16_t color2 = lores_palette[color_buf_a[col] & 0xF];
            uint16_t color3 = lores_palette[color_buf_b[col] >> 4];
            uint16_t color4 = lores_palette[color_buf_b[col] & 0xF];
            col++;

            uint32_t bits = (bits_b << 7) | bits_a;
            uint32_t pixeldata;

            // Translate each pair of bits into a pair of pixels
            for(int i=0; i < 3; i++) {
                pixeldata = (bits & 0x2000) ? (color2) : (color1);
                pixeldata |= (bits & 0x1000) ?
                    (uint32_t)(color2) << 16 :
                    (uint32_t)(color1) << 16;
                bits <<= 2;

                sl->data[sl_pos] = pixeldata;
                sl_pos++;
            }

            pixeldata = (bits & 0x2000) ? (color2) : (color1);
            pixeldata |= (bits & 0x1000) ?
                (uint32_t)(color4) << 16 :
                (uint32_t)(color3) << 16;
            bits <<= 2;

            sl->data[sl_pos] = pixeldata;
            sl_pos++;

            for(int i=4; i < 7; i++) {
                pixeldata = (bits & 0x2000) ? (color4) : (color3);
                pixeldata |= (bits & 0x1000) ?
                    (uint32_t)(color4) << 16 :
                    (uint32_t)(color3) << 16;
                bits <<= 2;

                sl->data[sl_pos] = pixeldata;
                sl_pos++;
            }
        }

        // Pad 40 pixels on the right to center horizontally
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl->data[sl_pos++] = (text_border|THEN_EXTEND_3) | ((text_border|THEN_EXTEND_3) << 16); // 8 pixels per word

        sl->length = sl_pos;
        sl->repeat_count = 1;
        vga_submit_scanline(sl);
    }
}

