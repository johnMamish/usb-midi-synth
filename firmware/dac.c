#include "samda1e16b.h"

#include "clock.h"
#include "dac.h"
#include "my_dmac.h"
#include "main.h"
#include "port.h"

uint16_t dac_buffer[2][DAC_BUFSIZE];

static DmacDescriptor dmac_dac_descriptor_1 __attribute__((aligned(16)));

/**
 * Initializes the DAC to output 10-bit samples at 48ksps
 *
 *  - event triggering
 *    The DAC is triggered at FSAMP (nominally 48KHz) by
 * Also initializes DMA
 */
void dac_init()
{
    ////////////////////////////////////////////////////////////////
    // configure TC3 to run at 48KHz with a divider of 4.
    PM->APBCMASK.bit.TC3_ = 1;
    generic_clk_channel_init(GEN0, GCLK_CLKCTRL_ID_TCC2_TC3_Val);
    TC3->COUNT8.CTRLA.reg = ((2 << 8) |
                             (0 << 5) |
                             (1 << 2));

    TC3->COUNT8.PER.reg = 250;

    // generate an event on overflow
    TC3->COUNT8.EVCTRL.bit.OVFEO    = 1;

    ////////////////////////////////////////////////////////////////
    // setup DMAC channel 0 to feed DAC on DAC empty signal
    dmac_disable_channel(0);
    DMAC->CHID.bit.ID = 0;

    // Trigger source is DAC empty
    //DMAC->CHCTRLB.reg = (2 << 22) | (3 << 5);
    DMAC->CHCTRLB.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;
    DMAC->CHCTRLB.bit.TRIGSRC = 0x28;

    // Enable transfer complete interrupts
    DMAC->CHID.bit.ID = 0;
    DMAC->CHINTENSET.bit.TCMPL = 1;
    NVIC_ISER[DMAC_IRQn / 32] = (1 << DMAC_IRQn);

    dmac_base_descriptors[0].BTCTRL.bit.STEPSIZE = 0;   // step one beat at a time
    dmac_base_descriptors[0].BTCTRL.bit.STEPSEL = 1;    // step size applies to source addr
    dmac_base_descriptors[0].BTCTRL.bit.DSTINC = 0;     // don't increment dest addr
    dmac_base_descriptors[0].BTCTRL.bit.SRCINC = 1;     // increment source addr
    dmac_base_descriptors[0].BTCTRL.bit.BEATSIZE = 1;   // beat size is 16-bit halfword
    dmac_base_descriptors[0].BTCTRL.bit.BLOCKACT = 1;   // block interrupt on transfer complete
    dmac_base_descriptors[0].BTCTRL.bit.EVOSEL = 0;
    dmac_base_descriptors[0].BTCTRL.bit.VALID = 1;

    dmac_base_descriptors[0].BTCNT.reg = DAC_BUFSIZE;
    dmac_base_descriptors[0].SRCADDR.reg = (uint32_t)(&dac_buffer[0][DAC_BUFSIZE]);
    dmac_base_descriptors[0].DSTADDR.reg = (uint32_t)(&(DAC->DATABUF.reg));
    dmac_base_descriptors[0].DESCADDR.reg = (uint32_t)(&dmac_dac_descriptor_1);

    dmac_dac_descriptor_1.BTCTRL.bit.STEPSIZE = 0;   // step one beat at a time
    dmac_dac_descriptor_1.BTCTRL.bit.STEPSEL = 1;    // step size applies to source addr
    dmac_dac_descriptor_1.BTCTRL.bit.DSTINC = 0;     // don't increment dest addr
    dmac_dac_descriptor_1.BTCTRL.bit.SRCINC = 1;     // increment source addr
    dmac_dac_descriptor_1.BTCTRL.bit.BEATSIZE = 1;   // beat size is 16-bit halfword
    dmac_dac_descriptor_1.BTCTRL.bit.BLOCKACT = 1;   // block interrupt on transfer complete
    dmac_dac_descriptor_1.BTCTRL.bit.EVOSEL = 0;
    dmac_dac_descriptor_1.BTCTRL.bit.VALID = 1;

    dmac_dac_descriptor_1.BTCNT.reg = DAC_BUFSIZE;
    dmac_dac_descriptor_1.SRCADDR.reg = (uint32_t)(&dac_buffer[1][DAC_BUFSIZE]);
    dmac_dac_descriptor_1.DSTADDR.reg = (uint32_t)(&(DAC->DATABUF.reg));
    dmac_dac_descriptor_1.DESCADDR.reg = (uint32_t)(&dmac_base_descriptors[0]);

    dmac_enable_channel(0);

    ////////////////////////////////////////////////////////////////
    // configure the timer event to trigger the DAC via the Event System
    PM->APBCMASK.bit.EVSYS_ = 1;
    generic_clk_channel_init(GEN0, GCLK_CLKCTRL_ID_EVSYS_0_Val);

    // send EVSYS channel 0 to DAC start signal
    EVSYS->USER.reg = (1 << 8) | (0x1b << 0);

    // EVSYS channel 0 generates an event synchronously from TC3 rising edge
    EVSYS->CHANNEL.reg = ((1 << 26) | (2 << 24) | (0x33 << 16) | (0 << 0));

    ////////////////////////////////////////////////////////////////
    // Setup the DAC
    PM->APBCMASK.bit.DAC_ = 1;
    generic_clk_channel_init(GEN3, GCLK_CLKCTRL_ID_DAC_Val);

    initialize_af_pin(&dac_out_pin, PORT_PMUX_PMUXE_B_Val);

    DAC->CTRLB.bit.REFSEL = 1;
    DAC->CTRLB.bit.BDWP   = 1;
    DAC->CTRLB.bit.EOEN   = 1;
    DAC->CTRLB.bit.LEFTADJ = 1;

    DAC->EVCTRL.bit.STARTEI = 1;
    DAC->EVCTRL.bit.EMPTYEO = 1;

    DAC->CTRLA.bit.ENABLE = 1;

    DAC->DATABUF.reg = 0;

    // enable the timer
    TC3->COUNT8.CTRLA.reg |= (1 << 1);
}
