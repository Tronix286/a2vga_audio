#include <string.h>
#include <pico/stdlib.h>
#include <hardware/timer.h>
#include "common/config.h"
#include "common/flash.h"
#include "common/dmacopy.h"
#include "vga/vgabuf.h"
#include "vga/render.h"
#include "vga/vgaout.h"
#include "audio.h"

uint16_t text_fore;
uint16_t text_back;
uint16_t text_border;

compat_t machinefont = MACHINE_INVALID;
bool userfont = false;
bool mono_rendering = false;

uint16_t DELAYED_COPY_DATA(mono_colors)[14] = {
    _RGB(0x00, 0x00, 0x00), _RGB(0xFF, 0xFF, 0xFF), // White Normal
    _RGB(0xFF, 0xFF, 0xFF), _RGB(0x00, 0x00, 0x00), // White Inverse
    _RGB(0x00, 0x00, 0x00), _RGB(0xFE, 0x7F, 0x00), // Amber Normal
    _RGB(0xFE, 0x7F, 0x00), _RGB(0x00, 0x00, 0x00), // Amber Inverse
    _RGB(0x00, 0x00, 0x00), _RGB(0x00, 0xBF, 0x00), // Green Normal
    _RGB(0x00, 0xBF, 0x00), _RGB(0x00, 0x00, 0x00), // Green Inverse
    _RGB(0x35, 0x28, 0x79), _RGB(0x6C, 0x5E, 0xB5), // Commodore
};

// Initialize the character generator ROM
static void DELAYED_COPY_CODE(switch_font)() {
    if(romx_changed) {
        memcpy32(character_rom, (void*)FLASH_FONT(romx_textbank), 4096);
    } else if(userfont) {
        return;
    } else if(current_machine != machinefont) {
        if (romx_textbank==0)
        switch(current_machine) {
        default:
        case MACHINE_II:
            memcpy32(character_rom, (void*)FLASH_FONT_APPLE_II, 4096);
            break;
        case MACHINE_IIE:
            memcpy32(character_rom, (void*)FLASH_FONT_APPLE_IIE, 4096);
            break;
        case MACHINE_IIGS:
            memcpy32(character_rom, (void*)FLASH_FONT_APPLE_IIGS, 4096);
            break;
        case MACHINE_PRAVETZ:
            memcpy32(character_rom, (void*)FLASH_FONT_PRAVETZ, 4096);
            break;
        }
        machinefont = current_machine;
    }
}

uint16_t status_timeout = 900;
uint8_t status_line[81];

void DELAYED_COPY_CODE(update_status_right)(const char *str) {
    uint i, len;

    if(str != NULL) {
        len = strlen(str);
    } else {
        len = 0;
    }

    if(len < 80) {
        memset(status_line, ' ', 80 - len);
    } else {
        len = 80;
    }

    for(i = 0; i < len; i++) {
        status_line[(80-len) + i] = str[i];
    }

    status_timeout = 900;
}

void DELAYED_COPY_CODE(update_status_left)(const char *str) {
    uint i, len;

    if(str != NULL) {
        len = strlen(str);
    } else {
        len = 0;
    }

    if(len < 80) {
        memset(status_line + len, ' ', 80 - len);
    } else {
        len = 80;
    }

    for(i = 0; i < len; i++) {
        status_line[i] = str[i];
    }

    status_timeout = 900;
}

void DELAYED_COPY_CODE(render_init)() {
    switch_font();

    // Initialize "Half-Palette" for aperture grill effect
    for(int i = 1; i < 15; i++) {
        half_palette[i] = (dhgr_palette[i] >> 1) & _RGBHALF;
        lhalf_palette[i] = (lores_palette[i] >> 1) & _RGBHALF;
    }
    half_palette[0] = dhgr_palette[0];
    half_palette[15] = dhgr_palette[15];
    lhalf_palette[0] = lores_palette[0];
    lhalf_palette[15] = lores_palette[15];

    if((soft_switches & SOFTSW_MODE_MASK) == 0)
        internal_flags |= IFLAGS_TEST;

    memcpy(terminal_character_rom, (void*)FLASH_VIDEX_BASE, 4096);
    memset(status_line, 0, sizeof(status_line));

    terminal_clear_screen();

    render_test_init();
}

