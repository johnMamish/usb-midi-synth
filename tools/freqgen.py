#!/usr/bin/python3

import argparse
import math
import matplotlib.pyplot as plt
import numpy as np
import wave

def get_ith_impulse_offset(impulses, period, index):
    return (impulses[index % len(impulses)][0] + (period * np.floor(index / len(impulses))))

def blit_wave(period, num_samps, wave_style, graph=False):
    DO_PRINT = False
    BLIT_RADIUS = 10
    SCALE = 0.8
    LEAK_RATE = 0.0001

    if (wave_style == "square"):
        impulse_positions = [(period / 4, 1), (3*period / 4, -1)]
        dc_offset = 0
        initial_integrator_value = -SCALE / 2
    elif (wave_style == "sawtooth"):
        impulse_positions = [(period / 2, -1)]
        dc_offset = (1 / period)
        initial_integrator_value = 0.
    else:
        raise ValueError(f"wave_style {wave_style} not supported")

    phase = 0
    impulse_train = np.empty(num_samps)

    # generate impulse train
    for i in range(num_samps):
        sample = 0

        # Starting at the bottom end of one period, go backwards until we reach an impulse that's
        # outside our BLIT radius.
        j = -1
        if (DO_PRINT): print(f"i = {i}, phase = {phase:6.2f}, period = {period:6.2f}")
        while (True):
            offset = (phase - get_ith_impulse_offset(impulse_positions, period, j))
            if (DO_PRINT): print(f"loop 1: checking sample from impulse offset {offset}", end='')
            if (not(offset < BLIT_RADIUS)):
                if (DO_PRINT): print(" BREAK")
                break
            elif ((offset >= -BLIT_RADIUS) and (offset < BLIT_RADIUS)):
                sample += np.sinc(offset) * impulse_positions[j % len(impulse_positions)][1]
            else:
                pass
            if (DO_PRINT): print()
            sample += np.sinc(offset) * impulse_positions[j % len(impulse_positions)][1]
            j -= 1

        # Starting at the bottom end of one period, now go FORWARDS doing the same thing.
        j = 0
        while (True):
            offset = (phase - get_ith_impulse_offset(impulse_positions, period, j))
            if (DO_PRINT): print(f"loop 2: checking sample from impulse offset {offset}", end='')
            if (offset < -BLIT_RADIUS):
                if (DO_PRINT): print(" BREAK")
                break
            elif ((offset >= -BLIT_RADIUS) and (offset < BLIT_RADIUS)):
                sample += np.sinc(offset) * impulse_positions[j % len(impulse_positions)][1]
            else:
                pass
            if (DO_PRINT): print()
            j += 1

        if (DO_PRINT): print()
        impulse_train[i] = sample * SCALE

        phase += 1
        if (phase > period):
            phase -= period

    # integrate impulse train
    impulse_train_integrated = np.empty(num_samps)
    integrator = initial_integrator_value
    for i in range(num_samps):
        integrator += impulse_train[i]
        impulse_train_integrated[i] = integrator

    if (graph):
        plt.plot(impulse_train)
        plt.show()
        plt.plot(impulse_train_integrated)
        plt.show()

    return impulse_train_integrated

if __name__ == "__main__":
    ap = argparse.ArgumentParser(description="Test program for generating bandwidth-limited waves with BLITs")
    ap.add_argument("--wave-freq", type=float, action="store", default=466.16,
                    help='Wave frequency to synthesize')
    ap.add_argument("--num-samps", dest="num_samps", type=int, action="store", default=96000,
                    help='Number of samples to synthesize')
    ap.add_argument('--show-graph', dest='show_graph', action='store_true',
                    help='Display a matplotlib graph of the signal that will be written to the wav file')
    ap.add_argument('--output-file', dest='output_file', action='store', default='a.wav',
                    help='Wav file to write to')
    ap.add_argument('--amplitude', dest='amplitude', type=float, action='store', default=32768,
                    help='Amplitude of the sine wave')
    ap.add_argument("--wave-style", dest="wave_style", type=str, choices = ["square"], default="square")

    args = ap.parse_args()

    FSAMP = 48000

    # generate wave
    period = FSAMP / args.wave_freq
    output_wave = blit_wave(period, args.num_samps, args.wave_style, args.show_graph)
    output_wave = output_wave * 32767.0
    output_wave_ints = np.clip(output_wave, -32768., 32767.).astype(np.int16)
    output_wave_bytes = output_wave_ints.tobytes()

    with wave.open(args.output_file, "wb") as wav_file:
        # Configure wav file to be 16-bit mono at 48ksps
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(FSAMP)
        wav_file.setnframes(args.num_samps)

        # Write to file
        wav_file.writeframesraw(output_wave_bytes)
