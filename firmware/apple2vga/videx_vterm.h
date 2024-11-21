#pragma once

#include <stdint.h>
#include <stdbool.h>


extern volatile bool videx_vterm_enabled;
extern volatile bool videx_vterm_80col_enabled;
extern volatile bool videx_vterm_mem_selected;

extern void videx_vterm_init();
extern void videx_vterm_enable();
extern void videx_vterm_disable();
extern void videx_vterm_shadow_register(bool is_write, uint_fast16_t address, uint_fast8_t data);
extern void videx_vterm_shadow_c8xx(bool is_write, uint_fast16_t address, uint_fast8_t value);

extern void videx_vterm_update_flasher();
extern void render_videx_text();
