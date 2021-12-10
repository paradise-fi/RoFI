from common import Node, Pad, Origin, Bounds, getConfig, reconnect, lookAndTouch, getNodeByDirection, getDirection
from enums import Ori, Side, Dock, Degree

def __computeReconfig(rofibot, pad, origin, node, nextPossibleDirections, thinkOneStepFurther):
    """
    Compute reconfig either with nextPossible directions or without
    """

    if (thinkOneStepFurther):
        return rofibot.computeReconfig(pad, origin, node, nextPossibleDirections)
    else:
        return rofibot.computeReconfig(pad, origin, node)

def __goStraightToCorner(rofibot, pad, origin, node, down, thinkOneStepFurther):
    """
    Goes to bottom left corner either down or left

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param node: current node
    :param down: whether to go down (True) or left (False)
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations, current node
    """

    canConnect = True
    configs = []
    while(canConnect):
        if (down):
            node = Node(node.x, node.y - 1, node)
            nextPossibleDirections = [Ori.S, Ori.E]
        else:
            node = Node(node.x - 1, node.y, node)
            nextPossibleDirections = [Ori.W, Ori.N]
        currConfigs = __computeReconfig(rofibot, pad, origin, node, nextPossibleDirections, thinkOneStepFurther)
        configs.extend(currConfigs)
        canConnect = pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))
        if (canConnect):
            currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
    return configs, node.prev


def goToCorner(rofibot, pad, origin, node, bounds, thinkOneStepFurther = True):
    """
    Goes to bottom left corner

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param node: current node
    :param bounds: bounds of the pad (in relative coordinates)
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations, current node
    """

    down = True
    configs = []
    nextPossibleDirections = [Ori.S, Ori.W]
    while(True):        
        if (down):
            node = Node(node.x, node.y - 1, node)
        else:
            node = Node(node.x - 1, node.y, node)
        currConfigs = __computeReconfig(rofibot, pad, origin, node, nextPossibleDirections, thinkOneStepFurther)
        configs.extend(currConfigs)
        if (pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))):
            currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
            down = not(down)
        else:            
            currConfigs, node = __goStraightToCorner(rofibot, pad, origin, node.prev, not(down), thinkOneStepFurther)
            bounds.minX, bounds.minY = rofibot.fixedPosition()
            configs.extend(currConfigs)
            return configs, node

def __goStraightUpOrDown(rofibot, pad, origin, node, up, bounds, thinkOneStepFurther):
    """
    Goes straight up or down

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param node: current node
    :param up: whether to go up (True) or down (False)
    :param bounds: bounds of the pad (in relative coordinates)
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations, current node
    """

    configs = []
    canConnect = True
    while(canConnect):
        # already on the last position
        if (up and bounds.maxY is not None and bounds.maxY == node.y):
            return configs, node
        if (not(up) and bounds.minY is not None and bounds.minY == node.y):
            return configs, node

        if (up):
            node = Node(node.x, node.y + 1, node)
            nextPossibleDirections = [Ori.N]
            if (bounds.maxY is None or bounds.maxY == node.y):
                nextPossibleDirections.append(Ori.E)
        else:
            node = Node (node.x, node.y - 1, node)
            nextPossibleDirections = [Ori.S]
            if (bounds.minY is None or bounds.minY == node.y):
                nextPossibleDirections.append(Ori.E)
        currConfigs = __computeReconfig(rofibot, pad, origin, node, nextPossibleDirections, thinkOneStepFurther)
        configs.extend(currConfigs)
        canConnect = pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))
        if (canConnect):
            currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
        else:
            if (up):
                _, bounds.maxY = rofibot.fixedPosition()
            else:
                _, bounds.minY = rofibot.fixedPosition()
    return configs, node.prev


