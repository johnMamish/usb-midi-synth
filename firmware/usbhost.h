#ifndef _USBHOST_H
#define _USBHOST_H

#include <stdint.h>

typedef struct __attribute__((packed)) usbhost_setup {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usbhost_setup_t;


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
    usbhost_setup_t setup_packet;
    uint8_t* payload;
} usbhost_controlxfer_t;

void usbhost_init();

/**
 * Initiates a control xfer and blocks until it's complete.
 *
 * The direction of the transfer is determined by xfer->setup_packet.bmRequestType and determines
 * whether data is read from or written to xfer->payload.
 */
void usbhost_ctrl_xfer_blocking(int pipe, usbhost_controlxfer_t* xfer);

void usbhost_blocking_enumerate();

#endif
