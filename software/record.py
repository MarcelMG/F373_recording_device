import serial
import time
import sys, os

def main():
    # on Windows, check the Device-Manager to findout which port is the STM32F373, it should be "COMX" with a number X
    # on Linux, execute "dmesg" in the commandline to findout which path is the STM32F373, it should be either "/dev/ttyACMX" or "/dev/ttyUSBX" with a number X
    # on Linux make sure the user has permission to access the serial ports
    serialport_path = ""
    verbose = False # option for verbose output

    # parse script argument
    # if no argument is specified throw error
    if len(sys.argv) == 1:
        print("error: path to serial port must be specified, e.g. /dev/ttyACM0 on Linux or COM1 on Windows")
        quit()
    # if 1 argument is specified
    elif len(sys.argv) == 2:
        # display help
        if sys.argv[1] in ["-h", "--h", "-help", "--help"]:
            print("argument 1: path to serial port, e.g. /dev/ttyACM0 on Linux or COM1 on Windows")
            print("optional: -v or --v or -verbose as 2nd argument for verbose output")
            quit()
        # serial port path as argument
        else:
            serialport_path = sys.argv[1]
    # if 2 arguments are specified
    elif len(sys.argv) == 3:
        # serial port path as first argument
        serialport_path = sys.argv[1]
        # check for verbose option
        if sys.argv[2] in ["-v", "--v", "-verbose", "--verbose"]:
            verbose = True
    # if >2 arguments are specified
    else:
        print("error: too many arguments")
        quit()

    
    # try to open serial port until successfull
    if(verbose == True):
        print("opening serial port " + serialport_path + " ...")
    while True:
        try:
            # baud rate, parity and stopbits settings etc. don't matter since this is a USB virtual(!) COM port not a real serial port
            serialPort = serial.Serial(port=serialport_path, timeout=0.01)
        except:
            continue
        else:
            break
    # the OS uses FIFO buffers for the serial communication interface
    # clear the buffers at startup to make sure there's no garbage data left in the buffers
    serialPort.reset_input_buffer()
    serialPort.reset_output_buffer()
    print("enter gain factor config for amplifier for each channel.\nconfig - factor\n   0   - 10^7 V/A\n   1   - 10^6 V/A\n   2   - 10^5 V/A\n   3   - 10^4 V/A\n   4   - 10^3 V/A\n   5   - 10^2 V/A")
    ch_gain = [0, 0, 0]
    for i in range(3):
        prompt_str = "enter gain config (0-5) for channel " + str(i+1) + " : "
        ch_gain[i] = int(input(prompt_str))
        if ch_gain[i] not in range(6):
            print("error: value out of range (0-5)")
            serialPort.close()
            quit()
    gain_command = bytearray("G" + str(ch_gain[0]) + str(ch_gain[1]) + str(ch_gain[2]) + "\n", "ascii")
    verify_response = "OK" + str(ch_gain[0]) + str(ch_gain[1]) + str(ch_gain[2]) + "\n"
    serialPort.write(gain_command)
    response = serialPort.read_until()
    if verbose == True:
        print("sent command: " + gain_command.decode("ascii")),
        print("got response: " + response.decode("ascii")),
    if response.decode("ascii") == verify_response:
        if verbose == True:
            print("gain setting successful.")
    else:
        print("error occured during gain setting, wrong or no response received.")
        serialPort.close()
        quit()
    
    ## infinite loop ##
    try:
        while(True):
            # check if a file named "data_x.bin" already exists (in the same directory as this script)
            # name files "data_1.bin", "data_2.bin" etc. with ascending number
            filename = ""
            for i in range(99999):
                filename = "data_" + str(i) + ".bin"
                if not filename in os.listdir("./"):
                    break
            # if we receive no data for a duration longer than <layer_timeout>, we consider that the printing of one layer is finished
            # and we store the data to a file and wait until new data is available which signals the beginning of the next layer
            layer_timeout = 0.01
            adc_data = bytearray()
            layer_finished = False
            layer_started = False
            if(verbose == True):
                print("waiting for data...")
            # wait until there is some data available
            while( serialPort.in_waiting < 1 ):
                pass
            if(verbose == True):
                print("start reading data...")
            layer_started = True
            # save timestamp
            t1 = time.clock()
            t0 = time.clock()
            # record data
            while ( layer_finished == False ):
                # if data available
                if ( serialPort.in_waiting > 0 ):
                    # take timestamp (for timeout detection)
                    t1 = time.clock()
                    # read available data
                    adc_data += serialPort.read(serialPort.in_waiting)
                # no data available to read
                else:
                    # check layer timeout
                    if( (time.clock() - t1) > layer_timeout ):
                        layer_finished = True
            # save timestamp to measure transmission time and compute data rate
            t2 = time.clock()
            # compute data rate
            throughPut = len(adc_data)/(t2-t0)
            # verbose output
            if(verbose == True):
                print("received %d samples, %d bytes" %(len(adc_data)/6, len(adc_data))),
                print (" time elapsed: %f s" % (t2 -t0)),
                print (" data rate was %d Bytes/s " % throughPut)
            # open file for writing in binary mode
            output_file = open(filename, "wb")
            # write current gain settings to file
            output_file.write(str(ch_gain[0]) + str(ch_gain[1]) + str(ch_gain[2]) + "\n")
            # write recorded data to file in raw binary format
            output_file.write(adc_data)
            output_file.close()
            if(verbose == True):
                print("data written to file " + filename)
    # if program is aborted by Ctrl+C close the serial port before terminating
    except KeyboardInterrupt:
        if(verbose == True):
            print("\nserial port closed, exiting.")
        serialPort.close()
        sys.exit() # stop and exit script
            
	
if __name__ == '__main__':
    main()
