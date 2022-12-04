#include <string.h>
#include "usbhost.h"

#include "main.h"
#include "clock.h"
#include "cprintf.h"

#include "log_levels.h"
#include "sercom3.h"

#include "usb_util.h"

const uint16_t USB_DEVICE_ADDR = 1;

usb_descriptor_t device_descriptor __attribute__((aligned(4))) = { 0 };

uint8_t setupbuf[8];
usbhost_controlxfer_t xfer = {
    {0x80, 6, 0x0100, 0x0000, 0x08},
    setupbuf
};

uint8_t pipe0bank[64] __attribute__((aligned(4)));

volatile UsbHostDescBank pipes[8] __attribute__((aligned(16)));

volatile Usb* usb = USB;

// need to figure out which configuration to choose
// need to figure out which interface to communicate with (look for Audio Midi streaming)

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
    USB->HOST.DESCADD.reg = (uint32_t)&pipes[2 * 0];

    USB->HOST.STATUS.reg &= ~(3 << 2);

    pipes[2 * 0].ADDR.reg = (uint32_t)pipe0bank;
    pipes[2 * 0].PCKSIZE.bit.AUTO_ZLP = 0;
    pipes[2 * 0].PCKSIZE.bit.SIZE = 0;        // Pipe size 8 Bytes
    pipes[2 * 0].PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
    pipes[2 * 0].PCKSIZE.bit.BYTE_COUNT = 0;

    pipes[1 * 2].ADDR.reg = 0;
    pipes[1 * 2].PCKSIZE.bit.AUTO_ZLP = 0;
    pipes[1 * 2].PCKSIZE.bit.SIZE = 3;        // Pipe size 64 Bytes
    pipes[1 * 2].PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
    pipes[1 * 2].PCKSIZE.bit.BYTE_COUNT = 0;
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
    USB->HOST.HostPipe[0].BINTERVAL.reg  = 1;     // can't send multiple consecutive OUT tokens in frame

    USB->HOST.HostPipe[1].PCFG.bit.PTYPE = 3;     // bulk pipe
    USB->HOST.HostPipe[1].PCFG.bit.BK    = 0;     // single-banked
    //USB->HOST.HostPipe[1].BINTERVAL.reg  = 0;     // can't send multiple consecutive OUT tokens in frame
    USB->HOST.HostPipe[1].BINTERVAL.reg  = 0;     // can't send multiple consecutive OUT tokens in frame

    for (volatile int i = 0; i < 10000; i++);


    pipes[2 * 0].CTRL_PIPE.bit.PERMAX = 3;

    // get descriptor
    xfer.setup_packet = (usbpacket_setup_t){0x80, 6, 0x0100, 0x0000, 8};
    xfer.do_data_phase = 1;
    xfer.payload = (void*)&device_descriptor;
    usbhost_ctrl_xfer_blocking(0, &xfer);
    cprintf(sercom3_putch, "recieved partial descriptor: ");
    for (int i = 0; i < 8; cprintf(sercom3_putch, "%02x ", (xfer.payload[i++])));
    cprintf(sercom3_putch, "\r\n");

    // issue SET_ADDRESS command
    xfer.setup_packet = (usbpacket_setup_t){0x00, 5, USB_DEVICE_ADDR, 0x0000, 0};
    xfer.payload = (void*)NULL;
    xfer.do_data_phase = 0;
    usbhost_ctrl_xfer_blocking(0, &xfer);

    // reconfigure pipe and wait for 2 milliseconds
    pipes[2 * 0].CTRL_PIPE.bit.PDADDR = USB_DEVICE_ADDR;
    pipes[2 * 0].PCKSIZE.bit.SIZE = lookup_pcksize(device_descriptor.bMaxPacketSize);
    pipes[1 * 2].CTRL_PIPE.bit.PDADDR = USB_DEVICE_ADDR;
    pipes[1 * 2].CTRL_PIPE.bit.PEPNUM = 2;
    pipes[1 * 2].PCKSIZE.bit.SIZE = 3;
    for (volatile int i = 0; i < 96000; i++);

    // get full device descriptor
    xfer.setup_packet = (usbpacket_setup_t){0x80, 6, 0x0100, 0x0000, device_descriptor.bLength};
    xfer.payload = (void*)&device_descriptor;
    xfer.do_data_phase = 1;
    usbhost_ctrl_xfer_blocking(0, &xfer);
    LOG_INFO(sercom3_putch, "device descriptor:\r\n");
    print_device_descriptor(&device_descriptor);

    // TODO: get configuration descriptors

    if ((device_descriptor.idVendor == 0x09e8) && (device_descriptor.idProduct == 0x006d)) {
        LOG_INFO(sercom3_putch, "AKAI EWI connected\r\n");
    } else {
        LOG_INFO(sercom3_putch, "unknown USB device\r\n");
        return;
    }

    // select configuration
    xfer.setup_packet = (usbpacket_setup_t){0x00, 9, 1, 0, 0};
    xfer.do_data_phase = 0;
    usbhost_ctrl_xfer_blocking(0, &xfer);
    LOG_INFO(sercom3_putch, "configuration 1 selected\r\n");
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
        cprintf(sercom3_putch, "%02x ", *((uint8_t*)(&pipes[2 * 0]) + i));
    }
    cprintf(sercom3_putch, "\r\n\r\n");
}

