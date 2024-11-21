#include "common/config.h"
#include "vga/vgabuf.h"

volatile uint32_t mono_palette = 0;

// The currently programmed character generator ROM for text mode
uint8_t __attribute__((section(".uninitialized_data."))) character_rom[4096];
uint8_t __attribute__((section(".uninitialized_data."))) terminal_character_rom[4096];

uint8_t __attribute__((section(".uninitialized_data."))) terminal_fifo[256];
uint8_t terminal_fifo_wrptr = 0;
uint8_t terminal_fifo_rdptr = 0;

volatile uint8_t terminal_row = 0;
volatile uint8_t terminal_col = 0;

volatile uint8_t terminal_tbcolor = 0xF0;
volatile uint8_t terminal_border = 0x0;

volatile uint8_t romx_type = 0;
volatile uint8_t romx_unlocked = 0;
volatile uint8_t romx_textbank = 0;
volatile uint8_t romx_changed = 0;

