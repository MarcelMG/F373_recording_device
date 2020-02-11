import matplotlib
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
import matplotlib.pyplot as plt

matplotlib.use('TkAgg')


# see: https://github.com/PySimpleGUI/PySimpleGUI/blob/master/DemoPrograms/Demo_Matplotlib_Embedded_Toolbar.py
def draw_figure_w_toolbar(canvas, fig, canvas_toolbar):
    if canvas.children:
        for child in canvas.winfo_children():
            child.destroy()
    if canvas_toolbar.children:
        for child in canvas_toolbar.winfo_children():
            child.destroy()
    figure_canvas_agg = FigureCanvasTkAgg(fig, master=canvas)
    figure_canvas_agg.draw()
    # remove unnecessary and confusing buttons from matplotlib toolbar
    toolbar = Toolbar(figure_canvas_agg, canvas_toolbar)
    toolbar.children['!button2'].pack_forget()
    toolbar.children['!button3'].pack_forget()
    toolbar.children['!button6'].pack_forget()
    toolbar.update()
    figure_canvas_agg.get_tk_widget().pack(side='bottom', fill='both', expand=1)
    return figure_canvas_agg

class Toolbar(NavigationToolbar2Tk):
    def __init__(self, *args, **kwargs):
        super(Toolbar, self).__init__(*args, **kwargs)
        
# see: https://github.com/PySimpleGUI/PySimpleGUI/blob/master/DemoPrograms/Demo_Matplotlib_Browser.py
def draw_figure(canvas, figure, loc=(0, 0)):
    figure_canvas_agg = FigureCanvasTkAgg(figure, canvas)
    figure_canvas_agg.draw()
    figure_canvas_agg.get_tk_widget().pack(side='top', fill='both', expand=1)
    return figure_canvas_agg
def delete_figure_agg(figure_agg):
    figure_agg.get_tk_widget().forget()
    plt.close('all')
