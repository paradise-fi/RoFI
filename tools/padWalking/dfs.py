from common import Node, Pad, Origin, Bounds, getConfig, reconnect, lookAndTouch, getDirection, getNodeByDirection, areNeighbourNodes
from enums import Ori, Side, Dock, Degree
from strategies import DFSStrategy
from random import shuffle


def __canVisitNodeInDirections(u, visitedNodes, seenNodes, useBounds, bounds):
    """
    Decide if can visit neighbours in all 4 directions

    :param u: current node
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param seenNodes: nodes that were only looked 
    :param useBounds: whether to use bounds of the pad (possible only for rectangle pads)
    :param bounds: bounds of the pad
    :return: can go N, E, S, W (four booleans)
    """
    canN, canE, canS, canW = True, True, True, True
    if (useBounds):
        if (bounds.maxY is not None and bounds.maxY == u.y):
            canN = False
        if (bounds.maxX is not None and bounds.maxX == u.x):
            canE = False
        if (bounds.minY is not None and bounds.minY == u.y):
            canS = False
        if (bounds.minX is not None and bounds.minX == u.x):
            canW = False
    nodeN = Node(u.x, u.y + 1)
    nodeE = Node(u.x + 1, u.y)
    nodeS = Node(u.x, u.y - 1)
    nodeW = Node(u.x - 1, u.y)

    if (canN and nodeN in visitedNodes):
        canN = False
    if (canE and nodeE in visitedNodes):
        canE = False
    if (canS and nodeS in visitedNodes):
        canS = False
    if (canW and nodeW in visitedNodes):
        canW = False
    return canN, canE, canS, canW
 
def __isNodeClosed(node, visitedNodes, seenNodes):
    """
    Check if node is closed by neighbours

    :param node: node to be checked
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param seenNodes: nodes that were only looked 
    :return: True if node is closed, False otherwise   
    """

    nodeN = getNodeByDirection(node, Ori.N)
    nodeE = getNodeByDirection(node, Ori.E)
    nodeS = getNodeByDirection(node, Ori.S)
    nodeW = getNodeByDirection(node, Ori.W)
    return (nodeN in visitedNodes or nodeN in seenNodes) and (nodeE in visitedNodes or nodeE in seenNodes) and (nodeS in visitedNodes or nodeS in seenNodes) and (nodeW in visitedNodes or nodeW in seenNodes)

def __isOnEdgeClosed(node, visitedNodes, seenNodes, bounds, newBound):
    """
    Check if node is closed by neighbours and bounds

    :param node: node to be checked
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param seenNodes: nodes that were only looked 
    :param bounds: bounds of the pad
    :param newBound: which bound must be checked
    :return: True if node is closed, False otherwise   
    """
    
    nodeN = getNodeByDirection(node, Ori.N)
    nodeE = getNodeByDirection(node, Ori.E)
    nodeS = getNodeByDirection(node, Ori.S)
    nodeW = getNodeByDirection(node, Ori.W)
    if (newBound == "maxY" or newBound == "all"):
        if ((node.y == bounds.maxY) and (nodeE in visitedNodes or nodeE in seenNodes) and (nodeS in visitedNodes or nodeS in seenNodes) and (nodeW in visitedNodes or nodeW in seenNodes)):
        # The node is at the top and around are visited or seen
            return True 
    if (newBound == "maxX" or newBound == "all"):
        if ((node.x == bounds.maxX) and (nodeN in visitedNodes or nodeN in seenNodes) and (nodeS in visitedNodes or nodeS in seenNodes) and (nodeW in visitedNodes or nodeW in seenNodes)):
            return True 
    if (newBound == "minY" or newBound == "all"):
        if ((node.y == bounds.minY) and (nodeN in visitedNodes or nodeN in seenNodes) and (nodeE in visitedNodes or nodeE in seenNodes) and (nodeW in visitedNodes or nodeW in seenNodes)):
            return True 
    if (newBound == "minX" or newBound == "all"):
        if ((node.x == bounds.minX) and (nodeN in visitedNodes or nodeN in seenNodes) and (nodeE in visitedNodes or nodeE in seenNodes) and (nodeS in visitedNodes or nodeS in seenNodes)):
            return True 
    return False
    
