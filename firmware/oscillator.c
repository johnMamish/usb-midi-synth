/**
 *
 */

#include "oscillator.h"


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

void render_audio(oscillator_t* osc, uint8_t* buffer, size_t len)
{
    switch (osc->type) {
        case OSCILLATOR_TYPE_SINE: {
            for (int i = 0; i < len; i++) {

            }
            break;
        }

        default: {
            break;
        }
    }
}
