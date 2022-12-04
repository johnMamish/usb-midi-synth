#ifndef _USBHOST_H
#define _USBHOST_H
#include "main.h"
#include <stdint.h>

extern volatile UsbHostDescBank pipes[] __attribute__((aligned(16)));

typedef struct __attribute__((packed)) usbpacket_setup {
    uint8_t bmRequestType;
    uint8_t bRequest;

    // BE CAREFUL - looks like wValue is a lie. actually 2 values.
    uint16_t wValue;

    uint16_t wIndex;
    uint16_t wLength;
} usbpacket_setup_t;


typedef struct __attribute__((packed)) usb_descriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
} usb_descriptor_t;

typedef struct __attribute__((packed)) usbhost_controlxfer {
    usbpacket_setup_t setup_packet;
    uint8_t* payload;
    uint8_t do_data_phase;
} usbhost_controlxfer_t;

void usbhost_init();

/**
 * Initiates a control xfer and blocks until it's complete.
 *
 * The direction of the transfer is determined by xfer->setup_packet.bmRequestType and determines
 * whether data is read from or written to xfer->payload.
 */
void usbhost_ctrl_xfer_blocking(int pipe, usbhost_controlxfer_t* xfer);
int usbhost_in_xfer_blocking(int pipe, uint8_t* data, int len, int* retlen);
void usbhost_blocking_enumerate();

#endif
