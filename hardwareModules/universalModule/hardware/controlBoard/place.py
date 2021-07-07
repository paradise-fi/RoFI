#!/usr/bin/env python3

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

outerR = fromMm(37)
step = fromMm(1.5)
lines = 15

if __name__ == "__main__":
    filename = "controlBoard.kicad_pcb"

    if len(sys.argv) != 2 or sys.argv[1] not in ["front", "back", "via"]:
        print("Invalid usage!")
        print("call ./place.py [front|back|via]")
        sys.exit(1)

    board = pcbnew.LoadBoard(filename)

    for m in board.GetModules():
        if m.GetReference() == "U3":
            center = m.GetPosition()

    command = sys.argv[1]
    if command in ["front", "back"]:
        layer = pcbnew.F_Cu if command == "front" else pcbnew.B_Cu
        POLYGON_STEPS = 100
        for idx, r in enumerate([outerR - i * step for i in range(lines)]):
            for i in range(POLYGON_STEPS):
                step = 2 * pi / float(POLYGON_STEPS)
                a = i * step
                start = pcbnew.wxPoint(center[0] + r * cos(a), center[1] + r * sin(a))
                end = pcbnew.wxPoint(center[0] + r * cos(a + step), center[1] + r * sin(a + step))
                t = newTrack(board, start, end, layer)
                t.SetWidth(fromMm(1.25))
                t.SetNetCode(mapNetCode(idx))

    if command == "via":
        VIA_SPACING = fromMm(3)
        positions = [i for i in range(lines) if i not in [1, 3, 4, 5, 6, 10, 11]]
        for idx, r in enumerate([outerR - i * step for i in positions]):
            via_count = int(2 * pi * r / VIA_SPACING)
            for i in range(via_count):
                step = 2 * pi / float(via_count)
                a = i * step
                ro = r + fromMm((1.25 - 0.6) / 2)
                ri = r - fromMm((1.25 - 0.6) / 2)
                outer = pcbnew.wxPoint(center[0] + ro * cos(a), center[1] + ro * sin(a))
                inner = pcbnew.wxPoint(center[0] + ri * cos(a), center[1] + ri * sin(a))
                outerVia = newVia(board, outer)
                innerVia = newVia(board, inner)

                for via in [outerVia, innerVia]:
                    via.SetNetCode(mapNetCode(idx))
                    via.SetWidth(fromMm(0.6))
                    via.SetDrill(fromMm(0.3))

    board.Save(filename)