def walkDirect(rofibot, pad, origin, thinkOneStepFurther = True):
    """
    Walk the pad up and down

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations
    """
    
    node = Node(0, 0, None)
    bounds = Bounds()
    configs = [getConfig(rofibot, origin)]
    cornerConfigs, node = goToCorner(rofibot, pad, origin, node, bounds, thinkOneStepFurther)
    configs.extend(cornerConfigs)
    goUp = True
    canConnect = True
    while(canConnect):
        currConfigs, node = __goStraightUpOrDown(rofibot, pad, origin, node, goUp, bounds, thinkOneStepFurther)
        configs.extend(currConfigs)
        node = Node(node.x + 1, node.y)
        if (goUp):
            nextPossibleDirections = [Ori.S]
            if (bounds.minY is not None and rofibot.fixedPosition()[1] == bounds.minY):
                # it is still at the bottom
                nextPossibleDirections.append(Ori.E)
        else:
            nextPossibleDirections = [Ori.N]
            if (bounds.maxY is not None and rofibot.fixedPosition()[1] == bounds.maxY):
                # it is still at the top
                nextPossibleDirections.append(Ori.E)
        currConfigs = __computeReconfig(rofibot, pad, origin, node, nextPossibleDirections, thinkOneStepFurther)
        configs.extend(currConfigs)
        canConnect = pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))
        if (canConnect):
            currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
        else:
            bounds.maxX, _ = rofibot.fixedPosition()
        goUp = not(goUp)
    return configs


def __getNextNodeAndPossibleDirections(node, goUp, bounds, i):
    """
    Get next node and possible directions

    :param node: current node
    :param goUp: True if goes up, False otherwise
    :param bounds: bounds of the pad
    :param i: round of the walk
    :return: next node, list of next possible directions, True if it is enough only to touch the next node
    """
    onlyTouch = False
    if (goUp):
        if (i % 4 == 0):
            node = getNodeByDirection(node, Ori.E)
            nextPossibleDirections = [Ori.N]
            if (bounds.maxY is None or bounds.maxY == node.y):
                nextPossibleDirections.append(Ori.E)
        elif (i % 4 == 1):
            node = getNodeByDirection(node, Ori.N)
            nextPossibleDirections = [Ori.W]
        elif (i % 4 == 2):
            node = getNodeByDirection(node, Ori.W)
            nextPossibleDirections = [Ori.N]
            if (bounds.maxY is not None and bounds.maxY == node.y):
                onlyTouch = True
                nextPossibleDirections = []
        elif (i % 4 == 3):
            node = getNodeByDirection(node, Ori.N)
            nextPossibleDirections = [Ori.E]
    else:
        if (i % 4 == 0):
            node = getNodeByDirection(node, Ori.E)
            nextPossibleDirections = [Ori.S]
            if (bounds.minY is None or bounds.minY == node.y):
                nextPossibleDirections.append(Ori.E)
        elif (i % 4 == 1):
            node = getNodeByDirection(node, Ori.S)
            nextPossibleDirections = [Ori.W]
        elif (i % 4 == 2):
            node = getNodeByDirection(node, Ori.W)
            nextPossibleDirections = [Ori.S]
            if (bounds.minY is not None and bounds.minY == node.y):
                onlyTouch = True
                nextPossibleDirections = []
        elif (i % 4 == 3):
            node = getNodeByDirection(node, Ori.S)
            nextPossibleDirections = [Ori.E]
    
    return node, nextPossibleDirections, onlyTouch

def __goDoubleUpOrDown(rofibot, pad, origin, node, goUp, bounds, thinkOneStepFurther):
    """
    Goes two columns up or down

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param node: current node
    :param goUp: whether to go up (True) or down (False)
    :param bounds: bounds of the pad (in relative coordinates)
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations, current node, True if it can continue (False if all nodes were visited)
    """

    configs = []
    canConnect = True
    i = 0
    wasOnBorder = 0
    while(True):
        if (goUp and bounds.maxY is not None and bounds.maxY == node.y):
            wasOnBorder += 1
        if (not(goUp) and bounds.minY is not None and bounds.minY == node.y):
            wasOnBorder += 1
        if (wasOnBorder > 1):
            return configs, node, True
        
        node, nextPossibleDirections, onlyTouch = __getNextNodeAndPossibleDirections(node, goUp, bounds, i)
        currConfigs = __computeReconfig(rofibot, pad, origin, node, nextPossibleDirections, thinkOneStepFurther)
        configs.extend(currConfigs)
        canConnect = pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))
        if (canConnect):
            if (onlyTouch):
                currConfigs = lookAndTouch(rofibot, pad, origin)
                configs.extend(currConfigs)
                return configs, node.prev, True
            currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
        else:
            if (getDirection(node.prev, node) is Ori.E):
                bounds.maxX, _ = rofibot.fixedPosition()
                currConfigs, node = __goStraightUpOrDown(rofibot, pad, origin, node.prev, goUp, bounds, thinkOneStepFurther)
                configs.extend(currConfigs)
                return configs, node.prev, False
            else:
                # out is on the top or bottom
                if (goUp):
                    _, bounds.maxY = rofibot.fixedPosition()
                else:
                    _, bounds.minY = rofibot.fixedPosition()
                return configs, node.prev, True

        i+=1



