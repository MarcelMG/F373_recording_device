# own files
import import_bin
import import_pos
import f_theta_lens_correction
import misc
import simplegui_matplotlib_helpers as sgplt
import gui_layout
# external libraries/modules
import os
import PySimpleGUI as sg
import numpy as np
import matplotlib.pyplot as plt
from scipy import signal
from scipy.fft import fft
import natsort
import glob

debug_mode = False

# supress warnings from matplotlib about Toolbar, see: https://github.com/matplotlib/matplotlib/issues/15284
import warnings
warnings.filterwarnings(action="ignore", category=UserWarning)
# necessary to be able to remove buttons from matplotlib toolbar
plt.rcParams['toolbar'] = 'toolmanager'
# dictionary that contains which sensor/spectrum is connected to which ADC channel
channel_spectrum = { "ch1": "400-750nm (visible)",
                     "ch2": "1070nm (laser)",
                     "ch3": "1150-2600nm (thermal)" }

## main program ##

# let user select the folders which contain the signal data and position data files
if not debug_mode:
    window = sg.Window("Select Folders", gui_layout.browse_window_layout)
    while True:
        event, values = window.read()
        if event == "OK":
            window.close()
            break
        elif event in ["Cancel", None]:
            window.close()
            quit()
        else:
            pass
    # get file paths entered by user
    data_directory = values[0]
    pos_directory = values[1]
else: # during development, use fixed paths for quicker testing
    data_directory = "C:/Users/Marcel/OneDrive - student.kit.edu/Python/Versuch_06_02_20/2_Versuch_mit_Pos/optik"
    pos_directory = "C:/Users/Marcel/OneDrive - student.kit.edu/Python/Versuch_06_02_20/2_Versuch_mit_Pos/pos"

# fix paths and sort files numerically ascending (use natsort so order is correct, i.e. e.g. 1,..., 9, 10 instead of 1, 10, 11,... etc.)
data_files = natsort.natsorted( misc.fix_path( glob.glob(data_directory + "/*.bin") ) )
pos_files = natsort.natsorted( misc.fix_path( glob.glob(pos_directory + "/*.csv") ) )
# check if the number of signal data files and number of position data files is equal
if len(data_files) != len(pos_files):
    layout = [[sg.Text("ERROR: found "+str(len(data_files))+" files but "+str(len(pos_files))+" position files")],
              [sg.Cancel()]]
    window = sg.Window("Error", layout)
    while True:
        event, values = window.read()
        if event == "Cancel":
            window.close()
            quit()
        else:
            pass
        
# iterator for navigating between files
counter = 0 
max_counter_val = len(data_files) - 1

# create main window
window = sg.Window("Analyzer", gui_layout.main_window_layout, finalize=True)
figure_agg = None # object for integrating matplotlib plot inside window

#default view on startup
ch1_data, ch2_data, ch3_data, t = import_bin.import_bin(data_files[counter])
pos_x, pos_y = import_pos.import_pos(pos_files[counter])
pos_x, pos_y = f_theta_lens_correction.apply_correction(pos_x, pos_y)
window["info_text"].update("Signal data file: "+os.path.basename(data_files[counter])+" ("+str(len(ch1_data))+" samples per channel)\nPosition data file: "+os.path.basename(pos_files[counter])+" ("+str(len(pos_x))+" samples)")
plt.plot(t, ch1_data)
plt.gca().set(xlabel="time [s]", ylabel="photo-current [A]", title="Signal : "+channel_spectrum["ch1"])
fig = plt.gcf()
# integrate plot in GUI window
screen_w, screen_h = window.GetScreenDimensions()
fig.set_size_inches(0.75*screen_w/float(fig.get_dpi()), 0.5*screen_h/float(fig.get_dpi()))
#you have to play with this size (i.e. the 0.74) to reduce the movement error when the mouse hovers over the figure, it's close to canvas size (i.e. 0.75)
window["canvas"].set_size( (int(0.73*screen_w), int(0.73*screen_h)) )
figure_agg = sgplt.draw_figure_w_toolbar(window['canvas'].TKCanvas, fig, window['plot_controls'].TKCanvas)

