#include "common/buffers.h"
#include "common/config.h"
#include "common/abus.h"
#include "audio.h"
#include "audinterface.h"

volatile uint8_t ora[NCHANS];
volatile uint8_t psg_data[NCHANS][NREGS];
volatile bool via_timer[NCHANS][2];

void audioUpdateTimers()
{

}

static void start_timer(int channel, int no)
{
	if (no) {
		if (!(psg_data[channel][MB_IER] & 0x20)) return;
	} else {
		if (!(psg_data[channel][MB_IER] & 0x40)) return;
	}
	if (!no) {
        apple_memory[0xC404 | (channel*0x80)] = apple_memory[0xC406 | (channel*0x80)];
        apple_memory[0xC405 | (channel*0x80)] = apple_memory[0xC407 | (channel*0x80)];
	} else {
		//div = ss->psg_data[channel][MB_CNT2L] | (ss->psg_data[channel][MB_CNT2H]<<8);
	}
	via_timer[channel][no] = true;
//	system_command(ss->st->sr, SYS_COMMAND_SET_CPUTIMER, div, DEF_CPU_TIMER_ID(ss->st) | channel | (no << 2));
}

static void fix_ifr(int channel)
{
	if ((psg_data[channel][MB_IFR] &= 0x7F) & (psg_data[channel][MB_IER] & 0x7F)) {
		psg_data[channel][MB_IFR] |= 0x80;
		apple_memory[0xC40D | (channel*0x80)] = psg_data[channel][MB_IFR];
	}
}

static void psg_ifr(int channel, uint8_t data)
{
//	printf("psg_ifr %x on channel %i\n", data, channel);
	data = (~data) | 0x80;
	psg_data[channel][MB_IFR] &= data;
	apple_memory[0xC40D | (channel*0x80)] = psg_data[channel][MB_IFR];
	fix_ifr(channel);
	//if (!(data&0x80)) 
//	system_command(ss->st->sr, SYS_COMMAND_NOIRQ, 0, 0);
}

static void psg_ier(int channel, uint8_t data)
{
//	printf("psg_ier %x on channel %i\n", data, channel);
	if (data & 0x80) {
		data &= 0x7F;
		psg_data[channel][MB_IER] |= data;
		apple_memory[0xC40E | (channel*0x80)] = psg_data[channel][MB_IER] | 0x80;
		fix_ifr(channel);
		start_timer(channel, 0);
		start_timer(channel, 1);
	} else {
		//data ^= 0x7F;
		//psg_data[channel][MB_IER] &= data;
		psg_data[channel][MB_IER] &= 0x80 | ~data;
		apple_memory[0xC40E | (channel*0x80)] = psg_data[channel][MB_IER] | 0x80;
		if (!(psg_data[channel][MB_IER] & 0x20)) via_timer[channel][1] = false;
		if (!(psg_data[channel][MB_IER] & 0x40)) via_timer[channel][0] = false;
	}
}

void __time_critical_func(audio_businterface)(uint32_t address, uint32_t value) {
    uint8_t data = value & 0xff;
    int chan = 0;
    if (address & 0x80) chan = 1;
    
    if((address >= 0xC400) && (address < 0xC490)) {
    if(ACCESS_WRITE) {
        apple_memory[address] = data;
	    switch (address & 0x0F) {
            case MB_IFR:
                        psg_ifr(chan, data);
                        break;
            case MB_IER:
                        psg_ier(chan, data);
                        break;
	        case MB_CNT1H: // timer 1 counter h
		                //ss->psg_data[chan][MB_CNT1L] = ss->psg_data[chan][MB_LATCH1L];
                        apple_memory[0xC404 | (chan*0x80)] = apple_memory[0xC406 | (chan*0x80)];
		                //ss->psg_data[chan][MB_LATCH1H] = data;
                        apple_memory[0xC407 | (chan*0x80)] = data;
		                psg_data[chan][MB_IFR] &= ~0x40;
                        apple_memory[0xC40D | (chan*0x80)] = psg_data[chan][MB_IFR];
		                //ss->timer_start[chan][0] = cpu_get_tsc(ss->st->sr);
		                fix_ifr(chan);
		                start_timer(chan, 0);
		                break;
	        case MB_LATCH1L: // timer 1 latch l
		                break;
	        case MB_LATCH1H: // timer 1 latch h
		                psg_data[chan][MB_IFR] &= ~0x40;
                        apple_memory[0xC40D | (chan*0x80)] = psg_data[chan][MB_IFR];
		                fix_ifr(chan);
		                break;
	        case MB_CNT2L: // timer 2 counter l
		                //ss->t2l_l[chan] = data;
		                break;
	        case MB_CNT2H: // timer 2 counter h
		                //ss->psg_data[chan][MB_CNT2L] = ss->t2l_l[chan];
		                psg_data[chan][MB_IFR] &= ~0x20;
                        apple_memory[0xC40D | (chan*0x80)] = psg_data[chan][MB_IFR];
		                //ss->timer_start[chan][1] = cpu_get_tsc(ss->st->sr);
		                fix_ifr(chan);
		                start_timer(chan, 1);
		                break;        
            case MB_ORB:
                        if (data == 0x00) //ay reset
                        {
                            if (chan==0) audioResetPsg0(); else audioResetPsg1();
                        } else
                        if (data == 0x07) //ay latch
                        {
                            if (chan==0) audioWritePsg0Reg(ora[0] % NREGS); else audioWritePsg1Reg(ora[1] % NREGS);
                        } else
                        if (data == 0x06) //ay write
                        {
                            if (chan==0) audioWritePsg0(ora[0]); else audioWritePsg1(ora[1]);
                            //gpio_put(PICO_DEFAULT_LED_PIN, 1);
                        }
                        break;
	        case MB_ORAX:
						apple_memory[0xC401 | (chan*0x80)] = data;
			case MB_ORA: 
                        ora[chan] = data;
                        break;
            default:
                break;
	    }
            //apple_memory[0xc404] = 0xf8;
            //apple_memory[0xc484] = 0xf8;
        }
    } 
}

void audioInitBus()
{
	for (int j = 0; j < NCHANS; ++j) {
		for (int i = 0; i < 2; ++i) {
			via_timer[j][i] = false;
		}
	}
	via_timer[0][0] = true;
	via_timer[1][0] = true;
	for (int j = 0; j < NCHANS; ++j) {
		for (int i = 0; i < NREGS; ++i) {
			psg_data[j][i] = 0;
		}
	}
/*    psg_data[0][MB_IFR] = 0x7f;
    psg_data[1][MB_IFR] = 0x7f;
    psg_data[0][MB_IER] = 0x7f;
    psg_data[1][MB_IER] = 0x7f;*/
	apple_memory[0xc40d] = 0x00;
    apple_memory[0xc48d] = 0x00;
    apple_memory[0xc40e] = 0x00;
    apple_memory[0xc48e] = 0x00;
}