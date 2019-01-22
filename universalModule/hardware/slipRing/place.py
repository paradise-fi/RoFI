#!/usr/bin/env python

from math import cos, sin, radians, pi
import pcbnew
import sys
from copy import deepcopy

def fromMm(mm):
    return int(1000000 * mm)

def name(n):
    x = n.split("_")
    if len(x) != 2:
        return None
    return x[0]

def segment(n):
    x = n.split("_")
    if len(x) != 2:
        return None
    return x[1]

def newTrack(board, start, end, layer=pcbnew.F_Cu):
    t = pcbnew.TRACK(board)
    t.SetStart(start)
    t.SetEnd(end)
    t.SetLayer(layer)
    board.Add(t)
    return t

def newVia(board, pos):
    t = pcbnew.VIA(board)
    t.SetPosition(pos)
    t.SetDrillDefault()
    t.SetWidth(fromMm(1))
    board.Add(t)
    return t

def mapNetCode(code):
    if code < 6:
        return code + 1
    return 11 - code + 6 + 1

outerR = fromMm(35)
step = fromMm(2)
center = (fromMm(140), fromMm(90))

filename = "slipRing.kicad_pcb"

board = pcbnew.LoadBoard(filename)
module = board.GetModules()

POLYGON_STEPS = 100
for idx, r in enumerate([outerR - i * step for i in range(12)]):
    for i in range(POLYGON_STEPS):
        step = 2 * pi / float(POLYGON_STEPS)
        a = i * step
        start = pcbnew.wxPoint(center[0] + r * cos(a), center[1] + r * sin(a))
        end = pcbnew.wxPoint(center[0] + r * cos(a + step), center[1] + r * sin(a + step))
        t = newTrack(board, start, end, pcbnew.F_Cu)
        t.SetWidth(fromMm(1.5))
        t.SetNetCode(mapNetCode(idx))

board.Save(filename)