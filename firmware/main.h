#ifndef _MAIN_H
#define _MAIN_H
#include "samda1e16b.h"
#include <stdint.h>
#include "port.h"

#define FCLK (48000000)

typedef struct systick {
    volatile uint32_t CSR;
    volatile uint32_t RVR;
    volatile uint32_t CVR;
    volatile uint32_t CALIB;
} systick_t;

#define SYSTICK ((volatile systick_t*)0xe000e010)

#define NVIC_ISER ((volatile uint32_t*)0xe000e100)
#define NVIC_ICER ((volatile uint32_t*)0xe000e180)
#define NVIC_ISPR ((volatile uint32_t*)0xe000e200)
#define NVIC_ICPR ((volatile uint32_t*)0xe000e280)
#define NVIC_IPR  ((volatile uint32_t*)0xe000e400)

////////////////////////////////////////////////////////////////
// display pins
const extern gpio_pin_t disp_vdd_en_pin;
const extern gpio_pin_t disp_res_pin;
const extern gpio_pin_t disp_copi_pin;
const extern gpio_pin_t disp_sclk_pin;
const extern gpio_pin_t disp_cs_pin;
const extern gpio_pin_t disp_data_cmd_pin;

////////////////////////////////////////////////////////////////
// DAC pins
const extern gpio_pin_t dac_out_pin;
const extern gpio_pin_t volume_up_dn_pin;
const extern gpio_pin_t volume_clk_pin;

////////////////////////////////////////////////////////////////
// slider pins
const extern gpio_pin_t slider_1_pin;
const extern gpio_pin_t slider_2_pin;
const extern gpio_pin_t slider_3_pin;
const extern gpio_pin_t slider_4_pin;

////////////////////////////////////////////////////////////////
// cp2102 UART pins
const extern gpio_pin_t mcu_tx_cp2102_rx_pin;
const extern gpio_pin_t mcu_tx_cp2102_tx_pin;

////////////////////////////////////////////////////////////////
// power management pins
const extern gpio_pin_t mcu_power_button_pin;
const extern gpio_pin_t mcu_ldo_en_pin;
const extern gpio_pin_t mcu_vusb_host_enable_pin;

////////////////////////////////////////////////////////////////
// usb host pins
const extern gpio_pin_t usb_host_dn;
const extern gpio_pin_t usb_host_dp;

////////////////////////////////////////////////////////////////
// other pins
const extern gpio_pin_t led_hiz_pin;
const extern gpio_pin_t xtal_in_pin;
const extern gpio_pin_t xtal_out_pin;

#endif
