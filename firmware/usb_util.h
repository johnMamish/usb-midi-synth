#ifndef _USB_UTIL_H
#define _USB_UTIL_H

#include <stdint.h>
#include "usbhost.h"

uint8_t lookup_pcksize(uint32_t bytes);

void print_device_descriptor(const usb_descriptor_t* desc);

#endif
