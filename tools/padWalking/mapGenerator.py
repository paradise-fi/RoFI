import sys, getopt
import math
from random import randint, shuffle
import random
from common import isMapConnected

def generateRectangleMap(x, y, filename):
    """
    Generate rectangle map

    :param x: x dimension of the map
    :param y: y dimension of the map
    :param filename: path to file where to write the map
    """

    f = open(filename, "w+")
    for b in range(y):
        f.write("o" * x + "\n")    
    f.close()

def generateRectangleWithOneRandomHole(x, y, filename):
    """
    Generate map with single random hole

    :param x: x dimension of the map
    :param y: y dimension of the map
    :param filename: path to file where to write the map
    """

    rx = randint(0, x - 1)
    ry = randint(0, y - 1)
    f = open(filename, "w+")
    for b in range(y):
        if (not (b == y - ry - 1)):
            f.write("o" * x + "\n")
        else:
            for a in range(x):
                if (a == rx):
                    f.write(".")
                else:
                    f.write("o")
            f.write("\n")    
    f.close()

def __printMapToFile(x, y, padMap, filename):
    """
    Write the map to the file

    :param x: x dimension of the map
    :param y: y dimension of the map
    :param padMap: 2D list of booleans, padMap[x][y] is True if it is possible to connect on coordinates x, y, False otherwise
    :param filename: path to file where to write the map    
    """
    f = open(filename, "w+")
    for b in range(y-1, -1, -1):
        for a in range(x):
            if (padMap[a][b]):
                f.write("o")
            else:
                f.write(".")
        f.write("\n")


def generateMapWithSomeHoles(x, y, filename):
    """
    Generate map with some (at most 1/4) random holes

    :param x: x dimension of the map
    :param y: y dimension of the map
    :param filename: path to file where to write the map
    """

    numberOfPositions = x * y
    numberOfHoles = randint(1, numberOfPositions//4)
    i = 0
    for i in range(math.comb(numberOfPositions, numberOfHoles)):
        padMap = [[True for _ in range(y)] for _ in range(x)]
        for _ in range(numberOfHoles):
            newHole = False
            while (not newHole):
                rx = randint(0, x - 1)
                ry = randint(0, y - 1)
                if (padMap[rx][ry]):
                    padMap[rx][ry] = False
                    newHole = True
        if (isMapConnected(x, y, padMap)):
            __printMapToFile(x, y, padMap, filename)
            return
    print ("Was not able to generate map " + str(x) + "x" + str(y) + " with " + str(numberOfHoles) + " holes on " + str(i) + " tries.")


def __getXRange(vertices):
    initV = vertices[0]
    minX = initV[0]
    maxX = initV[0]
    for v in vertices:
        if v[0] < minX:
            minX = v[0]
        if v[0] > maxX:
            maxX = v[0]
    return maxX - minX + 1

def __getYRange(vertices):
    initV = vertices[0]
    minY = initV[1]
    maxY = initV[1]
    for v in vertices:
        if v[1] < minY:
            minY = v[1]
        if v[1] > maxY:
            maxY = v[1]
    return maxY - minY + 1

def __addNewNode(x, y, vertices):
    newNode = False
    while(not newNode):
        v = random.choices(vertices, weights=[i for i in range(1, len(vertices) + 1)], k=1)[0]
        directions = ["N", "E", "S", "W"]
        d = random.choice(directions)
        newNode = False
        if (d == "N"):
            newV = (v[0], v[1] + 1)
        if (d == "E"):
            newV = (v[0] + 1, v[1])
        if (d == "S"):
            newV = (v[0], v[1] - 1)
        if (d == "W"):
            newV = (v[0] - 1, v[1])
        if (d == "N" or d == "S"):
            yRange = __getYRange(vertices + [newV])
            if (yRange <= y and newV not in vertices):
                vertices.append(newV)
                newNode = True
        elif (d == "E" or d == "W"):
            xRange = __getXRange(vertices + [newV])
            if (xRange <= x and newV not in vertices):
                vertices.append(newV)
                newNode = True

def generateMapWithManyHoles(x, y, filename):
    """
    Generate map with many (at least 1/4) random holes

    :param x: x dimension of the map
    :param y: y dimension of the map
    :param filename: path to file where to write the map
    """

    totalNumberOfPositions = x * y 
    numberOfHoles = randint(totalNumberOfPositions // 4, totalNumberOfPositions - 1)
    numberOfPositions = totalNumberOfPositions - numberOfHoles
    vertices = [(0, 0)]
    for i in range(numberOfPositions - 1):
        __addNewNode(x, y, vertices)
    
    minX = min(vertices, key=lambda a:a[0])[0]
    minY = min(vertices, key=lambda a:a[1])[1]
    padMap = [[False for _ in range(y)] for _ in range(x)]
    for v in vertices:
        padMap[v[0] - minX][v[1] - minY] = True

    __printMapToFile(x, y, padMap, filename)


def generate(argv):
    """
    Generate map by arguments

    :param argv: list of arguments [x dimension, y dimension, filename, mapType optional]
    """
    
    if (len(argv) < 3):
        print("Not enough arguments.")
        sys.exit(2)
    if (not argv[0].isdigit()):
        print(argv[0] + " is not a digit.")
        sys.exit(2)
    x = int(argv[0])
    if (not argv[1].isdigit()):
        print(argv[1] + " is not a digit.")
        sys.exit(2)
    y = int(argv[1])

    filename = argv[2]

    mapType = "r"
    if (len(argv) > 3):
         mapType = argv[3]
    
    if (mapType in ("r", "rectangle")):
        generateRectangleMap(x, y, filename)
    elif (mapType in ("o", "1", "one")):
        generateRectangleWithOneRandomHole(x, y, filename)
    elif (mapType in ("s", "some")):
        generateMapWithSomeHoles(x, y, filename)
    elif (mapType in ("m", "many")):
        generateMapWithManyHoles(x, y, filename)
    else:
        print("Uknown map type. Use r, o, s or m.")

if __name__ == "__main__":
    generate(sys.argv[1:])