def __checkSeenNeighboursClosedByUOrBounds(u, visitedNodes, seenNodes, useBounds, bounds, newBound):
    """
    Check if some node should be closed (moved to visited, so that not visited again)

    :param u: current node
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param seenNodes: nodes that were only looked    
    :param useBounds: whether to use bounds of the pad (possible only for rectangle pads)
    :param newBound: which bound must be checked
    :param bounds: bounds of the pad
    """
    closed = set()

    if (u is not None):
        for direction in Ori:
            # check all seen neighbours of u
            neighbour = getNodeByDirection(u, direction)
            if (neighbour in seenNodes):
                if (__isNodeClosed(neighbour, visitedNodes, seenNodes)):
                    # closed by other nodes
                    closed.add(neighbour)
                if (useBounds and __isOnEdgeClosed(neighbour, visitedNodes, seenNodes, bounds, "all")):
                    # closed by a bound and other nodes
                    closed.add(neighbour)
    
    if (newBound is not None and useBounds):
        for node in seenNodes:
            # check all nodes on the new bound
            if (__isOnEdgeClosed(node, visitedNodes, seenNodes, bounds, newBound)):
                closed.add(node)
    
    seenNodes.difference_update(closed)
    visitedNodes.update(closed)

def __checkSeenNode(u, visitedNodes, seenNodes, useBounds, bounds):
    """
    Check if seen node u should be marked as visited

    :param u: current node
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param seenNodes: nodes that were only looked    
    :param useBounds: whether to use bounds of the pad (possible only for rectangle pads)
    :param bounds: bounds of the pad
    """

    if (u not in seenNodes):
        return

    closed = False
    if (__isNodeClosed(u, visitedNodes, seenNodes)):
        closed = True
    if (useBounds and __isOnEdgeClosed(u, visitedNodes, seenNodes, bounds, "all")):
        closed = True
    if (closed):
        seenNodes.remove(u)
        visitedNodes.add(u)

def __setBounds(bounds, nodeFrom, nodeOut):
    """
    Set bounds of the pad

    :param bounds: bounds instance
    :param nodeFrom: node fixed to pad (the last one)
    :param nodeOut: node out of the pad
    """

    direction = getDirection(nodeFrom, nodeOut)
    if (direction is Ori.N):
        bounds.maxY = nodeFrom.y 
        return "maxY"
    elif (direction is Ori.E):
        bounds.maxX = nodeFrom.x 
        return "maxX"
    elif (direction is Ori.S):
        bounds.minY = nodeFrom.y
        return "minY"
    elif(direction is Ori.W):
        bounds.minX = nodeFrom.x
        return "minX"

def __goToNode(rofibot, pad, origin, u, first, nextPossibleDirections, visitedNodes, seenNodes, strategy, backtrack, thinkOneStepFurther, useBounds, bounds, look):
    """
    Go to the node u

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param u: node where to go
    :param first: True if this is first node, False otherwise
    :param nextPossibleDirections: next possible directions from node u
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param seenNodes: nodes that were only looked
    :param strategy: strategy for choosing next node
    :param backtrack: True if it is on way back, False otherwise
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :param useBounds: whether to use bounds of the pad (possible only for rectangle pads)
    :param bounds: bounds of the pad
    :param look: whether to look around on each node
    :return: list of configurations, True if can continue from u, first
    """

    configs = []
    canContinueFromThisNode = True
    if (first):
        # initial configuration, does not need to move
        first = False
        initConfig = getConfig(rofibot, origin)
        configs.append(initConfig)
    else:
        fixedX, fixedY = rofibot.fixedPosition()
        alreadyFixedToU = (fixedX == u.x and fixedY == u.y)
        # u is the new position, task is to get the rofibot to the new position
        if (thinkOneStepFurther):
            currConfigs = rofibot.computeReconfig(pad, origin, u, nextPossibleDirections)
        else:
            currConfigs = rofibot.computeReconfig(pad, origin, u)
        configs.extend(currConfigs)
        if (not (pad.canConnect(*origin.relativeToAbsolutePosition(u.x, u.y)))):
            # node u is not on the pad
            canContinueFromThisNode = False
            newBound = __setBounds(bounds, u.prev, u)
            if (look):
                __checkSeenNeighboursClosedByUOrBounds(u, visitedNodes, seenNodes, useBounds, bounds, newBound)
        else:
            # reconnect to new node
            currConfigs = []
            if (len(nextPossibleDirections) <= 1 and not(backtrack)):
                # the node is leaf (only direction is back)
                # if it needs reconnection in next step, reconnect now
                if (not alreadyFixedToU):
                    nextPossibleDirectionsPrev = __getNextPossibleDirections(u.prev, visitedNodes, seenNodes, strategy, useBounds, bounds, look)
                    if (len(nextPossibleDirectionsPrev) <= 2 and len(nextPossibleDirectionsPrev) > 0 and rofibot.needsReconnectionInNextStep(getNodeByDirection(u.prev, nextPossibleDirectionsPrev[0]))):
                        currConfigs = reconnect(rofibot, pad, origin)
                    else:
                        currConfigs = lookAndTouch(rofibot, pad, origin)
                canContinueFromThisNode = False
            else:
                if (not alreadyFixedToU):
                    currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
            
        # else the node is already connected on u
    return configs, canContinueFromThisNode, first

