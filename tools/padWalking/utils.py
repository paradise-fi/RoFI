from enums import Ori, Side, Dock, Degree
from configuration import Module, Edge, Configuration


def getLeftOri(ori):
    """
    Get orientation left to ori

    :param ori: initial orientation
    :return: orientation left to ori
    """

    if (ori is Ori.N):
        return Ori.W
    if (ori is Ori.W):
        return Ori.S
    if (ori is Ori.S):
        return Ori.E
    if (ori is Ori.E):
        return Ori.N

def getRightOri(ori):
    """
    Get orientation right to ori

    :param ori: initial orientation
    :return: orientation right to ori
    """
    
    if (ori is Ori.N):
        return Ori.E
    if (ori is Ori.E):
        return Ori.S
    if (ori is Ori.S):
        return Ori.W
    if (ori is Ori.W):
        return Ori.N

def getOppositeOri(ori):
    """
    Get orientation opposite to ori

    :param ori: initial orientation
    :return: orientation opposite to ori
    """
    
    if (ori is Ori.N):
        return Ori.S
    if (ori is Ori.S):
        return Ori.N
    if (ori is Ori.E):
        return Ori.W
    if (ori is Ori.W):
        return Ori.E


def getOtherSide(side):
    """
    Get other side of universal module

    :param side: initial side
    :return: the other side
    """

    if (side is Side.A):
        return Side.B
    return Side.A

def edgeToPad(id, side, dock, ori, posId):
    """
    Get the edge connected to pad

    :param id: id of the module
    :param side: side connected to the pad
    :param dock: dock connected to the pad
    :ori: orientation of connection
    :posId: id of the position on the pad
    :return: edge connected to the pad
    """

    return Edge(id, side, dock, ori, Dock.MINUS_Z, Side.B, posId)

def getPositionId(x, y):
    """
    Get position id (name) (id of module on the pad to join)

    :param x: x coordinate of the position
    :param y: y coordinate of the position
    :return: id of the position
    """

    if (x == 0 and y == 0):
        return "0"
    return "1" + str(x).zfill(2) + str(y).zfill(2)

def getPositionFromId(id1):
    """
    Get position coordinates from its id

    :id1: id of the position
    :return: x, y coordinates of the position or -1, -1 if the position is not on the pad
    """
    idStr = str(id1)
    if (idStr == "0"):
        return 0, 0
    if (not(idStr[0] == "1")):
        print("not on the pad: ", idStr)
        return -1, -1
    x = int(idStr[1:3])
    y = int(idStr[3:5])
    return x, y

def __whichDockCanConnect(fixedSide, fixedDock, alpha, beta, gamma):
    if (fixedSide is Side.A):
        if (fixedDock is Dock.PLUS_X):
            if (gamma is Degree.ZERO):
                return Dock.MINUS_X
            elif (gamma is Degree.ONEHUNDREDEIGHTY or gamma is Degree.MINUS_ONEHUNDREDEIGHTY):
                return Dock.PLUS_X
            elif ((gamma is Degree.NINETY and beta is Degree.NINETY) or (gamma is Degree.MINUS_NINETY and beta is Degree.MINUS_NINETY)):
                return Dock.MINUS_Z
            else:
                return "err"
        elif (fixedDock is Dock.MINUS_X):
            if (gamma is Degree.ZERO):
                return Dock.PLUS_X
            elif (gamma is Degree.ONEHUNDREDEIGHTY or gamma is Degree.MINUS_ONEHUNDREDEIGHTY):
                return Dock.MINUS_X
            elif ((gamma is Degree.NINETY and beta is Degree.MINUS_NINETY) or (gamma is Degree.MINUS_NINETY and beta is Degree.NINETY)):
                return Dock.MINUS_Z
            else:
                return "err"
        elif (fixedDock is Dock.MINUS_Z):
            if ((alpha is Degree.NINETY and gamma is Degree.NINETY) or (alpha is Degree.MINUS_NINETY and gamma is Degree.MINUS_NINETY)):
                return Dock.PLUS_X
            elif ((alpha is Degree.NINETY and gamma is Degree.MINUS_NINETY) or (alpha is Degree.MINUS_NINETY and gamma is Degree.NINETY)):
                return Dock.MINUS_X
            elif ((alpha is Degree.NINETY and gamma is Degree.ZERO and beta is Degree.NINETY) or (alpha is Degree.NINETY and gamma is Degree.ONEHUNDREDEIGHTY and beta is Degree.MINUS_NINETY) or
                  (alpha is Degree.NINETY and gamma is Degree.MINUS_ONEHUNDREDEIGHTY and beta is Degree.MINUS_NINETY)
                  or (alpha is Degree.MINUS_NINETY and gamma is Degree.ZERO and beta is Degree.MINUS_NINETY) or (alpha is Degree.MINUS_NINETY and gamma is Degree.ONEHUNDREDEIGHTY and beta is Degree.NINETY)
                  or (alpha is Degree.MINUS_NINETY and gamma is Degree.MINUS_ONEHUNDREDEIGHTY and beta is Degree.NINETY)):
                return Dock.MINUS_Z
            else:
                return "err"
        else:
            return "err"
    elif (fixedSide is Side.B):
        # just switch alpha and beta, it is symmetric
        return __whichDockCanConnect(Side.A, fixedDock, beta, alpha, gamma)
        
