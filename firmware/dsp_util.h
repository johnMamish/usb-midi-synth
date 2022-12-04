#ifndef _DSP_UTIL_H
#define _DSP_UTIL_H

#include <stdint.h>

/**
 * Takes as input a positive number which represents an angle in radians / 2pi; i.e. t = 32768
 * corresponds to theta = pi
 *
 * Returns a Q.15 value
 */
int16_t sine_u16(uint16_t t);

#endif
