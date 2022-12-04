#!/usr/bin/python3

import matplotlib.ticker as ticker
import matplotlib.pyplot as plt
import sys

def to_hex(x, pos):
    return '0x%x' % int(x)

with open(sys.argv[1], 'r') as f:
    vals = [int(x.strip(), 0) for x in f.read().split(', ') if len(x) != 0]

plt.plot(vals[::2])
axes = plt.gca()
#axes.get_yaxis().set_major_locator(ticker.MultipleLocator(1))
fmt = ticker.FuncFormatter(to_hex)
axes.get_yaxis().set_major_formatter(fmt)
plt.show()