def __whichOriCanConnectInCaseOfZ(fixedSide, fixedDock, fixedOri, alpha, beta, gamma):
    newOri = ""
    if (fixedSide is Side.A):
        if (alpha is Degree.NINETY and gamma is Degree.NINETY):
            # +X
            if (beta is Degree.ZERO):
                return getRightOri(fixedOri)
            if (beta is Degree.NINETY):
                return fixedOri
            if (beta is Degree.MINUS_NINETY):
                return getOppositeOri(fixedOri)
        if (alpha is Degree.NINETY and gamma is Degree.MINUS_NINETY):
            # -X
            if (beta is Degree.ZERO):
                return getLeftOri(fixedOri)
            if (beta is Degree.NINETY):
                return fixedOri
            if (beta is Degree.MINUS_NINETY):
                return getOppositeOri(fixedOri)
        if (alpha is Degree.NINETY and gamma is Degree.ZERO and beta is Degree.NINETY):
            return getOppositeOri(fixedOri)
        if (alpha is Degree.NINETY and (gamma is Degree.ONEHUNDREDEIGHTY or gamma is Degree.MINUS_ONEHUNDREDEIGHTY) and beta is Degree.MINUS_NINETY):
            return fixedOri

        if (alpha is Degree.MINUS_NINETY and gamma is Degree.NINETY):
            # -X
            if (beta is Degree.ZERO):
                return getRightOri(fixedOri)
            if (beta is Degree.NINETY):
                return getOppositeOri(fixedOri)
            if (beta is Degree.MINUS_NINETY):
                return fixedOri
        if (alpha is Degree.MINUS_NINETY and gamma is Degree.MINUS_NINETY):
            # +X
            if (beta is Degree.ZERO):
                return getLeftOri(fixedOri)
            if (beta is Degree.NINETY):
                return getOppositeOri(fixedOri)
            if (beta is Degree.MINUS_NINETY):
                return fixedOri
        if (alpha is Degree.MINUS_NINETY and gamma is Degree.ZERO and beta is Degree.MINUS_NINETY):
            return getOppositeOri(fixedOri)
        if (alpha is Degree.MINUS_NINETY and (gamma is Degree.ONEHUNDREDEIGHTY or gamma is Degree.MINUS_ONEHUNDREDEIGHTY) and beta is Degree.NINETY):
            return fixedOri
    elif (fixedSide is Side.B):
        # switch alpha and beta
        return __whichOriCanConnectInCaseOfZ(Side.A, fixedDock, fixedOri, beta, alpha, gamma)
        
def __whichOriCanConnect(fixedSide, fixedDock, fixedOri, newDock, alpha, beta, gamma):
    newOri = fixedOri
    if (fixedSide is Side.A):
        if (fixedDock is Dock.PLUS_X):
            if (alpha is Degree.NINETY):
                newOri = getRightOri(newOri)
            elif (alpha is Degree.MINUS_NINETY):
                newOri = getLeftOri(newOri)
        elif (fixedDock is Dock.MINUS_X):
            if (alpha is Degree.NINETY):
                newOri = getLeftOri(newOri)
            elif (alpha is Degree.MINUS_NINETY):
                newOri = getRightOri(newOri)

        if (newDock is Dock.PLUS_X):
            if (beta is Degree.NINETY):
                newOri = getLeftOri(newOri)
            elif (beta is Degree.MINUS_NINETY):
                newOri = getRightOri(newOri)
        elif (newDock is Dock.MINUS_X):
            if (beta is Degree.NINETY):
                newOri = getRightOri(newOri)
            elif (beta is Degree.MINUS_NINETY):
                newOri = getLeftOri(newOri)

        if (newDock is Dock.MINUS_Z or fixedDock is Dock.MINUS_Z):
            newOri = __whichOriCanConnectInCaseOfZ(fixedSide, fixedDock, fixedOri, alpha, beta, gamma)

        # at the begining we set newOri = fixedOri, but it is true only if we have one +X and one "-X"
        if ((fixedDock is Dock.PLUS_X and newDock is Dock.PLUS_X) or (fixedDock is Dock.MINUS_X and newDock is Dock.MINUS_X)):
            newOri = getOppositeOri(newOri)
        return newOri
    elif (fixedSide is Side.B):
        # switch alpha and beta
        return __whichOriCanConnect(Side.A, fixedDock, fixedOri, newDock, beta, alpha, gamma)

# return newSide, newDock, newOri which can connect for single module
def computeNewEdge(fixedSide, fixedDock, fixedOri, alpha, beta, gamma):
    """
    Compute new edge of the single rofibot

    The rofibot is rotated and wants to connect by the free part to the pad.
    This function computes the new connection parameters (the new edge)

    :param fixedSide: fixed side of the module
    :param fixedDock: fixed dock of the module
    :param fixedOri: fixed ori of the module
    :param alpha: angle alpha of the module
    :param beta: angle beta of the module
    :param gamma: angle gamma of the module
    :return: new side, new dock and new ori of the new edge
    """

    newSide = getOtherSide(fixedSide)
    newDock = __whichDockCanConnect(fixedSide, fixedDock, alpha, beta, gamma)
    newOri = __whichOriCanConnect(fixedSide, fixedDock, fixedOri, newDock, alpha, beta, gamma)
    return newSide, newDock, newOri

def getDifference(angleFrom, angleTo):
    """
    Get difference of two angles

    :param angleFrom: initial angle
    :param angleTo: goal angle
    :return: angles difference
    """
    return abs(angleFrom.value - angleTo.value)
