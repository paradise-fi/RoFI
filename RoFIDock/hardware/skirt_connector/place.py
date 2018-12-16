#!/usr/bin/env python

from math import cos, sin, radians, pi
import pcbnew
import sys
from copy import deepcopy

def fromMm(mm):
    return 1000000 * mm

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

outerR = fromMm(11)
innerR = fromMm(7.5)
center = (fromMm(150), fromMm(100))

offset = {
    "A": 90,
    "B": 180,
    "2": 225,
    "1": 45
}

innerOffset = {
    "48V": 45.0 / 4 * 3,
    "GND": 45.0 / 4 * 3,
    "RX": 45.0 / 4 * 1,
    "TX": 45.0 / 4 * 1,
    "SENSE": 45.0 / 4  * 1
}

radius = {
    "48V": outerR,
    "RX": outerR,
    "TX": outerR,
    "GND": innerR,
    "SENSE": innerR
}


filename = "skirt_connector.kicad_pcb"

board = pcbnew.LoadBoard(filename)
module = board.GetModules()

pos = module.GetPosition()

vplus_max = 0
vplus_min = 0

while True:
    if not module:
        break
    n = name(module.GetValue())
    if not n:
        module = module.Next()
        continue
    s = segment(module.GetValue())

    c = module.GetPosition()
    r = radius[n]
    o = offset[s]
    io = innerOffset[n]
    if s == "1" or s == "2":
        io = 45 - io
    a = -(o + io) / 180.0 * pi

    c.Set(int(center[0] + r * cos(a)), int(center[1] + r * sin(a)))
    module.SetPosition(c)

    if n == "48V" and s == "1":
        vplus_min = a
    elif n == "48V" and s == "2":
        vplus_max = a

    if n == "48V" or n == "GND":
        # add vias
        newVia(board, c)
        VIA_COUNT = 6
        VIA_R = fromMm(0.75)
        for i in range(VIA_COUNT):
            a = i * 2 * pi / VIA_COUNT
            p = pcbnew.wxPoint(c.x + VIA_R * cos(a), c.y + VIA_R * sin(a))
            newVia(board, p)

    print(n + "-" + s + ": " + str(o + io) + ", " + str(r))
    module = module.Next()

POLYGON_STEPS = 20
for r in [radius["48V"], radius["GND"]]:
    for i in range(POLYGON_STEPS):
        step = (vplus_max - vplus_min) / float(POLYGON_STEPS)
        a = vplus_min + i * step
        start = pcbnew.wxPoint(center[0] + r * cos(a), center[1] + r * sin(a))
        end = pcbnew.wxPoint(center[0] + r * cos(a + step), center[1] + r * sin(a + step))
        t = newTrack(board, start, end, pcbnew.B_Cu)
        t.SetWidth(fromMm(2))


board.Save(filename)