def __lookAround(rofibot, pad, origin, u, orderedDirections, visitedNodes, seenNodes, useBounds, bounds, thinkOneStepFurther):
    """
    Look around node u

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param u: node where to go
    :param orderedDirections: ordered directions to look arount
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param seenNodes: nodes that were only looked
    :param useBounds: whether to use bounds of the pad (possible only for rectangle pads)
    :param bounds: bounds of the pad
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations
    """

    configs = []
    # process all but last directions (last is to go there, not only look)
    for i in range(len(orderedDirections) - 1):
        # rotate to the new node
        newNode = getNodeByDirection(u, orderedDirections[i])
        currConfigs = rofibot.computeReconfig(pad, origin, newNode)
        configs.extend(currConfigs)
        if (pad.canConnect(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))):
            if (thinkOneStepFurther and rofibot.needsReconnectionInNextStep(getNodeByDirection(u, orderedDirections[i+1]))):
                # reconnect to new node
                currConfigs = reconnect(rofibot, pad, origin)
                configs.extend(currConfigs)
                # rotate back to u, but with proper next possible directions
                currConfigs = rofibot.computeReconfig(pad, origin, u, orderedDirections[i:])
                configs.extend(currConfigs)
                # reconnect back to u
                currConfigs = reconnect(rofibot, pad, origin)
                configs.extend(currConfigs)
            else:
                # touch the new node
                currConfigs = lookAndTouch(rofibot, pad, origin)
                configs.extend(currConfigs)
            seenNodes.add(newNode)
            __checkSeenNode(newNode, visitedNodes, seenNodes, useBounds, bounds)
            __checkSeenNeighboursClosedByUOrBounds(newNode, visitedNodes, seenNodes, useBounds, bounds, None)
        else:
            visitedNodes.add(newNode)
            newBound = __setBounds(bounds, u, newNode)
            __checkSeenNeighboursClosedByUOrBounds(newNode, visitedNodes, seenNodes, useBounds, bounds, newBound)
    return configs
    
def __getPossibleLooks(u, nextPossibleDirections, visitedNodes, seenNodes):
    """
    Returns list of possible lookes

    :param u: current node
    :nextPossibleDirections: next possible directions from node u
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param seenNodes: nodes that were only looked
    :return: list of directions for looks
    """
    looks = []
    for d in nextPossibleDirections:
        node = getNodeByDirection(u, d)
        if (node not in visitedNodes and node not in seenNodes):
            looks.append(d)
    return looks


# Prejdi na uzel u, tam se rozhlidni a vyber dalsi uzel
# nerozhlizet se na uzel zpatky, ani tam neprechazet
def __handleNode(rofibot, pad, origin, u, first, nextPossibleDirections, visitedNodes, seenNodes, strategy, backtrack, thinkOneStepFurther, useBounds, bounds, look):
    """
    Go to the node u, look around there if possible and choose next node

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param u: node where to go
    :param first: True if this is first node, False otherwise
    :param nextPossibleDirections: next possible directions from node u
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param seenNodes: nodes that were only looked
    :param strategy: strategy for choosing next node
    :param backtrack: True if it is on way back, False otherwise
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :param useBounds: whether to use bounds of the pad (possible only for rectangle pads)
    :param bounds: bounds of the pad
    :param look: whether to look around on each node
    :return: list of configurations, first, True if can continue from u, next node or None (if cannot continue)
    """


    configs, canContinueFromThisNode, first = __goToNode(rofibot, pad, origin, u, first, nextPossibleDirections, visitedNodes, seenNodes, strategy, backtrack, thinkOneStepFurther, useBounds, bounds, look)

    if (not canContinueFromThisNode):
        return configs, first, canContinueFromThisNode, None

    possibleLooks = __getPossibleLooks(u, nextPossibleDirections, visitedNodes, seenNodes)
    v, canDoStep = __getNextNodeForDFS(u, rofibot, visitedNodes, seenNodes, strategy, useBounds, bounds, look)

    if (not canDoStep):
        # nowhere to go
        return configs, first, False, None

    if (len(possibleLooks) == 0 or not look):
        # do not look around, just choose next node
        return configs, first, canDoStep, v

    if (len(possibleLooks) == 1):
        # only one way -> go there
        return configs, first, True, getNodeByDirection(u, possibleLooks[0])
    
    # more ways -> look around
    orderedDirections = __orderLooks(rofibot, u, possibleLooks, strategy)
    currConfigs = __lookAround(rofibot, pad, origin, u, orderedDirections, visitedNodes, seenNodes, useBounds, bounds, thinkOneStepFurther)
    configs.extend(currConfigs)
   
    nextNode = getNodeByDirection(u, orderedDirections[-1])
    return configs, first, True, nextNode


