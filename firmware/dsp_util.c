#include "dsp_util.h"

/**
 * AN IDEA:
 * Might be good to have a standardized pipeline of functions
 *
 * void function_node(struct_t* state, uint16_t* data_in, uint16_t* data_out);
 * void source_node(struct_t* state, uint16_t* data_out)
 *
 * Chain them together like
 *     square_wave_source(&sq_state, source_buf);
 *     low_pass_node(&low_pass_state, source_buf, output_buf);
 */

/**
 * Code heavily taken from
 *     https://www.nullhardware.com/blog/fixed-point-sine-and-cosine-for-embedded-systems/
 */
int16_t sine_u16(uint16_t _theta)
{
    // round the calculation into the first quadrant of the circle
    uint16_t c = _theta & 0x8000;
    uint16_t theta = _theta & 0x7fff;

    // now, the ranges [0x0000, 0x7fff] and [0x8000, 0xffff] are superimposed;
    // e.g. theta = 0x8000 -> theta = 0x0000, theta = 0x800a -> theta = 0x000a, etc.
    if(theta & 0x4000)
        theta = 0x8000 - theta;

    const uint32_t theta_q = 14;
    const uint32_t A_q     = 30;
    const uint32_t B_q     = 31;
    const uint32_t C_q     = 2 + (32 - theta_q);

    const uint32_t A = (1.56971863421 * (1ul << A_q)) + 0.5;
    const uint32_t B = (0.63943726841 * (1ull << B_q)) + 0.5;
    const uint32_t C = (0.0697186342  * (1ul << C_q)) + 0.5;

    // calculate C * theta with output in Q.31 precision
    uint32_t y = (C * ((uint32_t)theta)) >> 3;

    // calculate (B - C * theta^2) with output in Q.31 precision
    y = B - ((uint32_t)theta * (y >> theta_q));

    // calculate theta^2 * (B - C * theta^2) with output in Q.31 precision
    y = (uint32_t)theta * (y >> theta_q);
    y = (uint32_t)theta * (y >> theta_q);

    // calculate A - (theta^2 * (B - C * theta^2)) with output in Q.30 precision
    y = A - (y >> (31 - A_q));

    // output of this stage is Q.31 precision.
    y = (uint32_t)theta * (y >> (theta_q - 1));

    // round off result and shift down into Q1.14
    y = (y + (1ul << (31 - 15 - 1))) >> (31 - 14);

    if (c) return -y;
    else return y;
}