def walkDirectZigZag(rofibot, pad, origin, thinkOneStepFurther = True):
    """
    Walks the pad up and down in two columns

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations
    """

    node = Node(0, 0, None)
    bounds = Bounds()
    configs = [getConfig(rofibot, origin)]
    cornerConfigs, node = goToCorner(rofibot, pad, origin, node, bounds, thinkOneStepFurther)
    configs.extend(cornerConfigs)
    goUp = True
    canConnect = True
    while(canConnect):
        currConfigs, node, canContinue = __goDoubleUpOrDown(rofibot, pad, origin, node, goUp, bounds, thinkOneStepFurther)
        configs.extend(currConfigs)
        if (not(canContinue)):
            break
        
        goUp = not(goUp)

        if (getDirection(node, node.prev) is Ori.E):
            node = getNodeByDirection(node, Ori.E)
            currConfigs = __computeReconfig(rofibot, pad, origin, node, [Ori.E], thinkOneStepFurther)
            configs.extend(currConfigs)
            if (pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))):
                currConfigs = reconnect(rofibot, pad, origin)
                configs.extend(currConfigs)
            else:
                bounds.maxX, _ = rofibot.fixedPosition()
                break
        node = getNodeByDirection(node, Ori.E)
        if (goUp):
            nextPossibleDirections = [Ori.E, Ori.N]
        else:
            nextPossibleDirections = [Ori.E, Ori.S]
        currConfigs = __computeReconfig(rofibot, pad, origin, node, nextPossibleDirections, thinkOneStepFurther)
        configs.extend(currConfigs)
        if (pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))):
            currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
        else:
            bounds.maxX, _ = rofibot.fixedPosition()
            break

    return configs



def __getNextPossibleDirections(node, bounds, goesDown, nextRightOrUp):
    """
    Compute next possible directions for zigzag

    :param node: current node
    :param bounds: pad bounds
    :param goesDown: True if the zigzag goes down
    :param nextRightOrUp: True if next node is right or up
    :return: list of next possible directions
    """
    if(goesDown):
        if (nextRightOrUp):
            if (bounds.minY is not None and bounds.minY >= node.y and bounds.maxX is not None and bounds.maxX <= node.x):
                # rofibot is in the bottom right corner
                return [Ori.N]
            if (bounds.minY is not None and bounds.minY >= node.y):
                # rofibot is at very bottom
                return [Ori.E, Ori.N]
            if (bounds.maxX is not None and bounds.maxX <= node.x):
                # rofibot is at very right
                return [Ori.S, Ori.N]
            return [Ori.S, Ori.E]
        else:
            if (bounds.maxX is not None and bounds.maxX <= node.x):
                # rofibot is in the very right (can be in corner)
                return [Ori.N]
            if (bounds.minY is not None and bounds.minY >= node.y):
                # rofibot is at very bottom
                return [Ori.E, Ori.N]
            return [Ori.E]
    else:
        if (nextRightOrUp):
            if (bounds.minX is not None and bounds.minX >= node.x and bounds.maxY is not None and bounds.maxY <= node.y):
                # rofibot is in the upper left corner
                return [Ori.E]
            if (bounds.minX is not None and bounds.minX >= node.x):
                # rofibot is at very left
                return [Ori.N, Ori.E]
            if (bounds.maxY is not None and bounds.maxY <= node.y):
                # rofibot is at very top
                return [Ori.W, Ori.E]
            return [Ori.W, Ori.N]
        else:
            if (bounds.maxY is not None and bounds.maxY <= node.y):
                # rofibot is at very top (can be in corner)
                return [Ori.E]
            if (bounds.minX is not None and bounds.minX >= node.x):
                #rofibot is at very left
                return [Ori.N, Ori.E]
            return [Ori.N]