/**
 * Blocks until a single setup packet has been sent. Setup packets always consist of 8 bytes.
 *
 * @param[in]     pipe         Which pipe number this should be sent down
 * @param[in]     data         Pointer to the setup packet to be sent
 * @return        returns 0 when an ACK is recieved, -1 if an ACK isn't recieved.
 */
int usbhost_setup_xfer_blocking(int pipe, const usbpacket_setup_t* pkt)
{
    LOG_TRACE(sercom3_putch, "USB SETUP \r\n");

    pipes[2 * pipe].ADDR.reg = (uint32_t)(pkt);
    pipes[2 * pipe].PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
    pipes[2 * pipe].PCKSIZE.bit.BYTE_COUNT = 8;
    USB->HOST.HostPipe[pipe].PCFG.bit.PTOKEN = 0;
    USB->HOST.HostPipe[pipe].PSTATUSSET.bit.BK0RDY = 1;

    USB->HOST.HostPipe[pipe].PSTATUSCLR.bit.PFREEZE = 1;

    const uint8_t PINTFLAG_MASK = (1 << 4) | (1 << 3) | (1 << 2);
    volatile uint16_t pintflag_reg = 0;
    volatile uint16_t pintflag_reg_prev = -1;
    do {
        pintflag_reg = USB->HOST.HostPipe[pipe].PINTFLAG.reg;
        if (pintflag_reg != pintflag_reg_prev) {
            LOG_TRACE(sercom3_putch, "PINTFLAG = %02x\r\n", pintflag_reg);
        }
        pintflag_reg_prev = pintflag_reg;
    } while (!(pintflag_reg & PINTFLAG_MASK));
    USB->HOST.HostPipe[pipe].PSTATUSSET.bit.PFREEZE = 1;

    LOG_TRACE(sercom3_putch, "PSTATUS = %02x\r\n", USB->HOST.HostPipe[pipe].PSTATUS.reg);

    if (pintflag_reg & ((1 << 3) | (1 << 2))) {
        LOG_TRACE(sercom3_putch, "STATUS_BK   = %02x\r\n", pipes[2 * pipe].STATUS_BK.reg);
        LOG_TRACE(sercom3_putch, "STATUS_PIPE = %02x\r\n", pipes[2 * pipe].STATUS_PIPE.reg);
        return -1;
    } else {
        USB->HOST.HostPipe[pipe].PINTFLAG.reg = (1 << 4);
        return 0;
    }
}

/**
 * Blocks until a potentially multi-packet OUT xfer has been completed.
 *
 * @return         returns 0  when all packets in the xfer have been successfully ACK'd
 *                 returns -1 if the xfer is terminated by a STALL
 *                 returns -2 if there's a pipe error
 *                 returns -3 if the transmission failed (NAK)
 */
