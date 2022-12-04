#ifndef _MY_DMAC_H
#define _MY_DMAC_H

#include "samda1e16b.h"
#include <stdint.h>

#define DMAC_N_CHANNELS 1

extern DmacDescriptor dmac_base_descriptors[DMAC_N_CHANNELS] __attribute__((aligned(16)));

extern volatile uint8_t dac_frontbuffer;
extern volatile uint8_t dac_buffer_ready;
extern volatile uint8_t dac_overflow_detect;

/**
 * Sets up the DMAC peripheral's core. Doesn't enable or configure any channels.
 */
void dmac_init();

void dmac_disable_channel(int chid);
void dmac_enable_channel(int chid);

#endif