def __goZigZagDown(rofibot, pad, origin, node, bounds, nextRight, thinkOneStepFurther):
    """
    Goes the pad zigzag down without initial look

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param node: current node
    :param bounds: pad bounds
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations, new node, True if rofibot was in upper right corner
    """

    configs = []
    canConnect = True
    while (canConnect):
        if (nextRight):
            if (bounds.maxX is not None and bounds.maxX <= node.x):
                # end of zigzag
                break
            # goes right
            node = Node(node.x + 1, node.y, node)
        else:
            # next down
            if (bounds.minY is not None and bounds.minY >= node.y):
                # end of zigzag
                break
            node = Node(node.x, node.y - 1, node)

        currConfigs = __computeReconfig(rofibot, pad, origin, node, __getNextPossibleDirections(node, bounds, True, nextRight), thinkOneStepFurther)
        configs.extend(currConfigs)
        canConnect = pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))
        if (canConnect):
            if (not(nextRight) and bounds.maxX is not None and bounds.maxX == node.x and not(rofibot.needsReconnectionInNextStep(Node(node.x, node.y + 2)))):
                currConfigs = lookAndTouch(rofibot, pad, origin)
                configs.extend(currConfigs)
                node = node.prev
            else:    
                currConfigs = reconnect(rofibot, pad, origin)
                configs.extend(currConfigs)
            nextRight = not(nextRight)
        else:
            if (nextRight):
                bounds.maxX = node.x - 1
            else:
                bounds.minY = node.y + 1
            node = node.prev

    wasInCorner = False
    if (nextRight and bounds.maxX is not None and bounds.maxY is not None):
        fixedX, fixedY = rofibot.fixedPosition()
        freeX, freeY = rofibot.freePosition()
        if ((fixedX == bounds.maxX and fixedY == bounds.maxY) or (freeX == bounds.maxX and freeY == bounds.maxY)):
            wasInCorner = True
            
    return configs, node, wasInCorner

def __goZigZagDownWhole(rofibot, pad, origin, node, bounds, thinkOneStepFurther):
    """
    Goes the pad zigzag down with initial look

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param node: current node
    :param bounds: pad bounds
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations, new node, True if rofibot was in upper right corner
    """

    configs = []
    if (bounds.maxY is None or bounds.maxY > node.y):
        # check the position up (belongs to this zigzag)
        node = Node(node.x, node.y + 1, node)
        currConfigs = __computeReconfig(rofibot, pad, origin, node, [], False)
        configs.extend(currConfigs)
        if (pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))):
            # if it is well rotate, only touch, do not reconnect
            if (rofibot.needsReconnectionInNextStep(Node(node.x + 1, node.y - 1))):
                currConfigs = reconnect(rofibot, pad, origin)
                configs.extend(currConfigs)
                node = Node(node.x, node.y - 1, node)
                currConfigs = __computeReconfig(rofibot, pad, origin, node, [Ori.E, Ori.N], thinkOneStepFurther)
                configs.extend(currConfigs)
                # returns back, can surely connect
                currConfigs = reconnect(rofibot, pad, origin)
                configs.extend(currConfigs)
            else:
                currConfigs = lookAndTouch(rofibot, pad, origin)
                configs.extend(currConfigs)
                node = Node(node.x, node.y - 1, node)
        else:
            node = node.prev
            _, bounds.maxY = rofibot.fixedPosition()
    downConfigs, node, wasInCorner = __goZigZagDown(rofibot, pad, origin, node, bounds, True, thinkOneStepFurther)
    configs.extend(downConfigs)
    return configs, node, wasInCorner
    

