#include "main.h"
#include "clock.h"
#include "sercom3.h"
#include "saeclib_circular_buffer.h"

#include <stdint.h>

saeclib_u8_circular_buffer_t sercom3_tx_buffer;
uint8_t sercom3_tx_buffer_space[256];


void sercom3_init()
{
    // initialize data structures
    saeclib_u8_circular_buffer_init(&sercom3_tx_buffer,
                                    sercom3_tx_buffer_space,
                                    sizeof(sercom3_tx_buffer_space));

    // Enable sercom3's clock
    PM->APBCMASK.bit.SERCOM3_ = 1;

    // initialize output pins
    initialize_af_pin(&mcu_tx_cp2102_rx_pin, PORT_PMUX_PMUXE_D_Val);
    initialize_af_pin(&mcu_tx_cp2102_tx_pin, PORT_PMUX_PMUXE_D_Val);

    // route GCLK0 (which contains a 48MHz clock) to sercom 3 and sercomx slow
    generic_clk_channel_init(GEN0, GCLK_CLKCTRL_ID_SERCOM3_CORE_Val);
    generic_clk_channel_init(GEN0, GCLK_CLKCTRL_ID_SERCOMX_SLOW_Val);

    SERCOM3->USART.CTRLA.reg = ((1 << 30) |    // DORD: LSB first
                                (1 << 20) |    // RXPO: SERCOM[1] is RX
                                (0 << 16) |    // TXPO: SERCOM[0] is TX
                                (1 << 13) |    // SAMPR: 16x oversamp w/ fractional baud gen
                                (1 << 2));     // use internal clock    uint8_t choffset = 0;

    // fractional division of 48MHz input clock by (16 * (26 + (0 / 8)))
    //SERCOM3->USART.BAUD.reg = (0 << 13) | (26 << 0);
    SERCOM3->USART.BAUD.reg = (2 << 13) | (3 << 0);

    // enable RX and TX
    SERCOM3->USART.CTRLB.reg = (1 << 17) | (1 << 16);

    // enable SERCOM interrupt in NVIC
    NVIC_ISER[SERCOM3_IRQn / 32] = (1 << SERCOM3_IRQn);

    // enable sercom 3
    SERCOM3->USART.CTRLA.reg |= (1 << 1);
}


void sercom3_putch(char ch)
{
    // disable SERCOM interrupt in NVIC
    NVIC_ICER[SERCOM3_IRQn / 32] = (1 << SERCOM3_IRQn);

    // add character to buffer
    saeclib_u8_circular_buffer_pushone(&sercom3_tx_buffer, ch);

    // enable SERCOM interrupt in NVIC and TX interrupt in peripheral
    SERCOM3->USART.INTENSET.bit.DRE = 1;
    NVIC_ISER[SERCOM3_IRQn / 32] = (1 << SERCOM3_IRQn);
}

void SERCOM3_Handler()
{
    uint32_t sta = SERCOM3->USART.INTFLAG.reg;

    if (sta & (1 << 0)) {
        uint8_t ch;

        if (saeclib_u8_circular_buffer_popone(&sercom3_tx_buffer, &ch) == SAECLIB_ERROR_UNDERFLOW) {
            SERCOM3->USART.INTENCLR.bit.DRE = 1;
        } else {
            SERCOM3->USART.DATA.reg = ch;
        }
    }
}