def __orderLooks(rofibot, u, nextPossibleDirectionsWithoutBack, strategy):
    """
    Order directions to look

    :param rofibot: rofibot
    :param u: node where to go
    :param nextPossibleDirectionsWithoubBack: next possible directions from node u
    :param strategy: strategy for choosing next node
    :return: ordered list of next possible directions
    """

    ords = []
    # reversed strict order - first look and the last possible go
    if (Ori.W in nextPossibleDirectionsWithoutBack):
        ords.append(Ori.W)
    if (Ori.S in nextPossibleDirectionsWithoutBack):
        ords.append(Ori.S)
    if (Ori.E in nextPossibleDirectionsWithoutBack):
        ords.append(Ori.E)
    if (Ori.N in nextPossibleDirectionsWithoutBack):
        ords.append(Ori.N)

    if (len(ords) <= 1):
        return ords
    
    if (strategy in (DFSStrategy.RANDOM, DFSStrategy.RANDOM_ROFIBOT_BEST, DFSStrategy.RANDOM_EARLY, DFSStrategy.RANDOM_ROFIBOT_BEST_EARLY)):
        shuffle(ords)
    
    if (rofibot is not None and strategy.value[1]):
        worstDir = rofibot.getWorstStepDirection()
        if (worstDir in ords):       
            # move the worst to the last position, so that the looks are easy
            ords.append(ords.pop(ords.index(worstDir)))
        freeX, freeY = rofibot.freePosition()
        alreadyRotatedDirection = getDirection(u, Node(freeX, freeY))
        if (alreadyRotatedDirection in ords):
            # put already rotated to the first position
            ords.remove(alreadyRotatedDirection)
            ords.insert(0, alreadyRotatedDirection)
    return ords


def __getNextPossibleDirections(u, visitedNodes, seenNodes, strategy, useBounds, bounds, look):
    """
    Get next possible directions to prerotate

    :param u: current node
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param seenNodes: nodes that were only looked
    :param strategy: strategy for choosing next node
    :param useBounds: whether to use bounds of the pad (possible only for rectangle pads)
    :param bounds: bounds of the pad
    :param look: whether to look around on each node
    :return: list of next possible directions from node u
    """

    # strategy can be used to prioritize some nodes
    # it is in strict order, random cannot prioritize
    canN, canE, canS, canW = __canVisitNodeInDirections(u, visitedNodes, seenNodes, useBounds, bounds)
    ords = []
    if (canN):
        ords.append(Ori.N)
    if (canE):
        ords.append(Ori.E)
    if (canS):
        ords.append(Ori.S)
    if (canW):
        ords.append(Ori.W)
    
    if (look):
        ords.reverse()

    # add possible step back
    if (u.prev is not None):
        ords.append(getDirection(u, u.prev))    
    
    return ords

def __areThereUnexploredPathsInStack(stack, visitedNodes, seenNodes, strategy, useBounds, bounds, look):
    """
    Check if there are unexplored paths in the stack

    :param stack: stack of nodes for backtrack
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param seenNodes: nodes that were only looked
    :param strategy: strategy for choosing next node
    :param useBounds: whether to use bounds of the pad (possible only for rectangle pads)
    :param bounds: bounds of the pad
    :param look: whether to look around on each node
    :return: True if there are unexplored paths, False otherwise
    """

    for node in stack:
        if (len(__getNextPossibleDirections(node, visitedNodes, seenNodes, strategy, useBounds, bounds, look)) > 1):
            return True
    if (len(__getNextPossibleDirections(stack[0], visitedNodes, seenNodes, strategy, useBounds, bounds, look)) > 0):
        return True
    return False