def __prepareForGoingUp(rofibot, pad, origin, node, bounds, thinkOneStepFurther):
    """
    Prepare for going up (rotate and reconnect rofibot)

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param node: current node
    :param bounds: pad bounds
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations, new node, True if rofibot was in upper right corner
    """

    configs = []
    if (bounds.maxX is None or bounds.maxX > node.x):
        # try to go right, is fixed at bottom
        node = Node(node.x + 1, node.y, node)
        currConfigs = __computeReconfig(rofibot, pad, origin, node, [Ori.N, Ori.E], thinkOneStepFurther)
        configs.extend(currConfigs)
        if (pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))):
            currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
            return configs, node, False
        bounds.maxX, _ = rofibot.fixedPosition()
        currConfigs, node, wasInCorner = __prepareForGoingUp(rofibot, pad, origin, node.prev, bounds, thinkOneStepFurther)
        configs.extend(currConfigs)
        return configs, node, wasInCorner
    
    # is fixed right
    if (node.prev.x == node.x):
        # came from top in last zigzag, needs to go two times up
        node = Node(node.x, node.y + 1, node)
        currConfigs = __computeReconfig(rofibot, pad, origin, node, [Ori.N, Ori.W], thinkOneStepFurther)
        configs.extend(currConfigs)
        if (pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))):
            currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
        else:
            # is in the upper right corner
            # should not happen
            _, bounds.maxY = rofibot.fixedPosition()
            return configs, node.prev, True
    
    # do one step up to next zigzag
    if (bounds.maxY is None or bounds.maxY > node.y):
        # try to do one step up
        node = Node(node.x, node.y + 1, node)
        currConfigs = __computeReconfig(rofibot, pad, origin, node, [Ori.N], thinkOneStepFurther)
        configs.extend(currConfigs)
        if (pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))):
            currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
        else:
            # is in the upper right corner
            _, bounds.maxY = rofibot.fixedPosition()
            return configs, node.prev, True
    
    else:
        return configs, node.prev, True 

    return configs, node, False

def __goZigZagUp(rofibot, pad, origin, node, bounds, nextUp, thinkOneStepFurther):
    """
    Goes the pad zigzag up without initial look

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param node: current node
    :param bounds: pad bounds
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations, new node, True if rofibot was in upper right corner
    """

    configs = []
    canConnect = True
    while (canConnect):
        if (nextUp):
            if (bounds.maxY is not None and bounds.maxY <= node.y):
                # end of zigzag
                break
            # goes up
            node = Node(node.x, node.y + 1, node)
        else:
            # next left
            if (bounds.minX is not None and bounds.minX >= node.x):
                # end of zigzag
                break
            node = Node(node.x - 1, node.y, node)

        currConfigs = __computeReconfig(rofibot, pad, origin, node, __getNextPossibleDirections(node, bounds, False, nextUp), thinkOneStepFurther)
        configs.extend(currConfigs)
        canConnect = pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))
        if (canConnect):
            if (not(nextUp) and bounds.maxY is not None and bounds.maxY == node.y and not(rofibot.needsReconnectionInNextStep(Node(node.x + 2, node.y)))):
                currConfigs = lookAndTouch(rofibot, pad, origin)
                configs.extend(currConfigs)
                node = node.prev
            else:    
                currConfigs = reconnect(rofibot, pad, origin)
                configs.extend(currConfigs)
            nextUp = not(nextUp)
        else:
            if (nextUp):
                bounds.maxY = node.y - 1
            else:
                bounds.minX = node.x + 1
            node = node.prev

    wasInCorner = False
    if (nextUp and bounds.maxX is not None and bounds.maxY is not None):
        fixedX, fixedY = rofibot.fixedPosition()
        freeX, freeY = rofibot.freePosition()
        if ((fixedX == bounds.maxX and fixedY == bounds.maxY) or (freeX == bounds.maxX and freeY == bounds.maxY)):
            wasInCorner = True
    
    return configs, node, wasInCorner

def __goZigZagUpWhole(rofibot, pad, origin, node, bounds, thinkOneStepFurther):
    """
    Goes the pad zigzag uo with initial look

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param node: current node
    :param bounds: pad bounds
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations, new node, True if rofibot was in upper right corner
    """

    configs = []
    if (bounds.maxX is None or bounds.maxX > node.x):
        # check the position right (belongs to this zigzag)
        node = Node(node.x + 1, node.y, node)
        currConfigs = __computeReconfig(rofibot, pad, origin, node, [], thinkOneStepFurther)
        configs.extend(currConfigs)
        if (pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))):
            if (rofibot.needsReconnectionInNextStep(Node(node.x - 1, node.y + 1))):
                currConfigs = reconnect(rofibot, pad, origin)
                configs.extend(currConfigs)
                node = Node(node.x - 1, node.y, node)
                currConfigs = __computeReconfig(rofibot, pad, origin, node, [Ori.N], thinkOneStepFurther)
                configs.extend(currConfigs)
                # returns back, can surely connect
                currConfigs = reconnect(rofibot, pad, origin)
                configs.extend(currConfigs)
            else:
                currConfigs = lookAndTouch(rofibot, pad, origin)
                configs.extend(currConfigs)
                node = Node(node.x - 1, node.y, node)
        else:
            node = node.prev
            bounds.maxX, _ = rofibot.fixedPosition()
    upConfigs, node, wasInCorner = __goZigZagUp(rofibot, pad, origin, node, bounds, True, thinkOneStepFurther)
    configs.extend(upConfigs)
    return configs, node, wasInCorner

