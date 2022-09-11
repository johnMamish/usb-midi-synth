#include <string.h>
#include "usbhost.h"

#include "main.h"
#include "clock.h"
#include "cprintf.h"
#include "sercom3.h"

usb_descriptor_t device_descriptor __attribute__((aligned(4)));

uint8_t setupbuf[8];
usbhost_controlxfer_t xfer = {
    {0x80, 6, 0x0001, 0x0000, 0x0008},
    setupbuf
};

uint8_t pipe0bank[64] __attribute__((aligned(4)));

volatile UsbHostDescBank pipes[8] __attribute__((aligned(16)));

volatile Usb* usb = USB;

void usbhost_init()
{
    // Datasheet errata; need to disable pull resistors explicitly
    // but not actually necessary; they're disabled by default.
    // usb_host_dn.port->PINCFG[usb_host_dn.pin] &= 0xfe;
    // usb_host_dp.port->PINCFG[usb_host_dp.pin] &= 0xfe;
    initialize_af_pin(&usb_host_dn, PORT_PMUX_PMUXE_G_Val);
    initialize_af_pin(&usb_host_dp, PORT_PMUX_PMUXE_G_Val);

#if 0
    ////////////////////////////////////////////////////////////////
    // setup TC4 to act as a one-shot timer for determining when to drive a USB reset on the bus
    // after device connection is detected
    PM->APBCMASK.bit.TC4_ = 1;
    generic_clk_channel_init(GEN0, GCLK_CLKCTRL_ID_TC4_TC5_Val);
    TC4->COUNT16.CTRLA.reg = ((7 << 8) |       // divide 48MHz input clock by 1024
                              (1 << 5) |       // wave generation is in "Match Frequency" mode
                              (0 << 2));       // counter is in 16-bit mode
    TC4->COUNT16.CTRLBSET.bit.ONESHOT = 1;
    TC4->COUNT16.CTRLBSET.bit.DIR = 1;

    NVIC_ISER[TC4_IRQn / 32] = (1 << TC4_IRQn);
#endif

    // Enable USB clocks
    // PM->ABPBMASK.bit.USB_ = 1; // no need, already enabled at startup.
    generic_clk_channel_init(0, GCLK_CLKCTRL_ID_USB_Val);

    // Load USB PADCAL values
    volatile uint32_t* swcal = ((volatile uint32_t*)0x806020);
    uint32_t USB_TRANSN = (swcal[45 / 32] >> (45 % 32)) & 0x1f;
    uint32_t USB_TRANSP = (swcal[50 / 32] >> (50 % 32)) & 0x1f;
    uint32_t USB_TRIM   = (swcal[55 / 32] >> (55 % 32)) & 0x07;
    USB->HOST.PADCAL.reg = (USB_TRIM << 12) | (USB_TRANSN << 6) | USB_TRANSP;

    // enable USB peripheral as a host
    USB->HOST.CTRLA.reg = (1 << 7) | (1 << 1);
    while (USB->HOST.SYNCBUSY.bit.ENABLE);

#if 0
    // enable USB interrupts
    //                        DDISC      DCONN      RST
    USB->HOST.INTENSET.reg = ((1 << 9) | (1 << 8) | (1 << 3));
    NVIC_ISER[USB_IRQn / 32] = (1 << USB_IRQn);
#endif

    USB->HOST.QOSCTRL.reg = (2 << 2) | (2 << 0);

    // signal that VUSB is stable
    USB->HOST.CTRLB.bit.VBUSOK = 1;

    // set up pipe 0 to be usable as a control pipe
    USB->HOST.DESCADD.reg = (uint32_t)&pipes[0];

    pipes[0].ADDR.reg = (uint32_t)pipe0bank;
    pipes[0].PCKSIZE.bit.AUTO_ZLP = 0;
    pipes[0].PCKSIZE.bit.SIZE = 0;        // Pipe size 8 Bytes
    pipes[0].PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
    pipes[0].PCKSIZE.bit.BYTE_COUNT = 0;

    pipes[1].ADDR.reg = (uint32_t)pipe0bank;
    pipes[1].PCKSIZE.bit.AUTO_ZLP = 0;
    pipes[1].PCKSIZE.bit.SIZE = 0;        // Pipe size 8 Bytes
    pipes[1].PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
    pipes[1].PCKSIZE.bit.BYTE_COUNT = 0;
}

