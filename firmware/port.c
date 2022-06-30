#include "port.h"

void initialize_output_pin(const gpio_pin_t* pin)
{
    pin->port->DIRSET.reg = (1 << pin->pin);
}

void deinitialize_output_pin(const gpio_pin_t* pin)
{
    pin->port->DIRCLR.reg = (1 << pin->pin);
}

void set_output_pin(const gpio_pin_t* pin, bool val)
{
    if (val)
        pin->port->OUTSET.reg = (1 << pin->pin);
    else
        pin->port->OUTCLR.reg = (1 << pin->pin);
}

void initialize_input_pin(const gpio_pin_t* pin, pull_dir_e pull)
{
    pin->port->DIRCLR.reg = (1 << pin->pin);
    pin->port->PINCFG[pin->pin].bit.INEN = 1;

    switch (pull) {
        case PULL_DIR_NO_PULL: {
            break;
        }

        case PULL_DIR_PULL_DOWN: {
            pin->port->PINCFG[pin->pin].bit.PULLEN = 1;
            pin->port->OUTCLR.reg = (1 << pin->pin);
            break;
        }

        case PULL_DIR_PULL_UP: {
            pin->port->PINCFG[pin->pin].bit.PULLEN = 1;
            pin->port->OUTSET.reg = (1 << pin->pin);
            break;
        }
    }
}

void initialize_af_pin(const gpio_pin_t* pin, int af_num)
{
    pin->port->PINCFG[pin->pin].bit.PMUXEN = 1;
    if ((pin->pin % 2) == 0) {
        pin->port->PMUX[pin->pin >> 1].bit.PMUXE = af_num;
    } else {
        pin->port->PMUX[pin->pin >> 1].bit.PMUXO = af_num;
    }
}