int usbhost_out_xfer_blocking(int pipe, uint8_t* data, int len)
{
    LOG_TRACE(sercom3_putch, "USB OUT\r\n");

    pipes[2 * pipe].ADDR.reg = (uint32_t)data;
    pipes[2 * pipe].PCKSIZE.bit.MULTI_PACKET_SIZE = 0;
    pipes[2 * pipe].PCKSIZE.bit.BYTE_COUNT = len;
    USB->HOST.HostPipe[pipe].PCFG.bit.PTOKEN = 2;
    USB->HOST.HostPipe[pipe].PSTATUSSET.bit.BK0RDY = 1;   // for OUT xfer, set bk0rdy

    USB->HOST.HostPipe[pipe].PSTATUSCLR.bit.PFREEZE = 1;

    const uint8_t PINTFLAG_MASK = (1 << 5) | (1 << 3) | (1 << 0);
    volatile uint16_t pintflag_reg = 0;
    volatile uint16_t pintflag_reg_prev = -1;
    do {
        pintflag_reg = USB->HOST.HostPipe[pipe].PINTFLAG.reg;
        if (pintflag_reg != pintflag_reg_prev) {
            LOG_TRACE(sercom3_putch, "PINTFLAG = %02x\r\n", pintflag_reg);
        }
        pintflag_reg_prev = pintflag_reg;
    } while (!(pintflag_reg & PINTFLAG_MASK));

    USB->HOST.HostPipe[pipe].PSTATUSSET.bit.PFREEZE = 1;

#ifdef PSEUDOCODE_DONT_COMPILE
    if (stall) {     // PINTFLAG[5]
        // device returned a stall. Needs to be handled at higher level.
        return -2;
    }
    if (txstp) {     // PINTFLAG[4]
        // should never happen. should only be set for setup packet.
    }
    if (perr) {      // PINTFLAG[3]
        // If either...
        //   * a CRC error on an IN packet, or
        //   * a time-out error on a handshake, or
        //   * an invalid PID (e.g. a PID's CRC is wrong) (I think), or
        //   * a wrong packet PID (e.g. DATA1 rx'd when DATA0 expected)
        // occurs, then the counter ERCNT in STATUS_PIPE is incremented.
        //
        // If ERCNT exceeds CTRL_PIPE.PERMAX, then too many errors have occured and PERR is set.
    }
    if (trfail) {    // PINTFLAG[2]
        // for control & bulk endpoints, doesn't trigger (?)
        // for interrupt endpoints, triggers on a data over/under flow
        // for isochronous endpoints, triggers on data over/underflow or CRC error.
        //
        // The CRC error only exists for isochronous endpoints. I'm not entirely sure what happens
        // if a CRC error occurs on a non-isochronous endpoint. According to the USB standard...
        //    For a device, a bad CRC on an OUT packet (host-to-dev) should result in NO HANDSHAKE
        //                  from the device.
        //    For a host,   a bad CRC on an IN paket should result in NO HANDSHAKE from host.
        //                  Software should retry.
        //                  I'm not sure how this could happen... I guess when designing a
        //                  USB protocol it's important to make sure that internal state of a device
        //                  is never advanced until an xfer is successfully hand-shook.
    }
#endif
    if (pintflag_reg & (1 << 5)) {
        LOG_DEBUG(sercom3_putch, "USB STALL\r\n");
        return -1;
    } else if (pintflag_reg & (1 << 3)) {
        LOG_TRACE(sercom3_putch, "pipe error\r\n");
        LOG_TRACE(sercom3_putch, "STATUS_PIPE = %02x\r\n", pipes[2 * pipe].STATUS_PIPE.reg);
        return -2;
    } else if (pintflag_reg & (1 << 2)) {
        LOG_TRACE(sercom3_putch, "transmission failed\r\n");
        LOG_TRACE(sercom3_putch, "STATUS_BK = %02x\r\n", pipes[2 * pipe].STATUS_BK.reg);
        return -3;
    } else {
        USB->HOST.HostPipe[pipe].PINTFLAG.reg = (1 << 0);
        return 0;
    }
}

/**
 * Blocks until a potentially multi-packet IN xfer has been completed.
 */
