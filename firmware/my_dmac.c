#include <stdint.h>

#include "samda1e16b.h"
#include "main.h"
#include "my_dmac.h"

DmacDescriptor dmac_base_descriptors[DMAC_N_CHANNELS] __attribute__((aligned(16)));
static uint8_t dmac_writeback[16 * DMAC_N_CHANNELS] __attribute__((aligned(16)));

void dmac_init()
{
    ////////////////////////////////////////////////////////////////
    // enable DMAC
    PM->AHBMASK.bit.DMAC_ = 1;

    // Datasheet seems to suggest a software reset?
    DMAC->CTRL.bit.SWRST = 1;
    while(DMAC->CTRL.bit.SWRST);

    // Set descriptor base and writeback memory addresses
    DMAC->BASEADDR.reg = (int32_t)(&dmac_base_descriptors[0]);
    DMAC->WRBADDR.reg  = (int32_t)(dmac_writeback);

    // Highest QoS level
    DMAC->QOSCTRL.reg |= 0x3f;

    // enable DMA
    DMAC->CTRL.reg = (0xf << 8) | (1 << 1);
}

void dmac_disable_channel(int chid)
{
    DMAC->CHID.bit.ID = chid;
    DMAC->CHCTRLA.bit.ENABLE = 0;
    while(DMAC->CHCTRLA.bit.ENABLE != 0);
}

void dmac_enable_channel(int chid)
{
    DMAC->CHID.bit.ID = chid;
    DMAC->CHCTRLA.bit.ENABLE = 1;
    while(DMAC->CHCTRLA.bit.ENABLE != 1);
}

void DMAC_Handler()
{
    if ((DMAC->INTPEND.reg & (0x7 << 8)) && (DMAC->INTPEND.bit.ID == 1)) {
        DMAC->INTPEND.bit.TCMPL = 1;
        initialize_output_pin(&led_hiz_pin);
        led_hiz_pin.port->OUTTGL.reg = (1 << led_hiz_pin.pin);
    }
}
