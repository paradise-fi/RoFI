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
    if len(sys.argv) != 7:
        print("Invalid usage!")
        print("Usage: splitBoard.py <input> <x> <y> <w> <h> <output>")
        sys.exit(1)

    board = pcbnew.LoadBoard(sys.argv[1])
    x = fromMm(int(sys.argv[2]))
    y = fromMm(int(sys.argv[3]))
    w = fromMm(int(sys.argv[4]))
    h = fromMm(int(sys.argv[5]))
    isolate(board, pcbnew.wxRect(x, y, w, h))
    board.Save(sys.argv[6])