// Skip lines to center vertically or blank the screen
void DELAYED_COPY_CODE(render_border)(uint count) {
    struct vga_scanline *sl = vga_prepare_scanline();
    uint sl_pos = 0;

    while(sl_pos < VGA_WIDTH/16) {
        sl->data[sl_pos] = (text_border|THEN_EXTEND_7) | ((text_border|THEN_EXTEND_7) << 16); // 16 pixels per word
        sl_pos++;
    }

    sl->length = sl_pos;
    sl->repeat_count = count - 1;
    vga_submit_scanline(sl);
}

uint32_t screentimeout = 0;
uint32_t testdone = 0;

void DELAYED_COPY_CODE(render_loop)() {
    for(;;) {
        config_handler();
        //audioUpdate();
#if 0
        if((busactive == 0) && (screentimeout > (15 * 60))) {
            vga_prepare_frame();
            render_border(VGA_HEIGHT);
            memset(status_line, 0, sizeof(status_line));
            status_timeout = 0;
            vga_dpms_sleep();
            while(busactive == 0);
            vga_dpms_wake();
        } else {
            if(busactive == 0) {
                screentimeout++;
                if(screentimeout == 30) {
                    update_status_right("Going to sleep...");
                }
            } else {
                if(screentimeout >= 30) {
                    // Clear the sleep mode message
                    memset(status_line, 0, sizeof(status_line));
                    status_timeout = 0;
                }
                screentimeout = 0;
            }
            busactive = 0;
#endif

            if(romx_changed || (machinefont != current_machine)) {
                switch_font();
                romx_changed = 0;
                machinefont = current_machine;
            }

            update_text_flasher();

            if(!(mono_palette & 0x8)) {
                if((current_machine == MACHINE_IIGS) && !(soft_switches & SOFTSW_MONOCHROME)) {
                    text_fore = lores_palette[APPLE_FORE];
                    text_back = lores_palette[APPLE_BACK];
                    text_border = lores_palette[APPLE_BORDER];
                } else {
                    text_fore = mono_colors[1];
                    text_back = mono_colors[0];
                    text_border = mono_colors[0];
                }
            } else if(mono_palette == 0xF) {
                text_fore = lores_palette[TERMINAL_FORE];
                text_back = lores_palette[TERMINAL_BACK];
                text_border = lores_palette[TERMINAL_BORDER];
            } else {
                int palette = mono_palette & 0x7;
                text_fore = mono_colors[palette*2+1];
                text_back = mono_colors[palette*2];
                text_border = (palette == 0x6) ? text_fore : text_back;
            }
            
            mono_rendering = ((soft_switches & SOFTSW_MONOCHROME) || (mono_palette & 0x8));

            if(internal_flags & IFLAGS_TEST) {
                render_testpattern();
                // Assume the RP2040 has been hard reset and try to default to text display
                if(busactive && (testdone == 0)) { // was ((soft_switches & SOFTSW_MODE_MASK) != 0)
                    soft_switches |= SOFTSW_TEXT_MODE;
                    internal_flags &= ~IFLAGS_TEST;
                    testdone = 1;
                    render_about_init();
                }
#if defined(ANALOG_GS)
            } else if(soft_switches & SOFTSW_SHR) {
                vga_prepare_frame();
                render_shr();
#endif
            } else if(internal_flags & IFLAGS_TERMINAL) {
                vga_prepare_frame();
                render_terminal();
            } else {
                vga_prepare_frame();

                render_border(16);
                if(status_line[0] != 0) {
                    render_status_line();
                } else {
                    render_border(32);
                }

                switch(soft_switches & SOFTSW_MODE_MASK) {
                case 0:
                    if(soft_switches & SOFTSW_DGR) {
                        render_dgr();
                    } else {
                        render_lores();
                    }
                    break;
                case SOFTSW_MIX_MODE:
                    if((soft_switches & (SOFTSW_80COL | SOFTSW_DGR)) == (SOFTSW_80COL | SOFTSW_DGR)) {
                        render_mixed_dgr();
                    } else {
                        render_mixed_lores();
                    }
                    break;
                case SOFTSW_HIRES_MODE:
                    if(soft_switches & SOFTSW_DGR) {
                        render_dhgr();
                    } else {
                        render_hires();
                    }
                    break;
                case SOFTSW_HIRES_MODE|SOFTSW_MIX_MODE:
                    if((soft_switches & (SOFTSW_80COL | SOFTSW_DGR)) == (SOFTSW_80COL | SOFTSW_DGR)) {
                        render_mixed_dhgr();
                    } else {
                        render_mixed_hires();
                    }
                    break;
                default:
                    render_text();
                    break;
                }

                render_border(48);
            }
#if 0
        }
#endif
    }
}