## main infinite loop ##
while(True):
    # get GUI status
    event, values = window.read()
    if event != None:
        if debug_mode:
            print(event)
            print(values)
        # navigate between files with arrow buttons
        if event == "Left" and counter > 0:
            counter -= 1
        elif event == "Right" and counter < max_counter_val:
            counter += 1    
        # switch between object view (show single data file) and layer view (merge data files of one layer)
        if values["mergelayer"] == True and int(values["num_obj_per_layer"]) > 1: # layer view, i.e. merge object data
            # compute which files/objects belong in the current layer
            file_num_begin = int(counter)*int(values["num_obj_per_layer"])
            file_num_end = file_num_begin + int(values["num_obj_per_layer"])
            file_nums = [ x for x in range(file_num_begin, file_num_end) ]
            if debug_mode:
                print("files:", end="")
                print(file_nums)
                print("layer_num: "+str(counter))
            # show layer number and files/object numbers of layer in GUI
            window["info_text"].update("Layer # "+str(int(counter))+"\nfiles "+str(file_num_begin)+"-"+str(file_num_end-1))
            # import and merge data
            ch1_data = []
            ch2_data = []
            ch3_data = []
            pos_x = []
            pos_y = []
            t = []
            for i in range(len(file_nums)):
                # import signal and position data
                ch1, ch2, ch3, tmp = import_bin.import_bin(data_files[i])
                x, y = import_pos.import_pos(pos_files[i])
                x, y = f_theta_lens_correction.apply_correction(x, y)
                # concatenate data
                ch1_data += list(ch1)
                ch2_data += list(ch2)
                ch3_data += list(ch3)
                pos_x += list(x)
                pos_y += list(y)
            # compute new timescale for concatenated signal
            t = np.linspace(0, len(ch1_data)/50000.0, len(ch1_data))
        else: # object view, show only data of one file/object
            # show file names in GUI
            window["info_text"].update("Signal data file: "+os.path.basename(data_files[counter])+" ("+str(len(ch1_data))+" samples per channel)\nPosition data file: "+os.path.basename(pos_files[counter])+" ("+str(len(pos_x))+" samples)")
            # import signal and position data
            ch1_data, ch2_data, ch3_data, t = import_bin.import_bin(data_files[counter])
            pos_x, pos_y = import_pos.import_pos(pos_files[counter])
            pos_x, pos_y = f_theta_lens_correction.apply_correction(pos_x, pos_y)
        
        # select which channel to plot based on user selection in GUI
        if values["ch1_selection"] == True:
            sig = ch1_data
            ch_name = channel_spectrum["ch1"]
        elif values["ch2_selection"] == True:
            sig = ch2_data
            ch_name = channel_spectrum["ch2"]
        elif values["ch3_selection"] == True:
            sig = ch3_data
            ch_name = channel_spectrum["ch3"]
        # apply notch-, low- or high-pass filterto signal if selected via GUI
        if values["lowpass"] == True:
            fc = float(values["lpfreq"])
            N = int(values["lporder"])
            fs = 50000.0 # sampling rate of STM32F373 recording device
            sos = signal.butter(N, fc, "lp", fs=fs, output="sos")
            sig = signal.sosfilt(sos, sig)
        if values["highpass"] == True:
            fc = float(values["hpfreq"])
            N = int(values["hporder"])
            fs = 50000.0 # sampling rate of STM32F373 recording device
            sos = signal.butter(N, fc, "hp", fs=fs, output="sos")
            sig = signal.sosfilt(sos, sig)
        if values["bandstop"] == True:
            f1 = float(values["bandstopfreq1"])
            f2 = float(values["bandstopfreq2"])
            N = int(values["bandstoporder"])
            fs = 50000.0 # sampling rate of STM32F373 recording device
            sos = signal.butter(N, (f1, f2), "bandstop", fs=fs, output="sos")
            sig = signal.sosfilt(sos, sig)
        # binarize signal via threshold if selected via GUI
        if event=="threshold":
            # values["threshold_val"] is between 0.0 and 1.0
            threshold = values["threshold_val"] * ( max(sig) - min(sig) ) + min(sig) 
            for i in range(len(sig)):
                if sig[i] < threshold:
                    sig[i] = 0.0
                else:
                    sig[i] = 1.0
        # plot different type of plot, depending on user selection via GUI
        if values["plotsig"] == True:
            plt.plot(t, sig)
            plt.gca().set(xlabel="time [s]", ylabel="photo-current [A]", title="Signal "+ch_name)
        elif values["plotfft"] == True:
            fs = 50000 # sampling frequency
            plt.magnitude_spectrum(sig, Fs=fs, scale="dB")
            plt.gca().set(xlabel="frequency [Hz]", ylabel="magnitude [dB]", title="Signal Spectrum (FFT): "+ch_name)
        elif values["plotpos"] == True:
            plt.scatter(pos_x, pos_y, linewidths=0, c="r", s=2)
            plt.axis('square')
            plt.gca().set(xlabel="x-coordinate [mm]", ylabel="y-coordinate [mm]", title="2D position of laser scan")
        elif values["plotheat"] == True:
            # upsample (i.e. interpolate) the position data so we have same number of samples as in signal data
            pos_x = signal.resample(pos_x, len(sig))
            pos_y = signal.resample(pos_y, len(sig))
            if event=="threshold":
                colormap = plt.cm.binary
            else:
                colormap = plt.cm.RdBu
            plt.scatter(pos_x, pos_y, c=sig, linewidths=0, s=5, cmap=colormap)
            plt.axis('square')
            plt.gca().set(xlabel="x-coordinate [mm]", ylabel="y-coordinate [mm]", title="2D heatmap of laser scan")
            plt.colorbar(label='photo-current [A]', orientation="vertical", format="%1.2E")
        # increment plot figure in GUI window
        fig = plt.gcf()
        screen_w, screen_h = window.GetScreenDimensions()
        fig.set_size_inches(0.75*screen_w/float(fig.get_dpi()), 0.5*screen_h/float(fig.get_dpi()))
        #you have to play with this size (i.e. the 0.74) to reduce the movement error when the mouse hovers over the figure, it's close to canvas size (i.e. 0.75)
        window["canvas"].set_size( (int(0.74*screen_w), int(0.74*screen_h)) )
        # clear previous plot if necessary
        if figure_agg != None:
            sgplt.delete_figure_agg(figure_agg)
        # draw figure in window canvas
        figure_agg = sgplt.draw_figure_w_toolbar(window['canvas'].TKCanvas, fig, window['plot_controls'].TKCanvas)

    else:
        window.close()
        quit()
