#include "dsp_util.h"
#include "math.h"
#include <stdio.h>

typedef struct audio_state {
    int32_t phase;     // current phase in samples; Q24.8
    uint32_t period;   // period in samples; uQ24.8
    uint32_t volume;   // uQ.16
} audio_state_t;

static uint16_t thetas[(1 << 16)];
static int ti = 0;
void render_audio(uint16_t* target, audio_state_t* s, int len)
{
    for (int i = 0; i < len; i++) {
        uint16_t theta = ((s->phase << 13) / (s->period >> 2)) << 1;
        thetas[ti++] = theta;
        int32_t sample = (uint32_t)sine_u16(theta);
        sample = (sample * s->volume) >> 8;
        target[i] = ((uint16_t)sample + 0x8000);

        s->phase += (1 << 8);
        while (s->phase >= s->period) {
            s->phase -= s->period;
        }
    }
}

int main(int argc, char** argv)
{
    int j[] = {65535, 32767, 32768, 0};

    volatile uint16_t x = sine_u16(0x4000);

    audio_state_t osc = {.phase = 0, .period = 871, .volume = (16)};
    osc.period = 44332;

    const int BLOCKSIZE = 512;
    const int NBLOCKS = 16;

    uint16_t buf[BLOCKSIZE * NBLOCKS];

    for (int i = 0; i < NBLOCKS; i++) {
        render_audio(&buf[i * BLOCKSIZE], &osc, BLOCKSIZE);
    }

    for (int i = 0; i < BLOCKSIZE * NBLOCKS; i++) {
        printf("0x%04x, 0x%04x, ", buf[i], thetas[i]);
    }

    return 0;
}
