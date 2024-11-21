#include <pico/stdlib.h>
#include <pico/multicore.h>
#include "common/config.h"
#include "z80/businterface.h"
#include "z80/z80buf.h"
#include "z80/z80rom.h"

#include "bsp/board.h"
#include "tusb.h"

// Used to ensure we interrupt the Z80 loop every 256 instructions
volatile uint8_t z80_cycle;

volatile uint32_t z80_vect = 0x000000;
volatile uint8_t __attribute__((section(".uninitialized_data."))) z80_irq;
volatile uint8_t __attribute__((section(".uninitialized_data."))) z80_nmi;
volatile uint8_t __attribute__((section(".uninitialized_data."))) z80_res;
volatile uint8_t __attribute__((section(".uninitialized_data."))) rom_shadow;
volatile uint8_t __attribute__((section(".uninitialized_data."))) ram_bank;
volatile uint8_t __attribute__((section(".uninitialized_data."))) ram_common;

volatile ctc_t ctc[4];
volatile uint8_t ctc_vector;

volatile sio_t sio[2];
volatile uint8_t sio_vector;

#define Z80break (z80_res || (config_cmdbuf[7] == 0) || (!z80_cycle++))

uint8_t DELAYED_COPY_CODE(zuart_read)(bool port) {
    uint8_t rv = 0;

    if(sio[port].datavalid) {
        rv = sio[port].data;
        sio[port].datavalid = 0;
    }

    switch(serialmux[port]) {
    default:
    case SERIAL_LOOP:
        break;
    case SERIAL_USB:
        if(tud_cdc_n_available(port)) {
            sio[port].data = tud_cdc_n_read_char(port);
            sio[port].datavalid = 1;
        }
        break;
#ifdef ANALOG_GS
    case SERIAL_UART:
        if(port) {
            if(uart_is_readable(uart1)) {
                sio[port].data = uart_getc(uart1);
                sio[port].datavalid = 1;
            }
        } else {
            if(uart_is_readable(uart0)) {
                sio[port].data = uart_getc(uart0);
                sio[port].datavalid = 1;
            }
        }
        break;
#endif
    }
    
    return rv;
}

uint8_t DELAYED_COPY_CODE(zuart_peek)(bool port) {
    uint8_t rv = 0;

    if(sio[port].datavalid) {
        rv = sio[port].data;
    }

    return rv;
}

uint8_t DELAYED_COPY_CODE(auart_read)(bool port) {
    zuart_read(port);
    return zuart_peek(port);
}

uint8_t DELAYED_COPY_CODE(auart_status)(bool port) {
    uint8_t rv;

    if(!sio[port].datavalid) {
        zuart_read(port);
    }

    rv = sio[port].datavalid ? 0x08 : 0x00;

    switch(serialmux[port]) {
    default:
    case SERIAL_LOOP:
        rv |= ((sio[port].control[5] & 0x02) ? 0x40 : 0x00) |
             ((sio[port].control[5] & 0x80) ? 0x20 : 0x00) |
             (sio[port].datavalid ? 0x00 : 0x10);
        break;
    case SERIAL_USB:
        rv |= ((tud_cdc_n_get_line_state(port) & 2) ? 0x00 : 0x40) | 
             (tud_cdc_n_connected(port) ? 0x00 : 0x20) |
             (tud_cdc_n_write_available(port) ? 0x10 : 0x00);
        break;
#ifdef ANALOG_GS
    case SERIAL_UART:
        if(port) {
            rv |= (uart_is_writable(uart1) ? 0x10 : 0x00);
        } else {
            rv |= (uart_is_writable(uart0) ? 0x10 : 0x00);
        }
        break;
#endif
    }

    return rv;
}

uint8_t DELAYED_COPY_CODE(auart_control)(bool port, uint8_t value) {
    return value;
}

uint8_t DELAYED_COPY_CODE(auart_command)(bool port, uint8_t value) {
    if(value & 0x1) {
        sio[port].control[5] |= 0x80;
    } else {
        sio[port].control[5] &= ~0x80;
    }
    if(value & 0xC) {
        sio[port].control[5] |= 0x02;
    } else {
        sio[port].control[5] &= ~0x02;
    }

    return value;
}

uint8_t DELAYED_COPY_CODE(zuart_write)(bool port, uint8_t value) {
    switch(serialmux[port]) {
    default:
    case SERIAL_LOOP:
        if(sio[port].datavalid) {
            sio[port].status[1] |= 0x20;
        }
        sio[port].datavalid = 1;
        sio[port].data = value;
        break;
    case SERIAL_USB:
        if(tud_cdc_n_write_available(port)) {
            tud_cdc_n_write_char(port, value);
        }
        break;
#ifdef ANALOG_GS
    case SERIAL_UART:
        if(port) {
            if(uart_is_writable(uart1)) {
                uart_putc(uart1, value);
            }
        } else {
            if(uart_is_writable(uart0)) {
                uart_putc(uart0, value);
            }
        }
        break;
#endif
    }
    return value;
}