static uint8_t pipe0_buf[64] __attribute__((aligned(4)));
void usbhost_blocking_enumerate()
{
    // wait until a usb device has been connected
    while (!USB->HOST.INTFLAG.bit.DCONN);
    USB->HOST.INTFLAG.reg = (1 << 8);

    // wait for 100ms
    for (volatile int i = 0; i < 2400000; i++);

    // send a bus reset
    USB->HOST.CTRLB.bit.BUSRESET = 1;
    while (!USB->HOST.INTFLAG.bit.RST);
    USB->HOST.INTFLAG.reg = (1 << 3);

    // reset completed, setup pipe 0
    USB->HOST.HostPipe[0].PCFG.bit.PTYPE = 1;     // control pipe
    USB->HOST.HostPipe[0].PCFG.bit.BK    = 0;     // single-banked
    USB->HOST.HostPipe[0].BINTERVAL.reg  = 0;     // can send multiple consecutive OUT tokens in frame

    for (volatile int i = 0; i < 10000; i++);

    // get descriptor
    //xfer.setup_packet = (usbhost_setup_t){0x80, 6, 0x0001, 0x0000, 8};
    xfer.setup_packet = (usbhost_setup_t){0x80, 6, 0x0100, 0x0000, 8};
    xfer.payload = (void*)&device_descriptor;
    usbhost_ctrl_xfer_blocking(0, &xfer);

    cprintf(sercom3_putch, "recieved descriptor: ");
    for (int i = 0; i < 8; cprintf(sercom3_putch, "%02x ", (xfer.payload[i++])));
    cprintf(sercom3_putch, "\r\n");

    // issue SET_ADDRESS command
    static uint8_t inbuf[8];
}

#if 0


// This function is specifically here to trigger a USB reset 100ms after the USB device is plugged
// in
void TC4_Handler()
{
    TC4->COUNT16.INTFLAG.reg = (1 << 0);
    TC4->COUNT16.INTENCLR.reg = (1 << 0);

    // we only use TC4 as a one-shot timer for initiating a USB reset after device attachment
    USB->HOST.CTRLB.bit.BUSRESET = 1;

    cprintf(sercom3_putch, "TC4\r\n");
}

#define CONTROL_PHASE_SETUP 0
#define CONTROL_PHASE_DATA 1
#define CONTROL_PHASE_STATUS 2
#define CONTROL_PHASE_IDLE 3

volatile uint8_t pipe0_phase = CONTROL_PHASE_IDLE;

void USB_Handler()
{
    int16_t sta = USB->HOST.INTFLAG.reg;

    cprintf(sercom3_putch, "USB_Handler: sta = 0x%04x\r\n", sta);

    if (sta & (1 << 9)) {
        // when a device is disconnected, we...
        USB->HOST.INTFLAG.reg = (1 << 9);
    }

    if (sta & (1 << 8)) {
        USB->HOST.INTFLAG.reg = (1 << 8);

        // when a device is connected, we need to wait for at least 100ms (according to the USB 2.0
        // spec section 9.1.2) before issuing a bus reset command.

        // we need a periodic usb handler function that initiate device reset 100ms after the device
        // was attached

        // enable TC4 overflow / underflow interrupt

        TC4->COUNT16.INTENSET.reg = (1 << 0);
        TC4->COUNT16.COUNT.reg = 4800000 / 1024;
        TC4->COUNT16.CTRLA.bit.ENABLE = 1;
        TC4->COUNT16.CTRLBSET.bit.CMD = 1;
    }

    if (sta & (1 << 3)) {
        // reset completed, need to set up pipe 0
        USB->HOST.HostPipe[0].PCFG.bit.PTYPE = 1;     // control pipe
        USB->HOST.HostPipe[0].PCFG.bit.BK    = 0;     // single-banked
        USB->HOST.HostPipe[0].BINTERVAL.reg  = 0;     // can send multiple consecutive OUT tokens in frame

    }

    ////////////////////////////////////////////////////////////////
    // pipe interrupts
    /*if (USB->HOST.PINTSMRY.reg & (1 << 0)) {

      }*/
}
#endif

void dump_regs()
{
    cprintf(sercom3_putch, "pipe regs\r\n");
    for (int i = 0; i < 10; i++) {
        cprintf(sercom3_putch, "%02x ", *((uint8_t*)(&USB->HOST.HostPipe[0].PCFG.reg) + i));
    }

    cprintf(sercom3_putch, "\r\npipe RAM\r\n");
    for (int i = 0; i < 16; i++) {
        cprintf(sercom3_putch, "%02x ", *((uint8_t*)(&pipes[0]) + i));
    }
    cprintf(sercom3_putch, "\r\n\r\n");
}

