#ifndef _DAC_H
#define _DAC_H

#include <stdint.h>

#define DAC_FSAMP 48000
#define DAC_BUFSIZE 512

extern uint16_t dac_buffer[2][DAC_BUFSIZE];

void dac_init();


#endif