def __getNextNodeForDFS(u, rofibot, visitedNodes, seenNodes, strategy, useBounds, bounds, look):
    """
    Get next node to be visited (by the strategy)

    :param u: current node
    :param rofibot: rofibot
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param seenNodes: nodes that were only looked
    :param strategy: strategy for choosing next node
    :param useBounds: whether to use bounds of the pad (possible only for rectangle pads)
    :param bounds: bounds of the pad
    :param look: whether to look around on each node
    :return: node to be visited, True if there is some node to be visited, None and False otherwise
    """

    canN, canE, canS, canW = __canVisitNodeInDirections(u, visitedNodes, seenNodes, useBounds, bounds)
    ords = []
    if (canN):
        ords.append(Ori.N)
    if (canE):
        ords.append(Ori.E)
    if (canS):
        ords.append(Ori.S)
    if (canW):
        ords.append(Ori.W)

    if (len(ords) == 0):
        # no way to go
        return None, False

    if (strategy in (DFSStrategy.RANDOM, DFSStrategy.RANDOM_ROFIBOT_BEST, DFSStrategy.RANDOM_EARLY, DFSStrategy.RANDOM_ROFIBOT_BEST_EARLY)):
        shuffle(ords)

    if (rofibot is not None and strategy.value[1]):
        worstDir = rofibot.getWorstStepDirection()
        if (worstDir in ords):       
            # move the worst to the last position
            ords.append(ords.pop(ords.index(worstDir)))

    firstNodeDirection = ords[0]
    if (firstNodeDirection is Ori.N):
        v = Node(u.x, u.y + 1, u)
    elif (firstNodeDirection is Ori.E):
        v = Node(u.x + 1, u.y, u)
    elif (firstNodeDirection is Ori.S):
        v = Node(u.x, u.y - 1, u)
    elif (firstNodeDirection is Ori.W):
        v = Node(u.x - 1, u.y, u)
    return v, True    
    

def dfsIterative(rofibot, pad, origin, strategy, thinkOneStepFurther, useBounds = False, look = False):
    """
    Iterative depth-first-search algorithm for walking the pad

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param strategy: strategy for choosing next node
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :param useBounds: whether to use bounds of the pad (possible only for rectangle pads)
    :param look: whether to look around on each node
    :return: list of configurations
    """

    configs = []
    first = True
    # stack of nodes
    stack = []
    # current node, relative coordinates
    u = Node(0, 0, None)
    stack.append(u)
    # only touched when look around, needs to be visited once more to search its subtree
    seenNodes = set()
    # visited nodes, do not visit them again (only in backtrack)
    # contains also nodes that have been seen and all its neighbours have been seen (it has no new path)
    visitedNodes = set()
    backtrack = False
    bounds = Bounds()
    while (len(stack) != 0):
        u = stack.pop()
        visitedNodes.add(u)
        if (u in seenNodes):
            seenNodes.remove(u)
        if (look):
            __checkSeenNeighboursClosedByUOrBounds(u, visitedNodes, seenNodes, useBounds, bounds, None)
        nextPossibleDirections = __getNextPossibleDirections(u, visitedNodes, seenNodes, strategy, useBounds, bounds, look)
        # got to the node u, look around if possible and choose next node v
        currConfigs, first, canContinueFromThisNode, v = __handleNode(rofibot, pad, origin, u, first, nextPossibleDirections, visitedNodes, seenNodes, strategy, backtrack, thinkOneStepFurther, useBounds, bounds, look)       
        configs.extend(currConfigs)

        if (strategy.value[2]):
            # strategy supports early end detection
            if (not __areThereUnexploredPathsInStack(stack + [u], visitedNodes, seenNodes, strategy, useBounds, bounds, look)):
                break        

        if (not(canContinueFromThisNode)):
            backtrack = True
        else:
            stack.append(u)
            stack.append(v)
            backtrack = False

    return configs

def dfs(rofibot, pad, origin, strategy, thinkOneStepFurther):
    """
    Walk the pad using dfs (depth-first-search) algorithm

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param strategy: strategy for choosing next node
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations
    """
    return dfsIterative(rofibot, pad, origin, strategy, thinkOneStepFurther, False, False)

def dfsWithBounds(rofibot, pad, origin, strategy, thinkOneStepFurther):
    """
    Walk the rectangle pad using dfs algorithm, using knowledge of bounds of the pad

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param strategy: strategy for choosing next node
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations
    """
    return dfsIterative(rofibot, pad, origin, strategy, thinkOneStepFurther, True, False)

def dfsLook(rofibot, pad, origin, strategy, thinkOneStepFurther):
    """
    Walk the pad using dfs algorithm with looking around on each node

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param strategy: strategy for choosing next node
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations
    """
    return dfsIterative(rofibot, pad, origin, strategy, thinkOneStepFurther, False, True)

def dfsLookWithBounds(rofibot, pad, origin, strategy, thinkOneStepFurther):
    """
    Walk the rectangle pad using dfs algorithm with looking around on each node, using knowledge of bounds of the pad

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param strategy: strategy for choosing next node
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations
    """
    return dfsIterative(rofibot, pad, origin, strategy, thinkOneStepFurther, True, True)