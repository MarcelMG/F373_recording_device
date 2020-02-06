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

# read gain configuration setting from file
my_file = open(filepath, "rb")
# read first 3 bytes of file and decode bytes to string
cfg = my_file.read(3).decode("ascii")
my_file.close()
# define gain settings and corresponding gain factors as dictionary
gain_cfg_dic = {0:10E7,
                1:10E6,
                2:10E5,
                3:10E4,
                4:10E3,
                5:10E2}
gain = [0, 0, 0]
for j in range(3):
    gain_setting = int(cfg[j])
    if gain_setting in gain_cfg_dic:
        gain[j] = gain_cfg_dic[gain_setting]
    else:
        print("error reading gain setting from file.")
        quit()
print("gain factors: "+str(gain[0])+" "+str(gain[1])+" "+str(gain[2]))

# read data from file
data = np.fromfile(filepath, np.int16,offset=3) # read binary data in signed 16bit integer format, skip first 3 bytes which contain gain setting
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
for k in range(0, int(total_length/packet_length), 3):
    channel1_data = np.concatenate((channel1_data, data[ k*500 : (k+1)*500 ]))
    channel2_data = np.concatenate((channel2_data, data[ (k+1)*500 : (k+2)*500 ]))
    channel3_data = np.concatenate((channel3_data, data[ (k+2)*500 : (k+3)*500 ]))
# convert ADC voltage to photo-current(~= illuminance) by dividing by the amplifier's gain factor
channel1_data /= gain[0]
channel2_data /= gain[1]
channel3_data /= gain[2]
# create time axis for the plots
time_scale = np.linspace(0, len(channel1_data)/sampling_rate, len(channel1_data))
# plot data of all 3 channels
plt.subplot(3, 1, 1)
plt.plot(time_scale, channel1_data)
plt.xlabel("time [s]")
plt.ylabel("photo-current [A]")
plt.title("channel 1")
plt.subplot(3, 1, 2)
plt.plot(time_scale, channel2_data)
plt.xlabel("time [s]")
plt.ylabel("photo-current [A]")
plt.title("channel 2")
plt.subplot(3, 1, 3)
plt.plot(time_scale, channel3_data)
plt.xlabel("time [s]")
plt.ylabel("photo-current [A]")
plt.title("channel 3")
# increase vertical distance between subplots, default is 0.3
plt.subplots_adjust(hspace=1.2)
plt.show()
