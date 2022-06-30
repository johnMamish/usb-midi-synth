#ifndef _CLOCK_H
#define _CLOCK_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Generic Clock Source enum
 */
typedef enum {
	XOSC, //0.4-32MHz Crystal Oscillator
	GCLKIN,
	GCLKGEN1,
	OSCULP32K, // 32.768kHz Ultra Low Power Internal Oscillator
	OSC32K, // 32.768kHz High Accuracy Internal Oscillator
	XOSC32K, // 32.768kHz Crystal Oscillator
	OSC8M,
	DFLL48M // Digital Frequency Locked Loop
} gclk_source;

/**
 * Generator Id enum
 */
typedef enum {
	GEN0,
	GEN1,
	GEN2,
	GEN3,
	GEN4,
	GEN5,
	GEN6,
	GEN7
} gclk_generator;

void generic_clk_generator_init(gclk_generator id, bool divsel, gclk_source src, bool oe);
void generic_clk_generator_div(gclk_generator id, uint16_t clk_div);
void generic_clk_channel_init(gclk_generator id, int channel);

#endif
