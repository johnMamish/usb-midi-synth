#ifndef _OSCILLATOR_H
#define _OSCILLATOR_H

/**
 * Some questions:
 *   - How to handle portamento?
 */

#include <stdint.h>

typedef enum {
    OSCILLATOR_TYPE_SINE,
    OSCILLATOR_TYPE_SQUARE,
    OSCILLATOR_TYPE_SAWTOOTH,
    OSCILLATOR_TYPE_TRIANGLE,
} oscillator_type_e;


/**
 * This class acts as an audio source.
 *
 * Given certain runtime-mutable parameters, it will render audio to a buffer.
 * Audio is rendered in indivisble time quanta; oscillator parameters can't be changed during a
 * render block. A major overhaul of this system
 */
typedef struct oscillator {
    // parameters
    /// which oscillator type do we have?
    oscillator_type_e type;

    /// q15_16 wave period in samples
    int32_t period;

    /// q15_16 volume. A volume of '1' is full-range.
    int32_t volume;

    // state
    /// Current phase into wave as a q_31 fraction of period.
    int32_t phase;
} oscillator_t;

void render_audio(oscillator_t* osc, uint8_t* buffer, size_t len);

#endif