def __prepareForGoingDown(rofibot, pad, origin, node, bounds, thinkOneStepFurther):
    """
    Prepare for going down (rotate and reconnect rofibot)

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param node: current node
    :param bounds: pad bounds
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations, new node, True if rofibot was in upper right corner
    """

    configs = []
    if (bounds.maxY is None or bounds.maxY > node.y):
        # try to go up, is fixed at left
        node = Node(node.x, node.y + 1, node)
        currConfigs = __computeReconfig(rofibot, pad, origin, node, [Ori.N, Ori.E], thinkOneStepFurther)
        configs.extend(currConfigs)
        if (pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))):
            currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
            return configs, node, False
        _, bounds.maxY = rofibot.fixedPosition()
        currConfigs, node, wasInCorner = __prepareForGoingDown(rofibot, pad, origin, node.prev, bounds, thinkOneStepFurther)
        configs.extend(currConfigs)
        return configs, node, wasInCorner
    
    # is fixed up
    if (node.prev.y == node.y):
        # came from right in last zigzag, needs to go two times right
        node = Node(node.x + 1, node.y, node)
        # TODO proc tady byly dve ori?
        currConfigs = __computeReconfig(rofibot, pad, origin, node, [Ori.E], thinkOneStepFurther)
        configs.extend(currConfigs)
        if (pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))):
            currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
        else:
            # is in the upper right corner
            # should not happen
            bounds.maxX, _ = rofibot.fixedPosition()
            return configs, node.prev, True
    
    # do one step right to next zigzag
    if (bounds.maxX is None or bounds.maxX > node.x):
        # try to do one step right
        node = Node(node.x + 1, node.y, node)
        currConfigs = __computeReconfig(rofibot, pad, origin, node, [Ori.E], thinkOneStepFurther)
        configs.extend(currConfigs)
        if (pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))):
            currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
        else:
            # is in the upper right corner
            bounds.maxX, _ = rofibot.fixedPosition()
            return configs, node.prev, True
    
    else:
        return configs, node.prev, True

    return configs, node, False



def walkZigZag(rofibot, pad, origin, thinkOneStepFurther = True):
    """
    Walks the pad zigzag in diagonals

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations
    """

    node = Node(0, 0, None)
    bounds = Bounds()
    configs = [getConfig(rofibot, origin)]
    cornerConfigs, node = goToCorner(rofibot, pad, origin, node, bounds, thinkOneStepFurther)
    configs.extend(cornerConfigs)
    goDown = True
    # Upper right corner means end of the surface
    wasInCorner = False
    while (not(wasInCorner)):
        if (goDown):
            currConfigs, node, wasInCorner = __goZigZagDownWhole(rofibot, pad, origin, node, bounds, thinkOneStepFurther)
            configs.extend(currConfigs)
            if (wasInCorner):
                break
            currConfigs, node, wasInCorner = __prepareForGoingUp(rofibot, pad, origin, node, bounds, thinkOneStepFurther)
            configs.extend(currConfigs)
        else:
            currConfigs, node, wasInCorner = __goZigZagUpWhole(rofibot, pad, origin, node, bounds, thinkOneStepFurther)
            configs.extend(currConfigs)
            if (wasInCorner):
                break
            currConfigs, node, wasInCorner = __prepareForGoingDown(rofibot, pad, origin, node, bounds, thinkOneStepFurther)
            configs.extend(currConfigs)
        goDown = not(goDown)
    
    return configs
