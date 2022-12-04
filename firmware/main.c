#include "samda1e16b.h"
#include <stdbool.h>
#include <stdint.h>
#include "main.h"
#include "./clock.h"

#include "dsp_util.h"

#include "sercom3.h"
#include "dac.h"
#include "my_dmac.h"
#include "cprintf.h"
#include "usbhost.h"

void sys_init();
void gpio_init();

void dac_init();

/**
 * Blocking function that increases or decreases the volume.
 * pass 'false' to decrease the volume and 'true' to increase it.
 */
void change_volume(bool dir);

static usbhost_controlxfer_t xfer __attribute__((aligned(4)));
static uint8_t xfer_buf[64] __attribute__((aligned(16)));

typedef struct audio_state {
    int32_t phase;     // current phase in samples; Q24.8
    uint32_t period;   // period in samples; uQ24.8
    uint32_t volume;   // uQ.16
} audio_state_t;

void render_audio(uint16_t* target, audio_state_t* s, int len)
{
    for (int i = 0; i < len; i++) {
	toggle_output_pin(&slider_1_pin);
        int32_t sample = (uint32_t)sine_u16(((s->phase << 13) / (s->period >> 2)) << 1);
        sample = (sample * s->volume) >> 8;
        target[i] = ((uint16_t)sample + 0x8000);

        s->phase += (1 << 8);
        while (s->phase >= s->period) {
            s->phase -= s->period;
        }
    }
}

// uQ24.8
// starts at midi note 0, which is inaudible.
const uint32_t midi_note_periods[] = {
 1502972,  1418617,  1338996,  1263844,  1192910,  1125957,  1062762,  1003114,   946813,   893673,   843515,   796172,
  751486,   709309,   669498,   631922,   596455,   562979,   531381,   501557,   473407,   446836,   421757,   398086,
  375743,   354654,   334749,   315961,   298227,   281489,   265690,   250778,   236703,   223418,   210879,   199043,
  187872,   177327,   167375,   157981,   149114,   140745,   132845,   125389,   118352,   111709,   105439,    99521,
   93936,    88664,    83687,    78990,    74557,    70372,    66423,    62695,    59176,    55855,    52720,    49761,
   46968,    44332,    41844,    39495,    37278,    35186,    33211,    31347,    29588,    27927,    26360,    24880,
   23484,    22166,    20922,    19748,    18639,    17593,    16606,    15674,    14794,    13964,    13180,    12440,
   11742,    11083,    10461,     9874,     9320,     8797,     8303,     7837,     7397,     6982,     6590,     6220,
    5871,     5541,     5230,     4937,     4660,     4398,     4151,     3918,     3698,     3491,     3295,     3110,
    2935,     2771,     2615,     2468,     2330,     2199,     2076,     1959,     1849,     1745,     1647,     1555,
};

int main()
{
    sys_init();
    gpio_init();
    set_output_pin(&mcu_ldo_en_pin, true);
    set_output_pin(&mcu_vusb_host_enable_pin, true);   for (volatile int i = 0; i < 100000; i++);

    audio_state_t osc = {.phase = 0, .period = ((DAC_BUFSIZE / 2) << 8), .volume = (16)};

    render_audio(dac_buffer[0], &osc, DAC_BUFSIZE);
    render_audio(dac_buffer[1], &osc, DAC_BUFSIZE);

    initialize_output_pin(&led_hiz_pin);

    //change_volume(true);
    //change_volume(true);
    //change_volume(false);
    //change_volume(false);

    sercom3_init();
    dac_init();
    usbhost_init();

    cprintf(sercom3_putch, "\r\n\r\nhello\r\n");

    usbhost_blocking_enumerate();

    cprintf(sercom3_putch, "EWI enumerated\r\n");

    int count = 0;
    uint16_t j = 0;
    uint8_t ewi_data[64];

    while (1) {
        // update audio generation parameters according to what was
        // while (!midi_queue.empty())
        //     update_audio_generator(&oscillator, &midi);
        int len = -1;
        int er = usbhost_in_xfer_blocking(1, ewi_data, 64, &len);

        if (er == -3) {
            USB->HOST.HostPipe[1].PINTFLAG.reg |= (1 << 2);
        } else if (er == 0) {
            for (int i = 0; i < len; i += 4) {
                if (ewi_data[i] == 0x0b) {
                    //cprintf(sercom3_putch, "pressure: %i\r\n", ewi_data[i + 3]);
                    osc.volume = ewi_data[i + 3];
                } else if (ewi_data[i] == 0x0d) {
                    // ignore, data is same as above
                } else if (ewi_data[i] == 0x09) {
                    //cprintf(sercom3_putch, "note %i, velocity %i\r\n", ewi_data[i + 2], ewi_data[i + 3]);
                    if (ewi_data[i + 3] > 0)
                        osc.period = midi_note_periods[ewi_data[i + 2]];
                } else if (ewi_data[i] == 0x0e) {
                    // pitch bend

                }
            }
        }
        // generate audio
        if (dac_buffer_ready) {
            dac_buffer_ready = 0;
            uint8_t backbuf = (dac_frontbuffer + 1) % 2;
            render_audio(dac_buffer[backbuf], &osc, DAC_BUFSIZE);
            count++;

	    if (dac_overflow_detect) {
		set_output_pin(&led_hiz_pin, true);
	    } else {
		set_output_pin(&led_hiz_pin, false);
	    }
	    dac_overflow_detect = 0;
        }
    }

    while(1);
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

    ////////////////////////////////////////////////////////////////
    // steal slider 1 and 2 to be debug output pins
    set_output_pin(&slider_1_pin, false);
    set_output_pin(&slider_2_pin, false);
    initialize_output_pin(&slider_1_pin);
    initialize_output_pin(&slider_2_pin);
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
