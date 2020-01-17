import sys, os
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

sampling_rate = 50000.0

# parse argument
if len(sys.argv) == 1:
    print("error: path to *.h5 file must be specified as argument")
    quit()
elif len(sys.argv) > 2:
    print("error: too many arguments")
    quit()
if sys.argv[1] in ["-h", "--h", "-help", "--help"]:
    print("argument: path to *.h5 file to be opened")
    quit()
filepath = sys.argv[1]
# check if specified file path is valid
if not os.path.isfile(filepath):
    print("error: file " + filepath + " not found")
    quit()
# read data from file
dataframe = pd.read_hdf(filepath)
channel1_data = dataframe["channel1"]
channel2_data = dataframe["channel2"]
channel3_data = dataframe["channel3"]
# create time axis for the plots
time_scale = np.linspace(0, len(channel1_data)/sampling_rate, len(channel1_data))
# plot data of all 3 channels
plt.subplot(3, 1, 1)
plt.plot(time_scale, channel1_data)
plt.xlabel("time [s]")
plt.ylabel("voltage [V]")
plt.title("channel 1")
plt.subplot(3, 1, 2)
plt.plot(time_scale, channel2_data)
plt.xlabel("time [s]")
plt.ylabel("voltage [V]")
plt.title("channel 2")
plt.subplot(3, 1, 3)
plt.plot(time_scale, channel3_data)
plt.xlabel("time [s]")
plt.ylabel("voltage [V]")
plt.title("channel 3")
# increase vertical distance between subplots, default is 0.3
plt.subplots_adjust(hspace=1.2)
plt.show()
