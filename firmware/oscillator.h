/**
 * This is a generic module that renders a bandlimited wave to an audio buffer.
 *
 *
 */

#ifndef _OSCILLATOR_H
#define _OSCILLATOR_H

/**
 * Some questions:
 *   - How to handle portamento?
 *     Just change the speed at which we update the phase.
 */

/**
 * Todos:
 *   - How to interpolate parameters as
 *     One way to do this cleanly software wise would be to
 */

#include <stdint.h>

#include "saeclib_circular_buffer.h"

#define  DELAY_LINE_CAPACITY (32)

typedef enum {
    OSCILLATOR_TYPE_SINE,
    OSCILLATOR_TYPE_SINE_FP,
    OSCILLATOR_TYPE_SQUARE,
    OSCILLATOR_TYPE_SAWTOOTH,
    OSCILLATOR_TYPE_TRIANGLE,
    OSCILLATOR_TYPE_SQUARE_ALIASED,
    OSCILLATOR_TYPE_PWM,
    OSCILLATOR_TYPE_IMPULSE,
    OSCILLATOR_TYPE_NOISE
} oscillator_type_e;

/**
 * This struct is used internally by the oscillator to keep track of when an impulse should be
 * emitted.
 */
typedef struct oscillator_impulse_event {
    /// Time at which the impulse should happen in samples.
    /// Implementations may wish to maintain these values so that they are always relative to
    /// sample 0 of a rendering window.
    /// This value might start rolling over if you have a rendering window that exceeds 16k samples,
    /// but why would you do that?
    int32_t t_q15_16;

    /// How far into the current wave period is this impulse, in fractions of a period?
    /// This information needs to be kept track of in order to properly manage frequency changes.
    /// TODO: would it be good to make this a q_31 instead?
    int32_t fractional_phase_q15_16;

    /// Magnitude and direction of this impulse
    int32_t magnitude_q0_31;
} oscillator_impulse_event_t;

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

    /// wave period in samples
    int32_t period_q15_16;

    /// volume. A volume of '1' is full-range.
    int32_t volume_q15_16;

    /// sampling rate in Hz.
    int32_t fsamp;

    ////////////////////////////////////////////////////////////////
    /// How far are we currently into the wave?
    /// Index of the most recently rendered impulse.
    int32_t index;

    /// How many samples ago was the most recently rendered impulse?
    int32_t sample_phase_q15_16;

    /// How far into the current period of the wave is the most recently rendered impulse?
    int32_t fractional_phase_q15_16;

    /// A buffer of oscillator_impulse_event_t.
    saeclib_circular_buffer_t delay_line;
    uint8_t delay_line_memory[DELAY_LINE_CAPACITY * sizeof(oscillator_impulse_event_t)];

    /// q2_29 output value.
    /// For some types of oscillator (square, sawtooth, triangle, pwm), we generate the wave by
    /// generating an impulse train and then integrating it. We give this value 2 extra bits of
    /// headroom
    int32_t output_integrator_q2_29;
} oscillator_t;


#define SINC_RADIUS (20)
#define SAMPLES_PER_SINC ((2 * SINC_RADIUS) + 1)
#define SUBSAMPLE_PHASE_PRECISION (6)
#define NUM_PHASES  (1 << SUBSAMPLE_PHASE_PRECISION)
const extern int32_t BLI_RADIUS_Q15_16;

extern int16_t blip_table_q_15[NUM_PHASES][SAMPLES_PER_SINC];

/**
 * This function sets up internal data structures used by this library and should be called before
 * any library functions are used.
 */
void oscillator_setup_tables();

/**
 *
 */
void oscillator_initialize(oscillator_t* osc, int32_t sampling_frequency);

/**
 * Given frequency is in hertz in q16_15.
 */
void oscillator_set_frequency(oscillator_t* osc, int32_t freq_q15_16);

/**
 * Renders audio to an int32_t buffer.
 *
 * This library assumes that all audio internally is dealt with as q2_29. This gives some headroom.
 * When audio is actually output to a device, it should be converted truncated to u16 or shifted
 * up to u32 depending on needs.
 */
void oscillator_render_to_buffer(oscillator_t* osc, int32_t* buffer, uint32_t len);


#endif
