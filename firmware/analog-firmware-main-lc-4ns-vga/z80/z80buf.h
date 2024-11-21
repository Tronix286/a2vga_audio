#pragma once

#include <stdint.h>
#include "common/buffers.h"

extern volatile uint32_t z80_vect;
extern volatile uint8_t z80_irq;
extern volatile uint8_t z80_nmi;
extern volatile uint8_t z80_res;
extern uint8_t z80_rom[2*1024];
#define z80_ram private_memory
extern volatile uint8_t *pcpi_reg;

#define clr_z80_stat { pcpi_reg[2] &= ~0x80; }
#define set_z80_stat { pcpi_reg[2] |= 0x80; }
#define rd_z80_stat (pcpi_reg[2] >> 7)

#define clr_6502_stat { pcpi_reg[3] &= ~0x80; }
#define set_6502_stat { pcpi_reg[3] |= 0x80; }
#define rd_6502_stat (pcpi_reg[3] >> 7)

typedef struct ctc_s {
    uint8_t control;
    uint8_t counter;
    uint8_t preload;
} ctc_t;

extern volatile ctc_t ctc[4];
extern volatile uint8_t ctc_vector;

typedef struct sio_s {
    uint8_t control[8];
    uint8_t status[2];
    uint8_t data;
    uint8_t datavalid;
    uint32_t baudrate;
} sio_t;

extern volatile sio_t sio[2];
extern volatile uint8_t sio_vector;

extern uint8_t auart_read(bool port);
extern uint8_t zuart_peek(bool port);
extern uint8_t auart_status(bool port);
extern uint8_t auart_command(bool port, uint8_t value);
extern uint8_t auart_control(bool port, uint8_t value);
extern uint8_t zuart_write(bool port, uint8_t value);
