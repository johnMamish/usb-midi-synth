#include <math.h>
#include <stdio.h>
#include <string.h>
#include "oscillator.h"

#include "saeclib_circular_buffer.h"

static const double PI = 3.14159265358979323846;

static double blackman_symmetrical(double n, double window_radius)
{
    return (0.42 +
            (0.5  * cos((2. * PI * n) / (2. * window_radius))) +
            (0.08 * cos((4. * PI * n) / (2. * window_radius))));
}

static double sinc(double t)
{
    return sin(PI * t) / (t * PI);
}

static double windowed_sinc(double t, double window_radius)
{
    if (fabs(t) > window_radius) return 0.;

    double val = sinc(t);
    if (isnan(val))
	val = 1.0;

    return (val * blackman_symmetrical(t, window_radius));
}

//#define SINC_RADIUS (20)
//#define SAMPLES_PER_SINC ((2 * SINC_RADIUS) + 1)
//#define SUBSAMPLE_PHASE_PRECISION (6)
//#define NUM_PHASES  (1 << SUBSAMPLE_PHASE_PRECISION)

const int32_t BLI_RADIUS_Q15_16 = SINC_RADIUS << 16;

/**
 * Table of windowed sinc pulses.
 *
 * TODO: due to symmetry, we can cut the size of this table by a factor of 4 if needed.
 */
int16_t blip_table_q_15[NUM_PHASES][SAMPLES_PER_SINC];

void oscillator_setup_tables()
{
    //printf("blip_table:\n"
    //"----------------------------------------------------------------\n");
    for (int32_t phase = 0; phase < NUM_PHASES; phase++) {
	for (int32_t i = 0; i < SAMPLES_PER_SINC; i++) {
	    double t = i - SINC_RADIUS - ((double)phase / ((double)NUM_PHASES));
	    double val = windowed_sinc(t, SINC_RADIUS);
	    blip_table_q_15[phase][i] = (int16_t)(val * 32767. + 0.5);

	    //printf("%6i, ", blip_table_q_15[phase][i]);
	}
	//printf("\n");
    }
    //printf("\n");
}

void oscillator_initialize(oscillator_t* osc, int32_t sampling_frequency)
{
    osc->type   = OSCILLATOR_TYPE_SINE;
    osc->period_q15_16 = 128 << 16;
    osc->volume_q15_16 = (1 << 16) - 1;
    osc->fsamp  = sampling_frequency;
    osc->index = 0;
    osc->sample_phase_q15_16 = 0;
    osc->fractional_phase_q15_16 = 0;
    osc->output_integrator_q2_29 = 0;

    saeclib_circular_buffer_init(&osc->delay_line,
				 osc->delay_line_memory,
				 DELAY_LINE_CAPACITY * sizeof(oscillator_impulse_event_t),
				 sizeof(oscillator_impulse_event_t));
}

void oscillator_set_frequency(oscillator_t* osc, int32_t freq_q15_16)
{
    osc->period_q15_16 = (int32_t)((((uint64_t)osc->fsamp) << (16*2)) / (freq_q15_16));
    printf("osc period set to %i = %f\n", osc->period_q15_16, ((double)osc->period_q15_16) / 65536.0);
}

/**
 * Some thoughts about time-synchronizing audio
 *
 * I'm bothered by the idea that notes will start, stop, and change pitch on a quantized grid.
 * If we only update oscillator once a frame and have 1 millisecond long frames, for instance,
 * notes started at t = -0.9ms and t = -0.1ms will both start at t = 0ms.
 *
 * Although this bothers me, I think that we've got a fundamental limitation in our MIDI
 * controller.
 */

/**
 * Tells whether a given BL-impulse overlaps with a window starting at t = 0.
 */
static bool impulse_overlaps_window(const oscillator_impulse_event_t* d,
				    int32_t impulse_radius_q15_16,
				    int32_t window_width_q15_16)
{
    if (d->t_q15_16 < (0 - impulse_radius_q15_16))
	return false;
    else if (d->t_q15_16 > (window_width_q15_16 + impulse_radius_q15_16))
	return false;
    else
	return true;
}

/**
 * Returns true if the impulse tail hangs over the start of the window.
 * Returns false if the impulse fully exists in the future.
 */
static bool impulse_overlaps_window_start(const oscillator_impulse_event_t* d,
					  int32_t impulse_radius_q15_16)
{
    return (d->t_q15_16 < impulse_radius_q15_16);
}

/**
 * Uses an oscillator's fractional and sample phase state to figure out which impulse should come
 * next and how far along in the delay line it will be placed
 */
static void oscillator_peek_next_impulse(const oscillator_t* osc,
					 oscillator_impulse_event_t* d,
					 const oscillator_impulse_event_t* waveform)
{
    // Search through the list until we've found where we left off.
    int d_idx = 0;
    for (int i = 0; waveform[i].fractional_phase_q15_16 != -1; i++) {
	if (waveform[i].fractional_phase_q15_16 > osc->fractional_phase_q15_16) {
	    d_idx = i;
	    break;
	}
    }

    // Use the oscillator's period and the fractional distance of this impulse in the waveform
    // from the previous one to determine the position of the new impulse.
    int32_t dfract_q15_16 = waveform[d_idx].fractional_phase_q15_16 - osc->fractional_phase_q15_16;
    if (dfract_q15_16 < 0) dfract_q15_16 += (1 << 16);
    int32_t dt = (int32_t)(((int64_t)dfract_q15_16 * osc->period_q15_16) >> 16);
    d->t_q15_16 = osc->sample_phase_q15_16 + dt;
    d->fractional_phase_q15_16 = waveform[d_idx].fractional_phase_q15_16;
    d->magnitude_q0_31 = waveform[d_idx].magnitude_q0_31;

    // if any part of this impulse happended in the past, shift it forward to the earliest time
    // such that it's not in the past.
    if (impulse_overlaps_window_start(d, BLI_RADIUS_Q15_16)) {
	d->t_q15_16 = BLI_RADIUS_Q15_16;
    }
}

