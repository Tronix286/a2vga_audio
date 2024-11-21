#pragma once

#include <stdint.h>

// Uncomment to enable test patter generator
#define RENDER_TEST_PATTERN

extern uint16_t lhalf_palette[16];
extern uint16_t half_palette[16];
extern uint16_t lores_palette[16];
extern uint16_t dhgr_palette[16];

extern uint16_t text_fore, text_back, text_border;
extern uint8_t status_line[81];
extern bool mono_rendering;

extern void terminal_clear_screen();

extern void update_status_left(const char *str);
extern void update_status_right(const char *str);

extern void render_init();
extern void render_loop();

extern void render_testpattern();
extern void render_test_init();
extern void render_about_init();
extern void render_test_sleep();

extern void update_text_flasher();
extern void render_text();
extern void render_mixed_text();
extern void render_text40_line(bool p2, unsigned int line);
extern void render_text80_line(bool p2, unsigned int line);
extern void render_color_text40_line(unsigned int line);
extern void render_color_text80_line(unsigned int line);
extern void render_status_line();

extern void render_terminal();

extern void render_border(uint count);

extern void render_lores();
extern void render_mixed_lores();

extern void render_hires();
extern void render_mixed_hires();

extern void render_dhgr();
extern void render_mixed_dhgr();

extern void render_dgr();
extern void render_mixed_dgr();

extern void render_shr();

extern volatile uint_fast32_t text_flasher_mask;

extern void vga_init();
extern void vga_deinit();

#ifdef ANALOG_GS
#define _RGB(r, g, b) ( \
    (((((uint)(r) * 256 / 18) + 255) / 256) << 8) | \
    (((((uint)(g) * 256 / 18) + 255) / 256) << 4) | \
    ((((uint)(b) * 256 / 18) + 255) / 256) \
)
#define _RGBHALF 0x777
#else
#define _RGB(r, g, b) ( \
    (((((uint)(r) * 256 / 36) + 128) / 256) << 6) | \
    (((((uint)(g) * 256 / 36) + 128) / 256) << 3) | \
    ((((uint)(b) * 256 / 36) + 128) / 256) \
)
#define _RGBHALF 0x0DB
#endif

#define RGB_BLACK   _RGB(0x00,0x00,0x00)
#define RGB_MAGENTA _RGB(0x6c,0x00,0x6c)
#define RGB_DBLUE   _RGB(0x00,0x00,0xb4)
#define RGB_HVIOLET _RGB(0xb4,0x24,0xfc)
#define RGB_DGREEN  _RGB(0x00,0x48,0x00)
#define RGB_DGRAY   _RGB(0x48,0x48,0x48)
#define RGB_HBLUE   _RGB(0x00,0x90,0xfc)
#define RGB_LBLUE   _RGB(0x6c,0x6c,0xfc)
#define RGB_BROWN   _RGB(0x24,0x24,0x00)
#define RGB_HORANGE _RGB(0xfc,0x48,0x00)
#define RGB_LGRAY   _RGB(0x90,0x90,0x90)
#define RGB_PINK    _RGB(0xfc,0x6c,0xfc)
#define RGB_HGREEN  _RGB(0x00,0xd8,0x24)
#define RGB_YELLOW  _RGB(0xd8,0xd8,0x00)
#define RGB_AQUA    _RGB(0x90,0xfc,0xb4)
#define RGB_WHITE   _RGB(0xff,0xff,0xff)
