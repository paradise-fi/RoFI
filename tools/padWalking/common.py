import utils
from enums import Ori, Side, Dock, Degree
from actions import Action
from configuration import Configuration, Module, Edge

class Node:
    """
    Class representing a node in a 2D grid

    Node has x and y coordinates and can have its parent prev (used for some algorithms)
    """
    def __init__(self, x, y, prev = None):
        self.x = x
        self.y = y
        self.prev = prev

    def __repr__(self):
        return "Node(%s, %s)" % (self.x, self.y)
    
    def __eq__(self, other):
        if isinstance(other, Node):
            return ((self.x == other.x) and (self.y == other.y))
        return False

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash(self.__repr__()) 

def areNeighbourNodes(u, v):
    """
    Defines if two nodes are neighbours

    :param u: first node
    :param v: second node
    :return: True if u and v are neighbours, False otherwise
    """

    if (u.x == v.x and (u.y == v.y - 1 or u.y == v.y + 1)):
        return True
    if (u.y == v.y and (u.x == v.x - 1 or u.x == v.x + 1)):
        return True
    return False

class Pad:
    """
    Class representing the pad

    Pad has its dimensions x and y. 
    Pad has its map - 2D list of booleans, map[x][y] contains True if there is possible to connect on coordinates x, y, False otherwise
    Pad has its config - configuration respresenting the pad built from rofi modules
    """

    def __maxLenLine(self,lines):
        maxLen = 0
        for line in lines:
            l = line.rstrip()
            if (len(l) > maxLen):
                maxLen = len(l) 
        return maxLen
        
    def __numberOfLines(self, lines):
        count = len(lines)
        for line in reversed(lines):
            l = line.rstrip()
            if (len(l) == 0):
                count -= 1
            else:
                return count
        return count
    
    def __readMapFile(self, filename):
        """
        Read map from file and save it to the list

        :param filename: path to file with the map
        :return: dimensions of the pad a (x), b (y) and the map list
        """

        f = open(filename, "r")
        lines = f.readlines()
        b = self.__numberOfLines(lines)
        a = self.__maxLenLine(lines)
        mapArr = [[False for i in range(b)] for j in range(a)]
        for i in range(b):
            y = b - i - 1
            for x in range(a):
                if (i >= len(lines) or x >= len(lines[i])):
                    # some lines in the file can be shorter
                    continue
                if (lines[i][x] == 'o'):
                    mapArr[x][y] = True 

        f.close()
        return a, b, mapArr

    def __buildPadWithHoles(self):
        """
        Build pad with possible holes

        By the map create modules and connect them to build pad from rofi modules.

        :return: configuration of the pad from rofi modules 
        """

        modules = []
        edges = [] 
        for i in range(self.x):
            for j in range(self.y):
                if (not(self.map[i][j])):
                    continue

                # create module
                alpha = Degree.ZERO
                beta = Degree.ZERO
                gamma = Degree.NINETY
                name = utils.getPositionId(i, j)
                m = Module(name, alpha, beta, gamma)
                modules.append(m)

                # create edges
                if (not (i == self.x - 1)):
                    if (self.map[i+1][j]):
                        idMy = utils.getPositionId(i, j)
                        idNeighbor = utils.getPositionId(i+1, j)
                        e = Edge(idMy, Side.A, Dock.PLUS_X, Ori.S, Dock.MINUS_X, Side.A, idNeighbor)
                        edges.append(e)
                if (not (j == self.y - 1)):
                    if (self.map[i][j+1]):
                        idMy = utils.getPositionId(i, j)
                        idNeighbor = utils.getPositionId(i, j+1)
                        e = Edge(idMy, Side.B, Dock.MINUS_X, Ori.S, Dock.PLUS_X, Side.B, idNeighbor)
                        edges.append(e)
        return Configuration(modules, edges)

    def __init__(self, filename):
        self.x, self.y, self.map = self.__readMapFile(filename)
        self.config = self.__buildPadWithHoles()     

    def canConnect(self, x, y):
        """
        Decide if it is possible to connect on coordinates x, y

        :param x: the x coordinate
        :param y: the y coordinate
        :return: True if it is possible to connect on coodinates x, y, False otherwise
        """

        if (x < 0 or x >= self.x or y < 0 or y >= self.y):
            # out of borders
            return False
        return self.map[x][y]

    # is rectangle without holes
    def isRectangle(self):
        """
        Decide if the pad is reactangle (without holes)

        Note: some algorithms can walk only rectangle pad
        
        :return: True if the pad is rectangle, False otherwise
        """

        for i in range(self.x):
            for j in range(self.y):
                if (not(self.map[i][j])):
                    return False
        return True   
    
    def isConnected(self):
        """
        Decide if the pad is connected

        Connected pad means, that for any two nodes, there is a path between them (using only steps to neighbours)

        :return: True if the pad is connected, False otherwise
        """

        return isMapConnected(self.x, self.y, self.map)


