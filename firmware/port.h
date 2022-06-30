#ifndef _PORT_H
#define _PORT_H

#include "samda1e16b.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct gpio_pin {
    PortGroup* port;
    uint32_t   pin;
} gpio_pin_t;

typedef enum {
    PULL_DIR_NO_PULL,
    PULL_DIR_PULL_DOWN,
    PULL_DIR_PULL_UP
} pull_dir_e;

void initialize_output_pin(const gpio_pin_t* pin);
void deinitialize_output_pin(const gpio_pin_t* pin);
void set_output_pin(const gpio_pin_t* pin, bool val);

void initialize_input_pin(const gpio_pin_t* pin, pull_dir_e pull);

void initialize_af_pin(const gpio_pin_t* pin, int af_num);



#endif
