import PySimpleGUI as sg
import icons

left_btn = sg.Button('', image_data=icons.arrow_left_base64, button_color=sg.COLOR_SYSTEM_DEFAULT, border_width=0, image_subsample=1, key='Left')
right_btn = sg.Button('', image_data=icons.arrow_right_base64, button_color=sg.COLOR_SYSTEM_DEFAULT, border_width=0, image_subsample=1, key='Right')
pltsig_btn = sg.Radio("Plot Signal", "plot_type_selector", auto_size_text=True, default=True, key="plotsig", enable_events=True)
pltfft_btn = sg.Radio("Plot Spectrum (FFT)", "plot_type_selector", auto_size_text=True, key="plotfft", enable_events=True)
plotpos_btn = sg.Radio("Plot 2D-Position", "plot_type_selector", auto_size_text=True, key="plotpos", enable_events=True)
plotheat_btn = sg.Radio("Plot Heatmap (Signal+2D-Position)", "plot_type_selector", auto_size_text=True, key="plotheat", enable_events=True)
ch1_btn = sg.Radio("Channel 1: 400-750nm (visible)", "channel_selection", auto_size_text=True, key="ch1_selection", default=True, enable_events=True)
ch2_btn = sg.Radio("Channel 2: 1070nm (laser)", "channel_selection", auto_size_text=True, key="ch2_selection", enable_events=True)
ch3_btn = sg.Radio("Channel 3: 1150-2600nm (thermal)", "channel_selection", auto_size_text=True, key="ch3_selection", enable_events=True)
lp_chkbox = sg.Checkbox("Apply Low-Pass Filter to Signal: ", default=False, key="lowpass", enable_events=True)
hp_chkbox = sg.Checkbox("Apply High-Pass Filter to Signal", default=False, key="highpass", enable_events=True)
lp_dropdwn = sg.Combo(["1", "2", "3", "4", "5"], size=(2,1), default_value="1", key="lporder")
hp_dropdwn = sg.Combo(["1", "2", "3", "4", "5"], size=(2,1), default_value="1", key="hporder")
lp_input = sg.Input(key="lpfreq", size=(6,1), default_text="100")
hp_input = sg.Input(key="hpfreq", size=(6,1), default_text="100")
thresh_btn = sg.Button(button_text="Threshold Signal (binarize)", auto_size_button=True, key="threshold")
thresh_slider = sg.Slider((0,1), resolution=0.01, orientation="horizontal", key="threshold_val")

main_window_layout = [ [pltsig_btn, pltfft_btn, plotpos_btn, plotheat_btn],
                       [sg.Canvas(size=(640,480), key='canvas')],
                       [sg.Canvas(key='plot_controls')],
                       [ch1_btn, sg.Text("", size=(20,1)), lp_chkbox, lp_dropdwn, sg.Text("Order", auto_size_text=True), lp_input, sg.Text("Frequency [Hz]", auto_size_text=True)],
                       [ch2_btn, sg.Text("", size=(23,1)), hp_chkbox, hp_dropdwn, sg.Text("Order", auto_size_text=True), hp_input, sg.Text("Frequency [Hz]", auto_size_text=True)],
                       [ch3_btn, sg.Text("", size=(20,1)), thresh_btn, thresh_slider],
                       [sg.Text(text="", size=(70,2), key="info_text")],
                       [left_btn, right_btn] ]

browse_window_layout = [[sg.Text("raw signal data file (*.bin) folder")],
                        [sg.Input(), sg.FolderBrowse()],
                        [sg.Text("reconstructed position file (*.csv) folder")],
                        [sg.Input(), sg.FolderBrowse()],
                        [sg.OK(), sg.Cancel()] ]
