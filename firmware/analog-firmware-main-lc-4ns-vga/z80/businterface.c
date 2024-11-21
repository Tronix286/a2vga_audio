#include <string.h>
#include <hardware/pio.h>
#include "common/config.h"
#include "common/buffers.h"
#include "common/abus.h"
#include "z80/businterface.h"
#include "z80/z80buf.h"

volatile uint8_t *pcpi_reg = apple_memory + 0xC0C0;

static inline void __time_critical_func(pcpi_read)(uint32_t address) {
    switch(address & 0xF) {
    case 0x0: // Read Data from Z80
        clr_6502_stat;
        break;
    case 0x5: // Z80 Reset
        z80_res = 1;
        break;
    case 0x6: // Z80 INT
        //z80_irq = 1;
        break;
    case 0x7: // Z80 NMI
        z80_nmi = 1;
        break;
    case 0x8: // 6551 A Data Register
        pcpi_reg[0x08] = auart_read(0);
        pcpi_reg[0x09] = auart_status(0);
        break;
    case 0x9: // 6551 A Status Register
        pcpi_reg[0x08] = zuart_peek(0);
        pcpi_reg[0x09] = auart_status(0);
        break;
    case 0xA: // 6551 A Command Register
        break;
    case 0xB: // 6551 A Control Register
        break;
    case 0xC: // 6551 B Data Register
        pcpi_reg[0x0C] = auart_read(1);
        pcpi_reg[0x0D] = auart_status(1);
        break;
    case 0xD: // 6551 B Status Register
        pcpi_reg[0x0C] = zuart_peek(1);
        pcpi_reg[0x0D] = auart_status(1);
        break;
    case 0xE: // 6551 B Command Register
        break;
    case 0xF: // 6551 B Control Register
        break;
    }
}


static inline void __time_critical_func(pcpi_write)(uint32_t address, uint32_t value) {
    switch(address & 0xF) {
    case 0x1: // Write Data to Z80
        pcpi_reg[1] = value & 0xff;
        set_z80_stat;
        break;
    case 0x5: // Z80 Reset
    case 0x6: // Z80 INT
    case 0x7: // Z80 NMI
        pcpi_read(address);
        break;
    case 0x8: // 6551 A Data Register
        zuart_write(0, value);
        pcpi_reg[0x08] = zuart_peek(0);
        pcpi_reg[0x09] = auart_status(0);
        break;
    case 0x9: // 6551 A Reset Register
        pcpi_reg[0x08] = zuart_peek(0);
        pcpi_reg[0x09] = auart_status(0);
        break;
    case 0xA: // 6551 A Command Register
        pcpi_reg[0x0A] = auart_command(0, value);
        break;
    case 0xB: // 6551 A Control Register
        pcpi_reg[0x0B] = auart_control(0, value);
        break;
    case 0xC: // 6551 B Data Register
        zuart_write(1, value);
        pcpi_reg[0x0C] = zuart_peek(1);
        pcpi_reg[0x0D] = auart_status(1);
        break;
    case 0xD: // 6551 B Reset Register
        pcpi_reg[0x0C] = zuart_peek(1);
        pcpi_reg[0x0D] = auart_status(1);
        break;
    case 0xE: // 6551 B Command Register
        pcpi_reg[0x0E] = auart_command(1, value);
        break;
    case 0xF: // 6551 B Control Register
        pcpi_reg[0x0F] = auart_control(1, value);
        break;
    }
}

void __time_critical_func(z80_businterface)(uint32_t address, uint32_t value) {
    volatile uint8_t *new_pcpi_reg;

    // Reset the Z80 when the Apple II resets
    if(reset_state == 3) z80_res = 1;

    // Shadow parts of the Apple's memory by observing the bus write cycles
    if(CARD_SELECT) {
        if(CARD_DEVSEL) {
            // Ideally this code should only run once.
            new_pcpi_reg = apple_memory + (address & 0xFFF0);
            if((uint32_t)new_pcpi_reg != (uint32_t)pcpi_reg) {
                memcpy((void*)new_pcpi_reg, (void*)pcpi_reg, 16);
                pcpi_reg = new_pcpi_reg;
            }

            if((value & (1u << CONFIG_PIN_APPLEBUS_RW-CONFIG_PIN_APPLEBUS_DATA_BASE)) == 0) {
                pcpi_write(address, value);
            } else {
                pcpi_read(address);
            }
        }
    }
}

