#pragma once

#define MB_ORB		0x00
#define MB_ORA		0x01
#define MB_DDRB		0x02
#define MB_DDRA		0x03
#define MB_CNT1L	0x04
#define MB_CNT1H	0x05
#define MB_LATCH1L 	0x06
#define MB_LATCH1H 	0x07
#define MB_CNT2L	0x08
#define MB_CNT2H	0x09
#define MB_SR		0x0A
#define MB_ACR 		0x0B
#define MB_PCR		0x0C
#define MB_IFR 		0x0D
#define MB_IER 		0x0E
#define MB_ORAX		0x0F

#define NCHANS 2
#define NREGS 16

//volatile uint8_t psg_data[NCHANS][NREGS];

void audio_businterface();
void audioUpdateTimers();
void audioInitBus();
