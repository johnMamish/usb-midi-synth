#include "samda1e16b.h"
#include "clock.h"

void generic_clk_generator_init(gclk_generator id, bool divsel,
				gclk_source src, bool oe) {
    uint32_t val = ((1 << 21) |
                    ((divsel == true) << 20) |
                    ((oe == true) << 19) |
                    (1 << 16) |
                    (((uint8_t) src & 0x1F) << 8) |
                    (id & 0xF));

    GCLK->GENCTRL.reg = val;
}

void generic_clk_generator_div(gclk_generator id, uint16_t clk_div) {
    GCLK->GENDIV.reg = (clk_div << 8) | (uint8_t) id;
}

void generic_clk_channel_init(gclk_generator id, int channel) {
    GCLK->CLKCTRL.reg = ((1 << 14) |
                     ((id & 0x7) << 8) |
                     ((uint8_t) channel & 0x3F));
}