def __dfs(a, b, x, y, visited, padMap):
    """
    DFS walking the map

    :param a: x dimension of the map
    :param b: y dimension of the map
    :param x: x coordinate of the initial node
    :param y: y coordinate of the initial node
    :param visited: 2D list to store visited nodes, visited[x][y] will be True if node with coordinates x, y will be visited by dfs
    :param padMap: 2D list of boolean values, padMap[x][y] is True if it is possible to connect on coordinates x, y, False otherwise
    """

    if (padMap[a][b]):
        visited[a][b] = True
        for i in range(4):
            if (i == 0 and b < y - 1 and not visited[a][b + 1]):
                __dfs(a, b + 1, x, y, visited, padMap)
            elif (i == 1 and a < x - 1 and not visited[a + 1][b]):
                __dfs(a + 1, b, x, y, visited, padMap)
            elif (i == 2 and b > 0 and not visited[a][b - 1]):
                __dfs(a, b - 1, x, y, visited, padMap)
            elif (i == 3 and a > 0 and not visited[a - 1][b]):
                __dfs(a - 1, b, x, y, visited, padMap)
                

# checks is the pad is connected
def isMapConnected(x, y, padMap):
    """
    Decide if the map is connected

    Connected map means, that for any two nodes, there is a path between them (using only steps to neighbours)

    :param x: x dimension of the map
    :param y: y dimension of the map
    :param padMap: 2D list of boolean values, padMap[x][y] is True if it is possible to connect on coordinates x, y, False otherwise
    :return: True if the map is connected, False otherwise
    """

    for a in range(x):
        for b in range(y):
            if (padMap[a][b]):
                # it is init node for dfs
                visited = [[False for _ in range(y)] for _ in range(x)]
                __dfs(a, b, x, y, visited, padMap)
                visitedCount = sum(l.count(True) for l in visited)                    
                totalCount = sum(l.count(True) for l in padMap)
                if (visitedCount == totalCount):
                    return True
                return False
    # empty graph, we do not want to work with it, but it is connected
    return True    

class Origin:
    """
    Class representing conversion from absolute pad coordinates and orientations to rofibot relative coordinates and orientations

    Origin has its x and y coordinates defining the absolute coordinates of initial position of the rofibot
    Origin has its direction defining the initial orientation of the rofibot
    """

    def __init__(self, x, y, direction):
        # absolute coordinates of origin (initial position of A)
        self.X, self.Y = x, y
        self.DIRECTION = direction

    def relativeToAbsolutePosition(self, x, y):
        """
        Convert relative position of the rofibot to absolute position of the pad

        :param x: relative x coordinate of the rofibot
        :param y: relative y coordinate of the rofibot
        :return: absolute x, y coordinates of the pad
        """

        if (self.DIRECTION is Ori.N):
            return self.X + x, self.Y + y
        if (self.DIRECTION is Ori.S):
            return self.X - x, self.Y - y
        if (self.DIRECTION is Ori.E):
            return self.X + y, self.Y - x
        if (self.DIRECTION is Ori.W):
            return self.X - y, self.Y + x

    def relativeToAbsoluteOri(self, ori):
        """
        Convert relative orientation of the rofibot to absolute orientation of the pad

        :param ori: relative orientation of the rofibot
        :return: absolute orientation of the pad
        """
        
        if (self.DIRECTION is Ori.N):
            return utils.getOppositeOri(ori)
        elif (self.DIRECTION is Ori.S):
            return ori
        elif (self.DIRECTION is Ori.E):
            return utils.getLeftOri(ori)
        elif (self.DIRECTION is Ori.W):
            return utils.getRightOri(ori)

    def absoluteToRelativeOri(self, ori):
        """
        Convert absolute orientation of the pad to absolute orientation of the rofibot

        :param ori: absolute orientation of the pad
        :return: relative orientation of the rofibot
        """

        if (self.DIRECTION is Ori.N):
            return utils.getOppositeOri(ori)
        elif (self.DIRECTION is Ori.S):
            return ori
        elif (self.DIRECTION is Ori.E):
            return utils.getRightOri(ori)
        elif (self.DIRECTION is Ori.W):
            return utils.getLeftOri(ori)


class Bounds:
    """
    Class representing bounds of rectangle pad

    It has minX, maxX, minY and maxY as minimal and maximal relative x, y coordinates of the pad
    """

    def __init__(self):
        self.minX, self.maxX, self.minY, self.maxY = None, None, None, None
    
def getRofibotsEdge(rofibot, origin):
    """
    Get edge of the rofibot fixed to the pad

    :param rofibot: rofibot
    :param origin: origin for position and orientation conversion
    :return: fixed edge
    """

    id, side, dock, ori, position = rofibot.getPadEdge()
    return utils.edgeToPad(id, side, dock, origin.relativeToAbsoluteOri(ori), utils.getPositionId(*origin.relativeToAbsolutePosition(*position)))

