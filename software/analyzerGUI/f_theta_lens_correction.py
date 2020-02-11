import numpy as np
from scipy import interpolate

def apply_correction(x, y):
    coords = np.array([x, y])
    # rotate "image" coordinates
    theta = np.radians(-90)
    r = np.array(((np.cos(theta), -np.sin(theta)),
                  (np.sin(theta), np.cos(theta))))
    coords = r.dot(coords)
    # add offset
    coords[1] = coords[1] + 2*32128
    # convert bit values to mm
    bit2mm_x = interpolate.interp1d([20084, 45752], [-60, 60])
    bit2mm_y = interpolate.interp1d([18633, 45628], [-60, 60])
    x = bit2mm_x(coords[0])
    y = bit2mm_y(coords[1])
    coords = np.array([x, y])
    # stützpunkte aus Datei <BsKonst.cfg>
    x_points = np.array([-96.2003, -63.8397, -31.2486,   0.8689,  32.6158,  64.7223,
           104.4753, -98.5149, -64.1382, -31.398 ,   0.4182,  32.2705,
            65.0634,  98.9137, -98.4666, -63.8027, -31.2777,   0.5971,
            32.4931,  65.079 ,  99.4007, -97.8446, -63.8265, -31.1757,
             0.6935,  32.5471,  65.2605,  99.0493, -97.248 , -63.7504,
           -31.071 ,   0.7481,  32.7387,  65.1997, 100.2795, -97.2403,
           -63.7549, -30.7998,   0.9595,  32.7984,  65.6341,  99.5146,
           -99.396 , -63.1618, -31.5285,   0.8794,  32.851 ,  65.529 ,
           100.4411])
    y_points = np.array([ 105.603 ,  105.6714,  102.2454,  101.2397,  102.5164,  107.3984,
             98.3851,   75.2998,   70.081 ,   68.0693,   67.5832,   68.333 ,
             70.7767,   75.984 ,   39.3765,   35.8168,   35.2874,   35.0308,
             35.6725,   36.7275,   39.6583,    2.6205,    2.8976,    3.0247,
              3.233 ,    3.3641,    3.7543,    3.5745,  -32.5362,  -30.3646,
            -29.1728,  -28.6602,  -28.7369,  -29.391 ,  -31.4097,  -68.8384,
            -64.2383,  -61.8723,  -61.027 ,  -61.4131,  -63.2646,  -68.3698,
           -100.2273, -100.315 ,  -95.9981,  -95.0378,  -95.9238,  -99.8928,
            -95.093 ])
    # koordinaten, 7x7 Feld mit 30x30mm Feldern, wie die Stützpunkte oben
    x_grid = np.array([-90., -60., -30.,   0.,  30.,  60.,  90.,
                  -90., -60., -30.,   0.,  30.,  60.,  90.,
                  -90., -60., -30.,   0.,  30.,  60.,  90.,
                  -90., -60., -30.,   0.,  30.,  60.,  90.,
                  -90., -60., -30.,   0.,  30.,  60.,  90.,
                  -90., -60., -30.,   0.,  30.,  60.,  90.,
                  -90., -60., -30.,   0.,  30.,  60.,  90.])
    y_grid = np.array([90., 90., 90., 90., 90., 90., 90.,
                  60., 60., 60., 60., 60., 60., 60.,
                  30., 30., 30., 30., 30., 30., 30.,
                  0., 0., 0., 0., 0., 0., 0.,
                  -30., -30., -30., -30., -30., -30., -30.,
                  -60., -60., -60., -60., -60., -60., -60.,
                  -90., -90., -90., -90., -90., -90., -90.])
    # spline-interpolation zwischen den stützpunkten
    x_spline = interpolate.interp2d(x_points, y_points, x_grid, kind='quintic')
    y_spline = interpolate.interp2d(x_points, y_points, y_grid, kind='quintic')
    # Spline F-Theta Korrektur auf Beispieldaten anwenden
    pos_corrected_x = np.array([])
    pos_corrected_y = np.array([])
    for i in range(len(coords[0])):
        pos_corrected_x = np.append(pos_corrected_x, x_spline(coords[0][i], coords[1][i]) )
        pos_corrected_y = np.append(pos_corrected_y, y_spline(coords[0][i], coords[1][i]) )
    return pos_corrected_x, pos_corrected_y
