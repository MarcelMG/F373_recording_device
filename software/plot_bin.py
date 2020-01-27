import sys, os
import numpy as np
import matplotlib.pyplot as plt

packet_length = 500 # number of samples sent at once (in a 'packet') by STM32F373 recording device
sampling_rate = 50000.0

# parse argument
if len(sys.argv) == 1:
    print("error: path to .bin file must be specified as argument")
    quit()
elif len(sys.argv) > 2:
    print("error: too many arguments")
    quit()
if sys.argv[1] in ["-h", "--h", "-help", "--help"]:
    print("argument: path to .bin file to be opened")
    quit()
filepath = sys.argv[1]
# check if specified file path is valid
if not os.path.isfile(filepath):
    print("error: file " + filepath + " not found")
    quit()
# read data from file
data = np.fromfile(filepath, np.int16) # read binary data in signed 16bit integer format
data = data.astype(np.int32) # convert array to int32 (else the added offset would overflow the 16bit integer)
data += 32768 # add offset
data = data.astype(np.float) # convert to floating point value
data = data * 3.3 / 65536 # convert to Volts (3.3V is ADC reference voltage, 65536=2^16 is max. value)

# check if the data packets are complete, or incomplete if the recording has been stopped during the transmission
# of a packet. Each packet (for each channel) is <packet_length> samples large, so the length of the whole dataset should be a
# multiple of 3*<packet_length> samples. If this is not the case, truncate the dataset. This way we make sure that the recording
# of each channel is equally long
if( ( len(data) % ( 3 * packet_length ) ) > 0 ):
    print("note: dataset incomplete (don't worry, that's ok). contains %i samples" % len(data) )
    # truncate dataset
    new_length = len(data) - (len(data) % (3*packet_length))
    data = data[ 0 : new_length ]
    print("truncated dataset to %i samples" % len(data) )
    
    
# separate data from 3 different channels
# they alternate every <packet_length> samples e.g.
# <packet_length> samples of channel 1
# <packet_length> samples of channel 2
# <packet_length> samples of channel 3
# <packet_length> samples of channel 1
# <packet_length> samples of channel 2
# ... and so on
total_length = len(data)
channel1_data = np.array([])
channel2_data = np.array([])
channel3_data = np.array([])
for k in range(0, total_length/packet_length, 3):
    channel1_data = np.concatenate((channel1_data, data[ k*500 : (k+1)*500 ]))
    channel2_data = np.concatenate((channel2_data, data[ (k+1)*500 : (k+2)*500 ]))
    channel3_data = np.concatenate((channel3_data, data[ (k+2)*500 : (k+3)*500 ]))
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
