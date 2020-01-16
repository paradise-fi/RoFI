#!/usr/bin/env python3

import pcbnew
import sys

def fromMm(mm):
    return int(1000000 * mm)

def removeFrom(board, items, predicate):
    toDel = []
    for item in items:
        if predicate(item):
            toDel.append(item)
    for item in toDel:
        board.RemoveNative(item)

def outsideRect(rect, pos):
    return pos.x < rect.GetX() or pos.x > rect.GetX() + rect.GetWidth() or \
        pos.y < rect.GetY() or pos.y > rect.GetY() + rect.GetHeight()

def isolate(board, rect):
    def pred(item):
        return outsideRect(rect, item.GetPosition())
    removeFrom(board, board.GetModules(), pred)
    removeFrom(board, board.GetTracks(), pred)
    removeFrom(board, board.Zones(), pred)
    removeFrom(board, board.GetDrawings(), pred)

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Invalid usage!")
        print("Usage: kicadSquarePads.py <input> <output>")
        sys.exit(1)

    board = pcbnew.LoadBoard(sys.argv[1])
    for pad in board.GetPads():
        radius = pad.GetRoundRectCornerRadius()
        if radius < fromMm(0.3):
            pad.SetRoundRectCornerRadius(0)
            pad.SetShape(pcbnew.PAD_SHAPE_RECT)
    board.Save(sys.argv[2])