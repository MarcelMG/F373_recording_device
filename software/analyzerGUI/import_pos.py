import sys
import csv

# import Simon's position log files (*.csv format) "Rekonstuiert_Koordinaten_X.csv"
def import_pos(filepath):
    f = open(filepath, "r")
    csvreader = csv.reader(f, delimiter=";")
    data = [data for data in csvreader] # read in csv file as list
    data = data[1:len(data)] # discard first row which contains "X;Y;"
    pos_x = [int(x[0]) for x in data] # extract first column, x-coordinates
    pos_y = [int(y[1]) for y in data] # extract second column, y-coordinates
    f.close()
    return pos_x, pos_y
