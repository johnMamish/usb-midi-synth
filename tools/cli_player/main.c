/**
 * CLI program that uses port audio to synthesize audio
 */

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "portaudio.h"

#include "oscillator.h"

#define NUM_SECONDS   (4)
#define SAMPLE_RATE   (48000)

#define BUFFER_LEN (512)

typedef struct {
    oscillator_t osc;
} user_t;

/**
 * Converts a buffer full of q2_29 fixed-point integers to q31.
 */
static void oscillator_buffer_to_int32(int32_t* out, int32_t* in, size_t len)
{
    for (int i = 0; i < len; i++) {
	out[i] = in[i] << 2;
    }
}

/**
 * This routine will be called by the PortAudio engine when audio is needed.
 * It may called at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
 */
static int patestCallback(const void *inputBuffer, void *outputBuffer,
			  unsigned long framesPerBuffer,
			  const PaStreamCallbackTimeInfo* timeInfo,
			  PaStreamCallbackFlags statusFlags,
			  void *userData)
{
    (void) inputBuffer; // Prevent unused variable warning.
    int32_t* out = (int32_t*)outputBuffer;
    user_t* user = (user_t*)userData;

    // render oscillator audio
    static int32_t temp_buffer[BUFFER_LEN];
    oscillator_render_to_buffer(&user->osc, temp_buffer, BUFFER_LEN);

    // render it to the output buffer.
    oscillator_buffer_to_int32(out, temp_buffer, BUFFER_LEN);

    return 0;
}

int main(void)
{
    PaStream *stream;
    PaError err;

    printf("CLI synth tester.\n");

    user_t user;
    oscillator_setup_tables();
    oscillator_initialize(&user.osc, SAMPLE_RATE);
    oscillator_set_frequency(&user.osc, ((int)(440.0 * 32768)));
    user.osc.type = OSCILLATOR_TYPE_SQUARE;
    user.osc.volume_q15_16 = (int)((1 << 16) * 0.3);

    // Initialize library before making any other calls.
    err = Pa_Initialize();
    if(err != paNoError) goto error;
    err = Pa_OpenDefaultStream(&stream,
			       0,          // no input channels
			       1,          // mono output
			       paInt32,    // 32 bit floating point output
			       SAMPLE_RATE,
			       BUFFER_LEN,        // frames per buffer
			       patestCallback,
			       &user);

    if( err != paNoError ) goto error;

    err = Pa_StartStream( stream );
    if( err != paNoError ) goto error;

    // while the portaudio
    double freqs[] = {
	110.0, 123.47, 138.59, 164.81, 185.00, 220.0,
	110.0, 123.47, 138.59, 164.81, 185.00, 220.0,
	110.0, 123.47, 138.59, 164.81, 185.00, 220.0,
	110.0, 123.47, 138.59, 164.81, 185.00, 220.0,
	110.0, 123.47, 138.59, 164.81, 185.00, 220.0,
	110.0, 123.47, 138.59, 164.81, 185.00, 220.0,

	123.47, 138.59, 146.83, 164.81, 185.00, 220.0,
	123.47, 138.59, 146.83, 164.81, 185.00, 220.0,
	123.47, 138.59, 146.83, 164.81, 185.00, 220.0,
	123.47, 138.59, 146.83, 164.81, 185.00, 220.0,
	123.47, 138.59, 146.83, 164.81, 185.00, 220.0,
	123.47, 138.59, 146.83, 164.81, 185.00, 220.0,

	8*123.47, 8*138.59, 8*146.83, 8*164.81, 8*185.00, 8*220.0,
	8*123.47, 8*138.59, 8*146.83, 8*164.81, 8*185.00, 8*220.0,
	8*123.47, 8*138.59, 8*146.83, 8*164.81, 8*185.00, 8*220.0,
	8*123.47, 8*138.59, 8*146.83, 8*164.81, 8*185.00, 8*220.0,
	8*123.47, 8*138.59, 8*146.83, 8*164.81, 8*185.00, 8*220.0,
	8*123.47, 8*138.59, 8*146.83, 8*164.81, 8*185.00, 8*220.0,

	8 *110.0, 8*123.47, 8*138.59, 8*164.81, 8*185.00, 8*220.0,
	8 *110.0, 8*123.47, 8*138.59, 8*164.81, 8*185.00, 8*220.0,
	8 *110.0, 8*123.47, 8*138.59, 8*164.81, 8*185.00, 8*220.0,
	8 *110.0, 8*123.47, 8*138.59, 8*164.81, 8*185.00, 8*220.0,
	8 *110.0, 8*123.47, 8*138.59, 8*164.81, 8*185.00, 8*220.0,
	8 *110.0, 8*123.47, 8*138.59, 8*164.81, 8*185.00, 8*220.0,
    };
    for (int i = 0; i < sizeof(freqs) / sizeof(freqs[0]); i++) {
        oscillator_set_frequency(&user.osc, ((int)((freqs[i]) * 32768)));
	Pa_Sleep(200);
    }

    err = Pa_StopStream( stream );
    if( err != paNoError ) goto error;
    err = Pa_CloseStream( stream );
    if( err != paNoError ) goto error;
    Pa_Terminate();
    printf("Test finished.\n");
    return err;

error:
    Pa_Terminate();
    fprintf( stderr, "An error occurred while using the portaudio stream\n" );
    fprintf( stderr, "Error number: %d\n", err );
    fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ));
    return err;
}
