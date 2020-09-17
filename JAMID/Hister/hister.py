import numpy as np
from matplotlib import pyplot as plt
import sys

if len(sys.argv) <= 1:
    sys.exit("enter log file's name and number of bins in the form 'python3 hister.py <log_file_name> <number_of_bins>'")

BINS = int(sys.argv[2])

ff = open(sys.argv[1])

ff = ff.read()
ff = ff.split("\n")
ff = ff[1:] if len(ff) > 0 else ff
ff = ff[:-1] if len(ff) > 0 else ff
ff = list(map(lambda f: f.split(","), ff))

mm = list(filter(lambda f: f[1] == '0', ff))
mm = list(map(lambda f: float(f[0]), mm))

hh = list(filter(lambda f: f[1] == '1', ff))
hh = list(map(lambda f: float(f[0]), hh))

tt = list(map(lambda f: float(f[0]), ff))

plt.subplot(311)
m_hist, m_bins = np.histogram(mm, bins=BINS)
plt.plot(m_bins[:-1], m_hist)
plt.title("misses count per time")

plt.subplot(312)
h_hist, h_bins = np.histogram(hh, bins=BINS)
plt.plot(h_bins[:-1], h_hist)
plt.title("hits count per time")

plt.subplot(313)
_, t_bins = np.histogram(tt, bins=BINS)
rates = [m / (m + h) * 100 if m + h != 0 else 0 for h, m in zip(h_hist, m_hist)]
plt.plot(t_bins[:-1], rates)
plt.title("miss rate per time")


plt.show()
