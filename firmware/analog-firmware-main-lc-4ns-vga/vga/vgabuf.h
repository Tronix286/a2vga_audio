#pragma once

#include <stdint.h>
#include "common/buffers.h"

#define text_memory text_p1
#define hires_memory hgr_p1
#define terminal_memory (private_memory+0xF000)

extern uint8_t character_rom[4096];
extern uint8_t terminal_character_rom[4096];

extern uint8_t terminal_fifo[256];
extern uint8_t terminal_fifo_wrptr;
extern uint8_t terminal_fifo_rdptr;

extern volatile uint8_t terminal_row;
extern volatile uint8_t terminal_col;

#define APPLE_FORE       ((apple_tbcolor>>4) & 0xf)
#define APPLE_BACK       (apple_tbcolor & 0xf)
#define APPLE_BORDER     (apple_border & 0xf)

extern volatile uint32_t mono_palette;
extern volatile uint8_t terminal_tbcolor;
extern volatile uint8_t terminal_border;

#define TERMINAL_FORE       ((terminal_tbcolor>>4) & 0xf)
#define TERMINAL_BACK       (terminal_tbcolor & 0xf)
#define TERMINAL_BORDER     (terminal_border & 0xf)

extern volatile uint8_t romx_type;
extern volatile uint8_t romx_unlocked;
extern volatile uint8_t romx_textbank;
extern volatile uint8_t romx_changed;