uint8_t DELAYED_COPY_CODE(zuart_status)(bool port) {
    uint8_t rv = 0;

    
    switch(sio[port].control[0] & 0x7) {
    case 0:
        if(!sio[port].datavalid) {
            zuart_read(port);
        }

        rv = sio[port].datavalid ? 0x01 : 0x00;

        switch(serialmux[port]) {
        default:
        case SERIAL_LOOP:
            rv |= ((sio[port].control[5] & 0x02) ? 0x20 : 0x00) |
                 ((sio[port].control[5] & 0x80) ? 0x08 : 0x00) |
                 (sio[port].datavalid ? 0x00 : 0x04);
            break;
        case SERIAL_USB:
            rv |= ((tud_cdc_n_get_line_state(port) & 2) ? 0x20 : 0x00) | 
                 (tud_cdc_n_connected(port) ? 0x08 : 0x00) |
                 (tud_cdc_n_write_available(port) ? 0x04 : 0x00);
            break;
#ifdef ANALOG_GS
        case SERIAL_UART:
            if(port) {
                rv |= 0x20 | 
                     (uart_is_writable(uart1) ? 0x00 : 0x04);
            } else {
                rv |= 0x20 | 
                     (uart_is_writable(uart0) ? 0x00 : 0x04);
            }
            break;
#endif
        }
        break;
    case 1:
        rv = sio[(port)].status[1];
        break;
    case 2:
        if(port)
            rv = sio_vector;
        break;
    }
    sio[(port)].control[0] &= 0xF8;

    return rv;
}

uint8_t DELAYED_COPY_CODE(cpu_in)(uint16_t address) {
    uint8_t rv = 0;
    if(address & 0x80) {
        switch(address & 0xff) {
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
            if(ctc[address & 0x03].control & 0x40) {
                rv = ctc[address & 0x03].counter;
            } else {
            }
            break;
        case 0xFC:
        case 0xFE:
            rv = zuart_read(address & 0x02);
            break;
        case 0xFD:
        case 0xFF:
            rv = zuart_status(address & 0x02);
            break;
        }
    } else {
        switch(address & 0xe0) {
        case 0x00: // Write Data to 6502
            rv = pcpi_reg[0];
            break;
        case 0x20: // Read Data from 6502
            clr_z80_stat;
            rv = pcpi_reg[1];
            //printf("I%01X:%02X\r\n", (address >> 4), rv);
            break;
        case 0x40: // Status Port
            if(rd_z80_stat)
                rv |= 0x80;
            if(rd_6502_stat)
                rv |= 0x01;
            break;
        case 0x60:
            break;
        }
    }
    return rv;
}

void DELAYED_COPY_CODE(cpu_out)(uint16_t address, uint8_t value) {
    uint16_t divisor;
    if(address & 0x80) {
        switch(address & 0xff) {
        case 0x80:
        case 0x81:
        case 0x82:
        case 0x83:
            if(ctc[address & 0x03].control & 0x04) {
                ctc[address & 0x03].control &= ~0x06;

                ctc[address & 0x03].preload = value;
                if((address & 0x02) == 0) {
                    divisor = value ? value : 256;
                    sio[address & 0x01].baudrate = 115200 / divisor;
#ifdef ANALOG_GS
                    if(serialmux[(address & 0x01)] == SERIAL_UART) {
                        if(address & 0x01) {
                            uart_set_baudrate(uart1, sio[1].baudrate);
                        } else {
                            uart_set_baudrate(uart0, sio[0].baudrate);
                        }
                    }
#endif
                }
            } else if(value & 1) {
                ctc[address & 0x03].control = value;
            } else if((address & 0x3) == 0) {
                ctc_vector = value & 0xF8;
            }
            break;
        case 0xFC:
        case 0xFE:
            zuart_write((address & 0x02), value);
            break;
        case 0xFD:
        case 0xFF:
            if(((sio[(address & 0x01)].control[0] & 0x7) == 2) && (address & 0x01))
                sio_vector = value;

            sio[(address & 0x01)].control[sio[(address & 0x01)].control[0] & 0x7] = value;
            sio[(address & 0x01)].control[0] &= 0xF8;
            break;
        }
    } else {
        switch(address & 0xe0) {
        case 0x00: // Write Data to 6502
            //printf("O%01X:%02X\r\n", (address >> 4), value);
            pcpi_reg[0] = value;
            set_6502_stat;
            break;
        case 0x60:
            rom_shadow = (value & 1);
            break;
        case 0xC0:
            ram_bank = (value >> 1) & 7;
            ram_common = (value >> 6) & 1;
            break;
        }
    }
}

uint8_t DELAYED_COPY_CODE(_RamRead)(uint16_t address) {
    if((rom_shadow & 1) && (address < 0x8000))
        return z80_rom[address & 0x7ff];

    if((address > 0xE000) && (ram_common)) {
        return z80_ram[address];
    }

    if(ram_bank) {
        return 0xff;
    }

    return z80_ram[address];
}

void DELAYED_COPY_CODE(_RamWrite)(uint16_t address, uint8_t value) {
    if((rom_shadow & 1) && (address < 0x8000))
        return;

    if((address > 0xE000) && (ram_common)) {
        z80_ram[address] = value;
        return;
    }

    if(ram_bank) {
        return;
    }

    z80_ram[address] = value;
}

#include "z80cpu.h"

void DELAYED_COPY_CODE(z80main)() {
    z80_res = 1;

    board_init();
    tusb_init();
    
    for(;;) {
        if(!z80_cycle) {
            tud_task();
        }
        if(config_cmdbuf[7] == 0) {
            config_handler();
        } else
        if(cardslot != 0) {
            if(z80_res) {
                rom_shadow = 1;
                ram_bank = 0;
                ram_common = 0;

                z80_nmi = 0;
                z80_irq = 0;
                z80_res = 0;

                // 6502 -> Z80
                clr_z80_stat;

                // Z80 -> 6502
                clr_6502_stat;

                Z80reset();
            }

            Z80run();
        }
    }
}

