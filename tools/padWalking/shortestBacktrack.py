from common import Node, Pad, Origin, Bounds, getConfig, reconnect, lookAndTouch, getDirection, getNodeByDirection, areNeighbourNodes
from enums import Ori, Side, Dock, Degree
from strategies import BacktrackStrategy
from random import shuffle


def __canVisitNodeInDirections(u, visitedNodes, emptyNodes):
    """
    Decide if can visit neighbours in all 4 directions

    :param u: current node
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param seenNodes: nodes that were only looked 
    :return: can go N, E, S, W (four booleans)
    """

    canN, canE, canS, canW = True, True, True, True
    nodeN = Node(u.x, u.y + 1)
    nodeE = Node(u.x + 1, u.y)
    nodeS = Node(u.x, u.y - 1)
    nodeW = Node(u.x - 1, u.y)
    if (nodeN in visitedNodes or nodeN in emptyNodes):
        canN = False
    if (nodeE in visitedNodes or nodeE in emptyNodes):
        canE = False
    if (nodeS in visitedNodes or nodeS in emptyNodes):
        canS = False
    if (nodeW in visitedNodes or nodeW in emptyNodes):
        canW = False
    return canN, canE, canS, canW


def __goToNode(rofibot, pad, origin, u, first, nextPossibleDirections, visitedNodes, emptyNodes, strategy, thinkOneStepFurther):
    """
    Go to the node u

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param u: node where to go
    :param first: True if this is first node, False otherwise
    :param nextPossibleDirections: next possible directions from node u
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param emptyNodes: nodes that is impossible to connect (hole)
    :param strategy: strategy for choosing next node
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
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
            visitedNodes.remove(u)
            emptyNodes.add(u)
            canContinueFromThisNode = False
        else:
            # reconnect to new node
            currConfigs = []
            if (len(nextPossibleDirections) <= 0):
                # the node is leaf (only direction is back)
                # if it needs reconnection in next step, reconnect now
                if (not alreadyFixedToU):
                    closestOpenNodeFromU, closestPathFromU = __getClosestOpenNode(u, visitedNodes, emptyNodes)
                    closestOpenNodeFromCurr, closestPathFromCurr = __getClosestOpenNode(Node(*rofibot.fixedPosition()), visitedNodes, emptyNodes)
                    if (len(closestPathFromU) <= len(closestPathFromCurr)):
                        currConfigs = reconnect(rofibot, pad, origin)
                    else:
                        currConfigs = lookAndTouch(rofibot, pad, origin)
                canContinueFromThisNode = False
            else:
                if (not alreadyFixedToU):
                    currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
            
    return configs, canContinueFromThisNode, first

def __lookAround(rofibot, pad, origin, u, orderedDirections, visitedNodes, emptyNodes, thinkOneStepFurther):
    """
    Look around node u

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param u: node where to go
    :param orderedDirections: ordered directions to look arount
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param emptyNodes: nodes that is impossible to connect (hole)
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
            visitedNodes.add(newNode)
        else:
            emptyNodes.add(newNode)
    return configs
    

def __handleNode(rofibot, pad, origin, u, first, nextPossibleDirections, visitedNodes, emptyNodes, strategy, thinkOneStepFurther, look):
    """
    Go to the node u, look around there if possible and choose next node

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param u: node where to go
    :param first: True if this is first node, False otherwise
    :param nextPossibleDirections: next possible directions from node u
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param emptyNodes: nodes that is impossible to connect (hole)
    :param strategy: strategy for choosing next node
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :param look: whether to look around on each node
    :return: list of configurations, first, True if can continue from u, next node or None (if cannot continue)
    """

    configs, canContinueFromThisNode, first = __goToNode(rofibot, pad, origin, u, first, nextPossibleDirections, visitedNodes, emptyNodes, strategy, thinkOneStepFurther)
    
    if (not canContinueFromThisNode):
        return configs, first, canContinueFromThisNode, None

    if (len(nextPossibleDirections) == 0):
        return configs, first, False, None

    if (len(nextPossibleDirections) == 1):
        # only one way -> go there
        return configs, first, True, getNodeByDirection(u, nextPossibleDirections[0])

    if (strategy is BacktrackStrategy.RANDOM_ROFIBOT_BEST):
        worstDir = rofibot.getWorstStepDirection()
        if (worstDir in nextPossibleDirections):
            nextPossibleDirections.append(nextPossibleDirections.pop(nextPossibleDirections.index(worstDir)))

    if (look):
        # more ways -> look around
        currConfigs = __lookAround(rofibot, pad, origin, u, nextPossibleDirections, visitedNodes, emptyNodes, thinkOneStepFurther)
        configs.extend(currConfigs)
   
        nextNode = getNodeByDirection(u, nextPossibleDirections[-1])
    else:
        nextNode = getNodeByDirection(u, nextPossibleDirections[0])
    return configs, first, True, nextNode
 

def __getNextPossibleDirections(u, rofibot, visitedNodes, emptyNodes, strategy, look):
    """
    Get next possible directions to prerotate, in order by strategy

    :param u: current node
    :param rofibot: rofibot
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param emptyNodes: nodes that is impossible to connect (hole)
    :param strategy: strategy for choosing next node
    :param look: whether to look around on each node
    :return: ordered list of next possible directions from node u
    """

    canN, canE, canS, canW = __canVisitNodeInDirections(u, visitedNodes, emptyNodes)
    ords = []
    if (canN):
        ords.append(Ori.N)
    if (canE):
        ords.append(Ori.E)
    if (canS):
        ords.append(Ori.S)
    if (canW):
        ords.append(Ori.W)
    
    if (strategy is BacktrackStrategy.SCAN):
        goRight = u.y % 2 == 0
        if (look):
            goRight = u.y % 4 == 0
        if (Ori.S in ords):
            ords.remove(Ori.S)
            ords.insert(0, Ori.S)
        if (goRight and Ori.E in ords):
            ords.remove(Ori.E)
            ords.insert(0, Ori.E)
        elif (goRight and Ori.E not in ords and Ori.W in ords):
            goRight = False
        elif (not goRight and Ori.W in ords):
            ords.remove(Ori.W)
            ords.insert(0, Ori.W)
        elif (not goRight and Ori.W not in ords and Ori.E in ords):
            goRight = True

    elif (strategy in  (BacktrackStrategy.RANDOM_ROFIBOT_BEST, BacktrackStrategy.RANDOM)):
        shuffle(ords)
      
    if (look):
        ords.reverse()

    return ords


