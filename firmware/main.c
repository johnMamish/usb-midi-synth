#include "samda1e16b.h"
#include <stdbool.h>
#include <stdint.h>
#include "main.h"
#include "./clock.h"

#include "sercom3.h"
#include "dac.h"
#include "my_dmac.h"
#include "cprintf.h"


void sys_init();
void gpio_init();

void dac_init();

/**
 * Blocking function that increases or decreases the volume.
 * pass 'false' to decrease the volume and 'true' to increase it.
 */
void change_volume(bool dir);

int main()
{
    sys_init();
    gpio_init();
    set_output_pin(&mcu_ldo_en_pin, true);
    set_output_pin(&mcu_vusb_host_enable_pin, true);

    for (int i = 0; i < DAC_BUFSIZE; i++) {
        dac_buffer[0][i] = i % 2 ? 500 : 0;
        dac_buffer[1][i] = 250;
    }

    sercom3_init();
    dac_init();

    uint16_t j = 0;
    while (1) {
    }
}

/**
 * Setup system clocks and enable power to relevant parts of the system
 */
void sys_init()
{
    // Enable XOSC. This automatically sets PA14 and PA15 to be connected to the crystal pins.
    SYSCTRL->XOSC.reg = ((0b1000 << 12) |
                         (0b1    << 11) |
                         (0b0    << 7)  |
                         (0b1    << 2) |
                         (0b1    << 1));
    while (SYSCTRL->PCLKSR.bit.XOSCRDY);

    ////////////////////////////////////////////////////////////////
    // Setup PLL to be fed by (XOSC / 250) via GCLK1. It should output 48MHz.
    generic_clk_generator_div(GEN1, 250);
    generic_clk_generator_init(GEN1, false, XOSC, false);
    generic_clk_channel_init(GEN1, GCLK_CLKCTRL_ID_DFLL48_Val);

    // wait for clock domains to sync up
    while(!SYSCTRL->PCLKSR.bit.DFLLRDY);

    // setup PLL and wait for it to lock.
    SYSCTRL->DFLLCTRL.reg = 0;
    SYSCTRL->DFLLMUL.reg = (10 << 26) | (32 << 16) | 1500;
    SYSCTRL->DFLLCTRL.reg |= ((1 << 2) | (1 << 1));
    while (!(SYSCTRL->PCLKSR.bit.DFLLLCKC && SYSCTRL->PCLKSR.bit.DFLLLCKF));

    // setup flash wait states for 48MHz
    NVMCTRL->CTRLB.bit.RWS = 1;

    // switch clock over to DFLL48
    generic_clk_generator_div(GEN0, 0);
    generic_clk_generator_init(GEN0, false, DFLL48M, false);

    ////////////////////////////////////////////////////////////////
    // configure clock generator 3 to generate DFLL48M / 138, giving us a clock of 347kHz for the DAC
    generic_clk_generator_div(GEN3, 138);
    generic_clk_generator_init(GEN3, false, DFLL48M, false);


    ////////////////////////////////////////////////////////////////
    // enable systick
    SYSTICK->RVR = 0x00ffffff;


    dmac_init();
}

/**
 * Initialize all gpios
 */
void gpio_init()
{
    ////////////////////////////////////////////////////////////////
    // power management pins
    initialize_output_pin(&mcu_ldo_en_pin);
    initialize_output_pin(&mcu_vusb_host_enable_pin);
    initialize_input_pin(&mcu_power_button_pin, PULL_DIR_PULL_UP);

    ////////////////////////////////////////////////////////////////
    // display
    set_output_pin(&disp_vdd_en_pin, false);
    initialize_output_pin(&disp_vdd_en_pin);

    set_output_pin(&disp_res_pin, false);
    initialize_output_pin(&disp_res_pin);

    ////////////////////////////////////////////////////////////////
    // dac control
    set_output_pin(&volume_up_dn_pin, false);
    initialize_output_pin(&volume_up_dn_pin);
    set_output_pin(&volume_clk_pin, false);
    initialize_output_pin(&volume_clk_pin);
}

/**
 * Initialize sercom3 to be on PA16 and PA17 and transmit at 115200
 */
void usb_host_init()
{

}

void change_volume(bool dir)
{
    set_output_pin(&volume_clk_pin, false);
    set_output_pin(&volume_up_dn_pin, dir);
    for(volatile int i = 0; i < 10; i++);

    set_output_pin(&volume_clk_pin, true);
    for(volatile int i = 0; i < 10; i++);
    set_output_pin(&volume_clk_pin, false);
}


////////////////////////////////////////////////////////////////
// display pins
const gpio_pin_t disp_vdd_en_pin    = { &PORT->Group[0],  0 };
const gpio_pin_t disp_res_pin       = { &PORT->Group[0],  1 };
const gpio_pin_t disp_copi_pin      = { &PORT->Group[0],  7 };
const gpio_pin_t disp_sclk_pin      = { &PORT->Group[0],  9 };
const gpio_pin_t disp_cs_pin        = { &PORT->Group[0], 10 };
const gpio_pin_t disp_data_cmd_pin  = { &PORT->Group[0], 11 };

////////////////////////////////////////////////////////////////
// DAC pins
const gpio_pin_t dac_out_pin            = { &PORT->Group[0], 2 };
const gpio_pin_t volume_up_dn_pin       = { &PORT->Group[0], 18};
const gpio_pin_t volume_clk_pin         = { &PORT->Group[0], 19};

////////////////////////////////////////////////////////////////
// slider pins
const gpio_pin_t slider_1_pin      = { &PORT->Group[0],  3 };
const gpio_pin_t slider_2_pin      = { &PORT->Group[0],  4 };
const gpio_pin_t slider_3_pin      = { &PORT->Group[0],  5 };
const gpio_pin_t slider_4_pin      = { &PORT->Group[0],  6 };

////////////////////////////////////////////////////////////////
// cp2102 UART pins
const gpio_pin_t mcu_tx_cp2102_rx_pin  = { &PORT->Group[0], 16 };
const gpio_pin_t mcu_tx_cp2102_tx_pin  = { &PORT->Group[0], 17 };

////////////////////////////////////////////////////////////////
// power management pins
const gpio_pin_t mcu_power_button_pin    = { &PORT->Group[0], 22 };
const gpio_pin_t mcu_ldo_en_pin          = { &PORT->Group[0], 23 };
const gpio_pin_t mcu_vusb_host_enable_pin = { &PORT->Group[0], 28 };

////////////////////////////////////////////////////////////////
// usb host pins
const gpio_pin_t usb_host_dn             = { &PORT->Group[0], 24 };
const gpio_pin_t usb_host_dp             = { &PORT->Group[0], 25 };

////////////////////////////////////////////////////////////////
// other pins
const gpio_pin_t led_hiz_pin            = { &PORT->Group[0], 7 };
const gpio_pin_t xtal_in_pin            = { &PORT->Group[0], 14};
const gpio_pin_t xtal_out_pin           = { &PORT->Group[0], 15};
