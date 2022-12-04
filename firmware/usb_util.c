#include "usb_util.h"

#include "cprintf.h"
#include "log_levels.h"
#include "sercom3.h"

/**
 * Given a pipe size, returns the corresponding setting for PCKSIZE.SIZE
 */
uint8_t lookup_pcksize(uint32_t bytes)
{
    for (int i = 0, size = 8; i <= 7; i++, size <<= 1) {
        if (bytes == size)
            return i;
    }
    return -1;
}

/**
 *
 */
void print_device_descriptor(const usb_descriptor_t* desc)
{
    LOG_INFO(sercom3_putch, "    Length: %02x\r\n", desc->bLength);
    LOG_INFO(sercom3_putch, "    DescriptorType: %02x\r\n", desc->bDescriptorType);
    LOG_INFO(sercom3_putch, "    bcdUSB: %04x\r\n", desc->bcdUSB);
    LOG_INFO(sercom3_putch, "    DeviceClass: %02x\r\n", desc->bDeviceClass);
    LOG_INFO(sercom3_putch, "    DeviceSubclass: %02x\r\n", desc->bDeviceSubClass);
    LOG_INFO(sercom3_putch, "    DeviceProtocol: %02x\r\n", desc->bDeviceProtocol);
    LOG_INFO(sercom3_putch, "    MaxPacketSize: %02x\r\n", desc->bMaxPacketSize);

    LOG_INFO(sercom3_putch, "    idVendor: %04x\r\n", desc->idVendor);
    LOG_INFO(sercom3_putch, "    idProduct: %04x\r\n", desc->idProduct);
    LOG_INFO(sercom3_putch, "    bcdDevice: %04x\r\n", desc->bcdDevice);
    LOG_INFO(sercom3_putch, "    NumConfigurations: %02x\r\n", desc->bNumConfigurations);
}