def __getNeighboursPossibleToConnect(u, visitedNodes):
    """
    Get neighbours of u which are possible to connect

    :param u: current node
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :return: list of nodes possible to connect
    """

    nodes = []
    for direction in [Ori.N, Ori.E, Ori.S, Ori.W]:
        node = getNodeByDirection(u, direction)
        if (node in visitedNodes):
            nodes.append(node)
    return nodes

def __hasUnexploredNeighbour(u, visitedNodes, emptyNodes):
    """
    Check if the node u has unexplored neighbour

    :param u: current node
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param emptyNodes: nodes that is impossible to connect (hole)
    :return: True if the node has unexplored neighbour, False otherwise
    """

    for direction in [Ori.N, Ori.E, Ori.S, Ori.W]:
        node = getNodeByDirection(u, direction)
        if (node not in visitedNodes and node not in emptyNodes):
            return True
    return False
    
def __getClosestOpenNode(u, visitedNodes, emptyNodes):
    """
    Get the closest node which has unexplored neighbour (by bfs algorithm)

    :param u: current node
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param emptyNodes: nodes that is impossible to connect (hole)
    :return: node which has unexplored neighbour, path to that node, None and [] if all nodes were explored
    """

    queue = [[u]]
    visited = []
    if (__hasUnexploredNeighbour(u, visitedNodes, emptyNodes)):
        return u, [u]
    while(queue):
        path = queue.pop(0)
        node = path[-1]
        if (node not in visited):
            neighbours = __getNeighboursPossibleToConnect(node, visitedNodes)
            for neighbour in neighbours:
                if (neighbour not in visited):
                    newPath = list(path)
                    newPath.append(neighbour)
                    queue.append(newPath)
                    if (__hasUnexploredNeighbour(neighbour, visitedNodes, emptyNodes)):
                        return neighbour, newPath
            visited.append(node)
    return None, []
        

def __backtrackToClosestNode(rofibot, pad, origin, u, visitedNodes, emptyNodes, thinkOneStepFurther):
    """
    Backtracks to closest node which has an unexplored neighbour

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param u: current node
    :param visitedNodes: nodes that were visited or do not have any unexplored neighbout
    :param emptyNodes: nodes that is impossible to connect (hole)
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations, new node
    """
    node, path = __getClosestOpenNode(u, visitedNodes, emptyNodes) 
    if (node is None):
        return [], None
    configs = []
    for i in range(len(path) - 1):
        stepNode = path[i]
        nextNode = path[i+1]
        nextDirection = getDirection(stepNode, nextNode)
        if (not (stepNode.x, stepNode.y) == rofibot.fixedPosition()):
            currConfigs = rofibot.computeReconfig(pad, origin, stepNode, [nextDirection])
            configs.extend(currConfigs)
            currConfigs = reconnect(rofibot, pad, origin)
            configs.extend(currConfigs)
    return configs, node


def shortestBacktrackImpl(rofibot, pad, origin, strategy, thinkOneStepFurther, look):
    """
    Shortest backtrack walking the pad

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param strategy: strategy for choosing next node
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :param look: whether to look around on each node
    :return: list of configurations
    """

    configs = []
    first = True
    # current node, relative coordinates
    u = Node(0, 0, None)
    # already visited nodes (can connect there)
    visitedNodes = set()
    # nodes where it is not possible to connect
    emptyNodes = set()
    while (True):
        visitedNodes.add(u)
        nextPossibleDirections = __getNextPossibleDirections(u, rofibot, visitedNodes, emptyNodes, strategy, look)
        # got to the node u, look around if possible and choose next node v
        currConfigs, first, canContinueFromThisNode, v = __handleNode(rofibot, pad, origin, u, first, nextPossibleDirections, visitedNodes, emptyNodes, strategy, thinkOneStepFurther, look)       
        configs.extend(currConfigs)
    
        if (not(canContinueFromThisNode)):
            currConfigs, node = __backtrackToClosestNode(rofibot, pad, origin, Node(*rofibot.fixedPosition()), visitedNodes, emptyNodes, thinkOneStepFurther)
            configs.extend(currConfigs)
            if (node is None):
                break
            u = node
        else:
            u = v

    return configs


def shortestBacktrack(rofibot, pad, origin, strategy, thinkOneStepFurther):
    """
    Walk the pad using shortest backtrack algorithm

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param strategy: strategy for choosing next node
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations
    """
    return shortestBacktrackImpl(rofibot, pad, origin, strategy, thinkOneStepFurther, False)

def shortestBacktrackLook(rofibot, pad, origin, strategy, thinkOneStepFurther):
    """
    Walk the pad using shortest backtrack algorithm with looking around on each node

    :param rofibot: rofibot
    :param pad: pad
    :param origin: origin
    :param strategy: strategy for choosing next node
    :param thinkOneStepFurther: whether the rofibot thinks one step further and prerotates
    :return: list of configurations
    """
    return shortestBacktrackImpl(rofibot, pad, origin, strategy, thinkOneStepFurther, True)