void usbhost_ctrl_xfer_blocking(int pipe, usbhost_controlxfer_t* xfer)
{
    // determine whether the data stage should be IN or OUT according to bmRequestType
    // if bit 7 is 0: request is host to device
    // if bit 7 is 1: request is device to host
    int ptoken1 = (xfer->setup_packet.bmRequestType & (1 << 7)) ? 1 : 2;
    int ptoken2 = (xfer->setup_packet.bmRequestType & (1 << 7)) ? 2 : 1;

    // SETUP stage
    // setup packet is always 8 bytes
    pipes[0].ADDR.reg = (uint32_t)(&xfer->setup_packet);
    pipes[0].PCKSIZE.bit.BYTE_COUNT = 8;
    pipes[0].PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
    USB->HOST.HostPipe[0].PSTATUSSET.bit.BK0RDY = 1;
    USB->HOST.HostPipe[0].PSTATUSCLR.bit.PFREEZE = 1;

    volatile uint16_t pintflag_reg = 0;
    volatile uint16_t pintflag_reg_prev = -1;
    do {
        pintflag_reg = USB->HOST.HostPipe[0].PINTFLAG.reg;
        if (pintflag_reg != pintflag_reg_prev) {
            cprintf(sercom3_putch, "PINTFLAG[0] = %04x\r\n", pintflag_reg);
        }
        pintflag_reg_prev = pintflag_reg;
    } while (!(pintflag_reg & (1 << 4)));
    USB->HOST.HostPipe[0].PSTATUSCLR.bit.BK0RDY = 1;
    USB->HOST.HostPipe[0].PSTATUSSET.bit.PFREEZE = 1;
    USB->HOST.HostPipe[0].PINTFLAG.reg = (1 << 4);
    cprintf(sercom3_putch, "----\r\n");

    // DATA stage
    USB->HOST.HostPipe[0].PSTATUSSET.bit.PFREEZE = 1;

    pipes[0].ADDR.reg = (uint32_t)xfer->payload;
    pipes[0].PCKSIZE.bit.BYTE_COUNT = xfer->setup_packet.wLength;
    //pipes[0].PCKSIZE.bit.MULTI_PACKET_SIZE = xfer->setup_packet.wLength;
    pipes[0].PCKSIZE.bit.MULTI_PACKET_SIZE = 0;

    USB->HOST.HostPipe[0].PCFG.bit.PTOKEN = ptoken1;
    if (ptoken1 == 1)
        USB->HOST.HostPipe[0].PSTATUSCLR.bit.BK0RDY = 1;   // for IN xfer, clear bk0rdy
    else
        USB->HOST.HostPipe[0].PSTATUSSET.bit.BK0RDY = 1;   // for OUT xfer, set bk0rdy
    USB->HOST.HostPipe[0].PSTATUSCLR.bit.PFREEZE = 1;

    do {
        pintflag_reg = USB->HOST.HostPipe[0].PINTFLAG.reg;
        if (pintflag_reg != pintflag_reg_prev) {
            cprintf(sercom3_putch, "PINTFLAG[0] = %02x\r\n", pintflag_reg);
        }
        pintflag_reg_prev = pintflag_reg;
    } while (!(pintflag_reg & (1 << 0)));
    USB->HOST.HostPipe[0].PSTATUSSET.bit.PFREEZE = 1;
    USB->HOST.HostPipe[0].PINTFLAG.reg = (1 << 0);

    cprintf(sercom3_putch, "----\r\n");

    // STATUS stage
    USB->HOST.HostPipe[0].PSTATUSSET.bit.PFREEZE = 1;
    USB->HOST.HostPipe[0].PCFG.bit.PTOKEN = ptoken2;
    pipes[0].ADDR.reg = (uint32_t)xfer->payload;
    pipes[0].PCKSIZE.bit.BYTE_COUNT = 0;

    USB->HOST.HostPipe[0].PSTATUSSET.bit.BK0RDY = 1;
    USB->HOST.HostPipe[0].PSTATUSCLR.bit.PFREEZE = 1;

    do {
        pintflag_reg = USB->HOST.HostPipe[0].PINTFLAG.reg;
        if (pintflag_reg != pintflag_reg_prev)
            cprintf(sercom3_putch, "PINTFLAG[0] = %02x\r\n", pintflag_reg);
        pintflag_reg_prev = pintflag_reg;
    } while (!(pintflag_reg & (1 << 0)));

    USB->HOST.HostPipe[0].PINTFLAG.reg = (1 << 0);

    cprintf(sercom3_putch, "----\r\n");
}