def getConfig(rofibot, origin):
    """
    Get current configuration of the rofibot 

    Without pad configuration, but with edge fixed to the pad

    :param rofibot: rofibot
    :param origin: origin for position and orientation conversion
    :return: current configuration of the rofibot
    """

    config = rofibot.toConfiguration()
    config.edges.append(getRofibotsEdge(rofibot, origin))
    return config

def getNewDirection(rofibot, newNode):
    """
    Get direction of the newNode with respect to the fixed position of the rofibot 

    :param rofibot: rofibot
    :param newNode: node 
    :return: direction form the fixed node to the new node
    """

    mX, mY = rofibot.fixedPosition()
    if (mX == newNode.x and mY == newNode.y - 1):
        return Ori.N
    if (mX == newNode.x and mY == newNode.y + 1):
        return Ori.S
    if (mX == newNode.x - 1 and mY == newNode.y):
        return Ori.E
    if (mX == newNode.x + 1 and mY == newNode.y):
        return Ori.W

def getDirection(nodeFrom, nodeTo):
    """
    Get direction from one node to the second one

    :param nodeFrom: node from
    :param nodeTo: node to
    :return: direction from nodeFrom to nodeTo
    """

    if (nodeFrom.x == nodeTo.x and nodeFrom.y == nodeTo.y - 1):
        return Ori.N
    if (nodeFrom.x == nodeTo.x and nodeFrom.y == nodeTo.y + 1):
        return Ori.S
    if (nodeFrom.x == nodeTo.x - 1 and nodeFrom.y == nodeTo.y):
        return Ori.E
    if (nodeFrom.x == nodeTo.x + 1 and nodeFrom.y == nodeTo.y):
        return Ori.W

def getNodeByDirection(u, direction):
    """
    Get neighbour node by direction

    :param u: node for which searching the neighbour
    :param direction: direction in which to find the neighbour
    :return: neighbour of u in direction direction
    """

    if (direction is Ori.N):
        return Node(u.x, u.y + 1, u)
    elif (direction is Ori.E):
        return Node(u.x + 1, u.y, u)
    elif (direction is Ori.S):
        return Node(u.x, u.y - 1, u)
    elif (direction is Ori.W):
        return Node(u.x - 1, u.y, u)

def reconnect(rofibot, pad, origin):
    """
    Add new edge and remove the old one, reconnect the rofibot

    Rofibot is rotated to the new position, connect it to the new position and disconnect form the old one

    :param rofibot: rofibot already rotated
    :param pad: pad to connect
    :origin: origin to convert relative and absolute coordinates and orientations

    :return: list of configurations used to reconnect the rofibot to the new position
    """

    configs = []
   
    oldEdge = getRofibotsEdge(rofibot, origin)
    newId, newSide, newDock, newOri = rofibot.computeNewEdge()
    rofibot.fixedId = newId 
    rofibot.fixedSide = newSide 
    rofibot.fixedDock = newDock
    rofibot.fixedOri = newOri
    rofibot.incrementNonConcurentAndAllActions(Action.ADD_EDGE)
    rofibot.incrementNonConcurentAndAllActions(Action.REMOVE_EDGE)
    newPosId = utils.getPositionId(*origin.relativeToAbsolutePosition(*rofibot.fixedPosition()))
    newEdge = utils.edgeToPad(newId, newSide, newDock, origin.relativeToAbsoluteOri(newOri), newPosId)

    configuration = rofibot.toConfiguration()

    configs.append(Configuration(configuration.modules, configuration.edges + [oldEdge, newEdge]))
    configs.append(Configuration(configuration.modules, configuration.edges + [newEdge]))

    return configs
   
# does not change the rofibot, only touches the new edge
def lookAndTouch(rofibot, pad, origin):
    """
    Only touch the new position

    Rofibot is rotated to the new position, connect it to the new position and disconnect
    After these steps rofibot stays in its initial position

    :param rofibot: rofibot already rotated
    :param pad: pad to connect
    :origin: origin to convert relative and absolute coordinates and orientations

    :return: list of configurations used to touch the rofibot the new position
    """
    
    configs = []

    oldEdge = getRofibotsEdge(rofibot, origin)
    newId, newSide, newDock, newOri = rofibot.computeNewEdge()

    rofibot.incrementNonConcurentAndAllActions(Action.ADD_EDGE)
    rofibot.incrementNonConcurentAndAllActions(Action.REMOVE_EDGE)
    
    newPosId = utils.getPositionId(*origin.relativeToAbsolutePosition(*rofibot.freePosition()))
    newEdge = utils.edgeToPad(newId, newSide, newDock, origin.relativeToAbsoluteOri(newOri), newPosId)

    configuration = rofibot.toConfiguration()

    configs.append(Configuration(configuration.modules, configuration.edges + [oldEdge, newEdge]))
    configs.append(Configuration(configuration.modules, configuration.edges + [oldEdge]))

    return configs
