#!/usr/bin/env python3

# Based on https://github.com/KiCad/kicad-source-mirror/blob/master/demos/python_scripts_examples/gen_gerber_and_drill_files_board.py

import sys
import os

from pcbnew import *

def help():
    print("Usage: kicadExportDxf.py <board file> [<output dir>]")
    print("    Default output dir same as the board directory'")

if len(sys.argv) == 1 or len(sys.argv) > 3:
    print("Invalid number of arguments.")
    help()
    sys.exit()

filename = sys.argv[1]
basename = os.path.dirname(filename)
if len(sys.argv) == 3:
    plotDir = sys.argv[2]
else:
    plotDir = basename
plotDir = os.path.abspath(plotDir)

board = LoadBoard(filename)

pctl = PLOT_CONTROLLER(board)
popt = pctl.GetPlotOptions()

popt.SetOutputDirectory(plotDir)
popt.SetAutoScale(False)
popt.SetScale(1)
popt.SetMirror(False)
popt.SetExcludeEdgeLayer(True)
popt.SetScale(1)
popt.SetDXFPlotUnits(DXF_PLOTTER.DXF_UNIT_MILLIMETERS)
popt.SetDXFPlotPolygonMode(False)

plot_plan = [
    # name, id, comment
    ("PasteBottom", B_Paste, "Paste Bottom"),
    ("PasteTop", F_Paste, "Paste top"),
    ("EdgeCuts", Edge_Cuts, "Edges"),
]

for name, id, comment in plot_plan:
    pctl.SetLayer(id)
    pctl.OpenPlotfile(name, PLOT_FORMAT_DXF, comment)
    print('plot {}'.format(pctl.GetPlotFileName()))
    if pctl.PlotLayer() == False:
        print("plot error")
pctl.ClosePlot()

sys.exit(0)