int usbhost_in_xfer_blocking(int pipe, uint8_t* data, int len, int* retlen)
{
    LOG_TRACE(sercom3_putch, "USB IN\r\n");

    pipes[2 * pipe].ADDR.reg = (uint32_t)data;
    pipes[2 * pipe].PCKSIZE.bit.MULTI_PACKET_SIZE = len;
    pipes[2 * pipe].PCKSIZE.bit.BYTE_COUNT = 0;
    USB->HOST.HostPipe[pipe].PCFG.bit.PTOKEN = 1;

    LOG_DEBUG(sercom3_putch, "PCKSIZE = %08x\r\n", pipes[2 * pipe].PCKSIZE.reg);

    USB->HOST.HostPipe[pipe].PSTATUSCLR.bit.BK0RDY = 1;
    USB->HOST.HostPipe[pipe].PSTATUSCLR.bit.PFREEZE = 1;

    const uint8_t PINTFLAG_MASK = (1 << 5) | (1 << 3) | (1 << 2) | (1 << 0);
    volatile uint16_t pintflag_reg = 0;
    volatile uint16_t pintflag_reg_prev = -1;
    do {
        pintflag_reg = USB->HOST.HostPipe[pipe].PINTFLAG.reg;
        if (pintflag_reg != pintflag_reg_prev) {
            LOG_TRACE(sercom3_putch, "PINTFLAG = %02x\r\n", pintflag_reg);
        }
        pintflag_reg_prev = pintflag_reg;
    } while (!(pintflag_reg & PINTFLAG_MASK));
    USB->HOST.HostPipe[pipe].PSTATUSSET.bit.PFREEZE = 1;

    LOG_DEBUG(sercom3_putch, "PCKSIZE = %08x\r\n", pipes[2 * pipe].PCKSIZE.reg);
    LOG_TRACE(sercom3_putch, "PSTATUS = %02x\r\n", USB->HOST.HostPipe[pipe].PSTATUS.reg);

    if (pintflag_reg & (1 << 5)) {
        LOG_DEBUG(sercom3_putch, "USB STALL\r\n");
        return -1;
    } else if (pintflag_reg & (1 << 3)) {
        LOG_TRACE(sercom3_putch, "pipe error\r\n");
        LOG_TRACE(sercom3_putch, "STATUS_PIPE = %02x\r\n", pipes[2 * pipe].STATUS_PIPE.reg);
        return -2;
    } else if (pintflag_reg & (1 << 2)) {
        LOG_TRACE(sercom3_putch, "transmission failed\r\n");
        LOG_TRACE(sercom3_putch, "STATUS_BK = %02x\r\n", pipes[2 * pipe].STATUS_BK.reg);
        return -3;
    } else {
        USB->HOST.HostPipe[pipe].PINTFLAG.reg = (1 << 0);
        if (retlen) *retlen = pipes[2 * pipe].PCKSIZE.bit.BYTE_COUNT;
        return 0;
    }
}

void usbhost_ctrl_xfer_blocking(int pipe, usbhost_controlxfer_t* xfer)
{
    LOG_TRACE(sercom3_putch, "USB ctrl xfer\r\n");

    int err = 0;

    // SETUP stage
    // setup packet is always 8 bytes
    err |= usbhost_setup_xfer_blocking(0, &xfer->setup_packet);
    LOG_TRACE(sercom3_putch, "\r\n");
    while (err);

    // DATA stage
    if (xfer->do_data_phase) {
        if (xfer->setup_packet.bmRequestType & (1 << 7)) {
            err |= usbhost_in_xfer_blocking(pipe, xfer->payload, xfer->setup_packet.wLength, NULL);
        } else {
            err |= usbhost_out_xfer_blocking(pipe, xfer->payload, xfer->setup_packet.wLength);
        }
    }
    LOG_TRACE(sercom3_putch, "\r\n");
    while(err);

    // STATUS stage
    if (xfer->setup_packet.bmRequestType & (1 << 7)) {
        err |= usbhost_out_xfer_blocking(pipe, NULL, 0);
    } else {
        err |= usbhost_in_xfer_blocking(pipe, NULL, 0, NULL);
    }
    LOG_TRACE(sercom3_putch, "\r\n");
    if (err)
        USB->HOST.HostPipe[pipe].PINTFLAG.reg |= (1 << 5);
}
