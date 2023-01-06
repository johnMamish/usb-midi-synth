#!/usr/bin/python3

import matplotlib.pyplot as plt
import math
import numpy as np

phase = 0.0

def blackman_window(n, window_radius):
    return (0.42 +
            (0.5  * math.cos((2. * math.pi * n) / (2 * window_radius))) +
            (0.08 * math.cos((4. * math.pi * n) / (2 * window_radius))))

def hamming_window(n, window_radius):
    return (0.54 + (0.46 * math.cos((2. * math.pi * n) / (2*window_radius))))

def windowed_sinc(n, window_radius):
    if (abs(n) > window_radius): return 0
    else: return (np.sinc(n) * blackman_window(n, window_radius))

sincx = np.array([(x + .25) for x in np.arange(-5, 5.0, 0.5)])
plt.plot(sincx, np.sinc(sincx), 'o')
plt.show()
for sumrange in [8, 16, 32, 64, 128, 256]:
    sums = [sum(np.sinc(np.arange(-sumrange, sumrange) + phi)) for phi in np.arange(0, 1, 0.01)]
    sums = [sum([windowed_sinc(x+phi, sumrange) for x in np.arange(-sumrange, sumrange)]) for phi in np.arange(0, 1, 0.04)]
    plt.plot(sums, label=f"range = {sumrange:9.0f}")

plt.legend()
plt.show()
