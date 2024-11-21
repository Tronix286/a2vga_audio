#include <string.h>
#include <hardware/pio.h>
#include "common/config.h"
#include "common/abus.h"
#include "vga/businterface.h"
#include "vga/vgabuf.h"
#include "vga/render.h"

uint8_t lores_to_dhgr[16] = {
    0x0, 0x8,
    0x1, 0x9,
    0x2, 0xA,
    0x3, 0xB,
    0x4, 0xC,
    0x5, 0xD,
    0x6, 0xE,
    0x7, 0xF
};

volatile uint8_t *terminal_page = terminal_memory;

void __time_critical_func(vga_businterface)(uint32_t address, uint32_t value) {
#ifdef ANALOG_GS
    // Apple IIgs: Bit 27 is GS jumper closed, Bit 26 is M2B0
    if(value & 0x4000000) {
        if(ACCESS_WRITE) {
            private_memory[address] = value & 0xff;
        }
        return;
    }
#endif

    // Shadow parts of the Apple's memory by observing the bus write cycles
    if(ACCESS_WRITE) {
        // Mirror Video Memory from MAIN & AUX banks
        if(soft_switches & SOFTSW_80STORE) {
            if(soft_switches & SOFTSW_PAGE_2) {
                if((address >= 0x400) && (address < 0x800)) {
                    private_memory[address] = value & 0xff;
                    return;
                } else if((soft_switches & SOFTSW_HIRES_MODE) && (address >= 0x2000) && (address < 0x4000)) {
                    private_memory[address] = value & 0xff;
                    return;
                }
            }
        } else if(soft_switches & SOFTSW_AUX_WRITE) {
            if((address >= 0x200) && (address < 0xC000)) {
                private_memory[address] = value & 0xff;
                return;
            }
        }

        if((address >= 0x200) && (address < 0xC000)) {
            apple_memory[address] = value & 0xff;
            return;
        }
    }
    
    // Nothing left to do for RAM accesses.
    if(address < 0xc000) return;

    // Shadow the soft-switches by observing all read & write bus cycles
    if(address < 0xc080) {
        switch(address & 0x7f) {
        case 0x00:
            if((internal_flags & (IFLAGS_IIGS_REGS | IFLAGS_IIE_REGS)) && ACCESS_WRITE) {
                soft_switches &= ~SOFTSW_80STORE;
            }
            break;
        case 0x01:
            if((internal_flags & (IFLAGS_IIGS_REGS | IFLAGS_IIE_REGS)) && ACCESS_WRITE) {
                soft_switches |= SOFTSW_80STORE;
            }
            break;
        case 0x04:
            if((internal_flags & (IFLAGS_IIGS_REGS | IFLAGS_IIE_REGS)) && ACCESS_WRITE) {
                soft_switches &= ~SOFTSW_AUX_WRITE;
            }
            break;
        case 0x05:
            if((internal_flags & (IFLAGS_IIGS_REGS | IFLAGS_IIE_REGS)) && ACCESS_WRITE) {
                soft_switches |= SOFTSW_AUX_WRITE;
            }
            break;
        case 0x08:
            if((internal_flags & (IFLAGS_IIGS_REGS | IFLAGS_IIE_REGS)) && ACCESS_WRITE) {
                soft_switches &= ~SOFTSW_AUXZP;
            }
            break;
        case 0x09:
            if((internal_flags & (IFLAGS_IIGS_REGS | IFLAGS_IIE_REGS)) && ACCESS_WRITE) {
                soft_switches |= SOFTSW_AUXZP;
            }
            break;
        case 0x0c:
            if((internal_flags & (IFLAGS_IIGS_REGS | IFLAGS_IIE_REGS)) && ACCESS_WRITE) {
                soft_switches &= ~SOFTSW_80COL;
            }
            break;
        case 0x0d:
            if((internal_flags & (IFLAGS_IIGS_REGS | IFLAGS_IIE_REGS)) && ACCESS_WRITE) {
                soft_switches |= SOFTSW_80COL;
            }
            break;
        case 0x0e:
            if((internal_flags & (IFLAGS_IIGS_REGS | IFLAGS_IIE_REGS)) && ACCESS_WRITE) {
                soft_switches &= ~SOFTSW_ALTCHAR;
            }
            break;
        case 0x0f:
            if((internal_flags & (IFLAGS_IIGS_REGS | IFLAGS_IIE_REGS)) && ACCESS_WRITE) {
                soft_switches |= SOFTSW_ALTCHAR;
            }
            break;
        case 0x21:
            if((internal_flags & (IFLAGS_IIGS_REGS | IFLAGS_IIE_REGS)) && ACCESS_WRITE) {
                if(value & 0x80) {
                    soft_switches |= SOFTSW_MONOCHROME;
                } else {
                    soft_switches &= ~SOFTSW_MONOCHROME;
                }
            }
            break;
        case 0x22:
            if((internal_flags & IFLAGS_IIGS_REGS) && ACCESS_WRITE) {
                apple_tbcolor = value & 0xff;
            }
            break;
        case 0x29:
            if((internal_flags & IFLAGS_IIGS_REGS) && ACCESS_WRITE) {
                soft_switches = (soft_switches & ~(SOFTSW_NEWVID_MASK << SOFTSW_NEWVID_SHIFT)) | ((value & SOFTSW_NEWVID_MASK) << SOFTSW_NEWVID_SHIFT);
            }
            break;
        case 0x34:
            if((internal_flags & IFLAGS_IIGS_REGS) && ACCESS_WRITE) {
                apple_border = value & 0x0f;
            }
            break;
        case 0x35:
            if((internal_flags & IFLAGS_IIGS_REGS) && ACCESS_WRITE) {
                soft_switches = (soft_switches & ~(SOFTSW_SHADOW_MASK << SOFTSW_SHADOW_SHIFT)) | ((value & SOFTSW_SHADOW_MASK) << SOFTSW_SHADOW_SHIFT);
            }
            break;
        case 0x50:
            soft_switches &= ~SOFTSW_TEXT_MODE;
            break;
        case 0x51:
            soft_switches |= SOFTSW_TEXT_MODE;
            break;
        case 0x52:
            soft_switches &= ~SOFTSW_MIX_MODE;
            break;
        case 0x53:
            soft_switches |= SOFTSW_MIX_MODE;
            break;
        case 0x54:
            soft_switches &= ~SOFTSW_PAGE_2;
            break;
        case 0x55:
            soft_switches |= SOFTSW_PAGE_2;
            break;
        case 0x56:
            soft_switches &= ~SOFTSW_HIRES_MODE;
            break;
        case 0x57:
            soft_switches |= SOFTSW_HIRES_MODE;
            break;
        case 0x5e:
            if(internal_flags & (IFLAGS_IIGS_REGS | IFLAGS_IIE_REGS)) {
                soft_switches |= SOFTSW_DGR;
            }
            break;
        case 0x5f:
            // Video 7 shift register
            if(soft_switches & SOFTSW_DGR) {
                internal_flags = (internal_flags & 0xfffffffc) | ((internal_flags & 0x1) << 1) | ((soft_switches & SOFTSW_80COL) ? 1 : 0);
            }

            if(internal_flags & (IFLAGS_IIGS_REGS | IFLAGS_IIE_REGS)) {
                soft_switches &= ~SOFTSW_DGR;
            }
            break;
        case 0x7e:
            if((internal_flags & IFLAGS_IIE_REGS) && ACCESS_WRITE) {
                soft_switches |= SOFTSW_IOUDIS;
            }
            break;
        case 0x7f:
            if((internal_flags & IFLAGS_IIE_REGS) && ACCESS_WRITE) {
                soft_switches &= ~SOFTSW_IOUDIS;
            }
            break;
        }
        return;
    }

    // Control sequences used by ROMX and ROMXe
    switch(current_machine) {
    case MACHINE_IIE:
        // Trigger on read sequence FACA FACA FAFE
        if(ACCESS_READ) {
            if((address >> 8) == 0xFA) {
                switch(address & 0xFF) {
                case 0xCA:
                    romx_unlocked = (romx_unlocked == 1) ? 2 : 1;
                    break;
                case 0xFE:
                    romx_unlocked = (romx_unlocked == 2) ? 3 : 0;
                    break;
                default:
                    if(romx_unlocked != 3)
                        romx_unlocked = 0;
                    break;
                }
            } else if(romx_unlocked == 3) {
                if((address >> 4) == 0xF81) {
                    romx_textbank = address & 0xF;
                }
                if(address == 0xF851) {
                    romx_changed = 1;
                    romx_unlocked = 0;
                }
            }
        }
        break;
    case MACHINE_II:
        // Trigger on read sequence CACA CACA CAFE
        if(ACCESS_READ) {
            if((address >> 8) == 0xCA) {
                switch(address & 0xFF) {
                case 0xCA:
                    romx_unlocked = (romx_unlocked == 1) ? 2 : 1;
                    break;
                case 0xFE:
                    romx_unlocked = (romx_unlocked == 2) ? 3 : 0;
                    break;
                default:
                    if(romx_unlocked != 3)
                        romx_unlocked = 0;
                    break;
                }
            } else if(romx_unlocked == 3) {
                if((address >> 4) == 0xCFD) {
                    romx_textbank = address & 0xF;
                }
                if((address >> 4) == 0xCFE) {
                    romx_changed = 1;
                    romx_unlocked = 0;
                }
            }
        }
        break;
    }

    // Card Registers
    if(ACCESS_WRITE) {
        if(CARD_SELECT && CARD_DEVSEL) {
            uint8_t color_index;

            cardslot = (address >> 4) & 0x7;
            switch(address & 0x0F) {
            case 0x01:
                mono_palette = (value >> 4) & 0xF;
                if(value & 8) {
                    internal_flags |= IFLAGS_OLDCOLOR;
                } else {
                    internal_flags &= ~IFLAGS_OLDCOLOR;
                }
                if(value & 4) {
                    internal_flags |= IFLAGS_VIDEO7;
                } else {
                    internal_flags &= ~IFLAGS_VIDEO7;
                }
                if(value & 2) {
                    internal_flags |= IFLAGS_GRILL;
                } else {
                    internal_flags &= ~IFLAGS_GRILL;
                }
                if(value & 1) {
                    internal_flags |= IFLAGS_INTERP;
                } else {
                    internal_flags &= ~IFLAGS_INTERP;
                }
                apple_memory[address] = value;
                break;
            case 0x02:
                terminal_tbcolor = value & 0xff;
                apple_memory[address] = terminal_tbcolor;
                break;
            case 0x03:
                terminal_border = value & 0x0f;
                apple_memory[address] = terminal_border;
                break;
            case 0x08:
                internal_flags &= ~IFLAGS_TERMINAL;
                break;
            case 0x09:
                internal_flags |= IFLAGS_TERMINAL;
                break;
            case 0x0A:
                terminal_fifo[terminal_fifo_wrptr++] = (value & 0xFF);
                apple_memory[address] = (terminal_fifo_rdptr - terminal_fifo_wrptr);
                break;
            case 0x0B:
#if 0
                if((value & 0xFF) <= 0x27) {
                    romx_textbank = (value & 0xFF);
                    romx_changed = 1;
                }
#endif
                break;
            case 0x0C:
                apple_memory[address] = value;
                break;
            case 0x0D:
                color_index = (apple_memory[address-1] >> 4) & 0xF;
#ifdef ANALOG_GS
                switch(apple_memory[address-1] & 3) {
                    case 1:
                        lores_palette[color_index] =
                            (lores_palette[color_index] & 0x00FF) |
                            (((uint16_t)(value & 0xF)) << 8);
                        break;
                    case 2:
                        lores_palette[color_index] =
                            (lores_palette[color_index] & 0x0F0F) |
                            (((uint16_t)(value & 0xF)) << 4);
                        break;
                    case 3:
                        lores_palette[color_index] =
                            (lores_palette[color_index] & 0x0FF0) |
                            (((uint16_t)(value & 0xF)) << 0);
                        break;
                }
#else
                switch(apple_memory[address-1] & 3) {
                    case 1:
                        lores_palette[color_index] =
                            (lores_palette[color_index] & 0x003F) |
                            (((uint16_t)(value & 0x7)) << 6);
                        break;
                    case 2:
                        lores_palette[color_index] =
                            (lores_palette[color_index] & 0x01C7) |
                            (((uint16_t)(value & 0x7)) << 3);
                        break;
                    case 3:
                        lores_palette[color_index] =
                            (lores_palette[color_index] & 0x01F8) |
                            (((uint16_t)(value & 0x7)) << 0);
                        break;
                }
#endif
                dhgr_palette[lores_to_dhgr[color_index]] = lores_palette[color_index];
                apple_memory[address] = value;
                break;
            }
        }
    } else if(CARD_SELECT && CARD_DEVSEL) {
        if((address & 0x0F) == 0x0A) {
            apple_memory[address] = (terminal_fifo_rdptr - terminal_fifo_wrptr);
        }            
    }

}