static void oscillator_advance_next_impulse(oscillator_t* osc,
					    const oscillator_impulse_event_t* waveform)
{
    // Find the next impulse and
    oscillator_impulse_event_t d;
    oscillator_peek_next_impulse(osc, &d, waveform);

    // advance the oscillator
    osc->sample_phase_q15_16 = d.t_q15_16;
    osc->fractional_phase_q15_16 = d.fractional_phase_q15_16;
}


/**
 * output buffer is in q2_29.
 */
void oscillator_render_to_buffer(oscillator_t* osc, int32_t* buffer, uint32_t len)
{
    switch (osc->type) {
        case OSCILLATOR_TYPE_SINE: {
            for (int i = 0; i < len; i++) {
		double phase = ((double)osc->sample_phase_q15_16) / ((double)osc->period_q15_16);
		int32_t sample = (int32_t)(((1 << 15) * sin(2 * PI * phase)) + 0.5);
		buffer[i] = (sample * osc->volume_q15_16) >> 1;

		osc->sample_phase_q15_16 += (1 << 16);
		if (osc->sample_phase_q15_16 >= osc->period_q15_16)
		    osc->sample_phase_q15_16 -= osc->period_q15_16;
            }
            break;
        }

	case OSCILLATOR_TYPE_SQUARE: {
	    static const oscillator_impulse_event_t single_period_pulse_train[] = {
		{0, ((int32_t)(0.25 * 65536)), (1 << 31)},
		{0, ((int32_t)(0.75 * 65536)), -(1 << 31)},
		{-1, -1, -1}
	    };

	    const int32_t LEN_Q15_16 = len << 16;

	    // Remove all impulses from the delay line that are too far in the past to be heard
	    oscillator_impulse_event_t impulse;
	    while (1) {
		saeclib_error_e se = saeclib_circular_buffer_peekone(&osc->delay_line, &impulse);
		if ((se != SAECLIB_ERROR_NOERROR) ||
		    impulse_overlaps_window(&impulse, BLI_RADIUS_Q15_16, LEN_Q15_16))
		    break;
		else
		    saeclib_circular_buffer_disposeone(&osc->delay_line);
	    }

	    // Add impulses that will occur during the current sample window to the delay line.
	    while (1) {
		oscillator_peek_next_impulse(osc, &impulse, single_period_pulse_train);
		if (!impulse_overlaps_window(&impulse, BLI_RADIUS_Q15_16, LEN_Q15_16))
		    break;
		oscillator_advance_next_impulse(osc, single_period_pulse_train);
		saeclib_error_e se = saeclib_circular_buffer_pushone(&osc->delay_line, &impulse);
		if (se != SAECLIB_ERROR_NOERROR)
		    printf("oscillator.c::oscillator_render_to_buffer: saeclib error\n");
	    }

	    printf("oscillator.c::oscillator_render_to_buffer: delay line contains %li pulses\n",
		   saeclib_circular_buffer_size(&osc->delay_line));

	    // render delay line to impulse train
	    int32_t impulse_train_q15[len];
	    memset(impulse_train_q15, 0, sizeof(impulse_train_q15));
	    for (int i = osc->delay_line.tail; i != osc->delay_line.head;
		 i = (((i + 1) == osc->delay_line.capacity) ? 0 : (i + 1))) {
		oscillator_impulse_event_t* d =
		    (void*)(osc->delay_line.data + (i * (osc->delay_line.elt_size)));

		const int sinc_phase = ((d->t_q15_16 >> (16 - SUBSAMPLE_PHASE_PRECISION)) &
					((1 << SUBSAMPLE_PHASE_PRECISION) - 1));

		int from_idx = 0;
		int to_idx = (d->t_q15_16 >> 16) - SINC_RADIUS;
		if (to_idx < 0) {
		    from_idx -= to_idx;
		    to_idx = 0;
		}

		for (; (to_idx < len) && (from_idx < SAMPLES_PER_SINC); to_idx++, from_idx++) {
		    impulse_train_q15[to_idx] += (int32_t)((blip_table_q_15[sinc_phase][from_idx] *
							    (int64_t)d->magnitude_q0_31) >> 31);
		}

	    }

	    for (int i = 0; i < len; i++) {
		buffer[i] = impulse_train_q15[i] << (29 - 15);
	    }

	    // slide the oscillator's phase back
	    osc->sample_phase_q15_16 -= (len << 16);

	    // slide back the phase of each wave in the
	    for (int i = osc->delay_line.tail; i != osc->delay_line.head;
		 i = (((i + 1) == osc->delay_line.capacity) ? 0 : (i + 1))) {
		oscillator_impulse_event_t* d =
		    (void*)(osc->delay_line.data + (i * (osc->delay_line.elt_size)));

		d->t_q15_16 -= (len << 16);
	    }

	    // TODO: integrate the impulse train to generate output.


	    break;
	}

        default: {
            break;
        }
    }
}
