import unittest
from mapGenerator import generate
from directZigZag import walkDirect, walkZigZag, walkDirectZigZag
import utils
from common import Pad, Origin
from rofibots import SingleRofibot, DoubleRofibot
from enums import Ori, RofibotType
from strategies import DFSStrategy, BacktrackStrategy
from dfs import dfs, dfsWithBounds, dfsLookWithBounds, dfsLook
from shortestBacktrack import shortestBacktrack, shortestBacktrackLook

def __isRofibotId(idNum):
    return ((not idNum == 0) and (idNum >= 20000 or idNum < 10000))

def __isPadId(idNum):
    return not(__isRofibotId(idNum))

def getVisitedPadNodes(pad, configurations):
    visitedNodes = [[False for _ in range(pad.y)] for _ in range(pad.x)]
    for c in configurations:
        for edge in c.edges:
            leftId = int(edge.idLeft)
            rightId = int(edge.idRight)
            if (__isRofibotId(leftId) and __isPadId(rightId)):
                # left is rofibot (not the pad) and right is pad
                x, y = utils.getPositionFromId(edge.idRight)
                visitedNodes[x][y] = True 
    return visitedNodes

def compareVisitedAndPad(pad, visitedNodes):
    for x in range(pad.x):
        for y in range(pad.y):
            if (pad.map[x][y] and not visitedNodes[x][y]):
                return x, y, "Should be visited (" + str(x) + ", " + str(y) + ")"
            if (not(pad.map[x][y]) and visitedNodes[x][y]):
                return x, y, "Should not be visited (" + str(x) + ", " + str(y) + ")"
    return -1, -1, "Ok"


def testSkeleton(padFilename, rofibot, x, y, ori, walkFunction, strategy, thinkOneStepFurther):
    pad = Pad(padFilename)
    origin = Origin(x, y, ori)
    if (strategy is not None):
        configurations = walkFunction(rofibot, pad, origin, strategy, thinkOneStepFurther)
    else:
        configurations = walkFunction(rofibot, pad, origin, thinkOneStepFurther)
    visitedNodes = getVisitedPadNodes(pad, configurations)
    return compareVisitedAndPad(pad, visitedNodes)


def __createRofibot(rofibotType):
    if (rofibotType is RofibotType.SINGLE):
        return SingleRofibot("1")
    if (rofibotType is RofibotType.DOUBLE):
        return DoubleRofibot("1", "2")

def testSetSkeleton(testClass, startX, startY, doNotTest = []):
    for thinkOneStepFurther in (True, False):
            for ori in (Ori.N, Ori.E, Ori.S, Ori.W): 
                for strategy in testClass.strategies: 
                    for rofibotType in list(RofibotType):
                        rofibot = __createRofibot(rofibotType)
                        if (strategy is None):
                            paramsStr = " rofibot: " + rofibotType.name + ", start: " + str(startX) + ", " + str(startY) + ", ori: " + ori.value + ", thinkFurther: " + str(thinkOneStepFurther) + ", strategy: None"
                        else:
                            paramsStr = " rofibot: " + rofibotType.name + ", start: " + str(startX) + ", " + str(startY) + ", ori: " + ori.value + ", thinkFurther: " + str(thinkOneStepFurther) + ", strategy: " + strategy.value[0][0]
                        if ((thinkOneStepFurther, ori, strategy, rofibotType) in doNotTest):
                            #print ("skipped test", paramsStr)
                            continue
                        x, y, shouldBeVisited = testSkeleton(testClass.padFile, rofibot, startX, startY, ori, testClass.walkFunction(), strategy, thinkOneStepFurther)
                        errString = shouldBeVisited + paramsStr
                        testClass.assertEqual((x, y), (-1, -1), errString)

# Test recangle pads

if (True):

    
    class TestDirect_5x1(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map5x1.txt"       

        def walkFunction(self):
            return walkDirect
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

    class TestDirect_1x5(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map1x5.txt"       

        def walkFunction(self):
            return walkDirect
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

    class TestDirect_5x6(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map5x6.txt"       

        def walkFunction(self):
            return walkDirect
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestDirect_2x2(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map2x2.txt"     

        def walkFunction(self):
            return walkDirect

        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

    class TestDirect_7x9(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map7x9.txt"      

        def walkFunction(self):
            return walkDirect
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDirect_9x4(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map9x4.txt"      

        def walkFunction(self):
            return walkDirect
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
    class TestDirect_10x10(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map10x10.txt"      

        def walkFunction(self):
            return walkDirect
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

        def test_5_5(self):
            testSetSkeleton(self, 5, 5)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    

    
    class TestZigZag_5x1(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map5x1.txt"       

        def walkFunction(self):
            return walkZigZag
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

    class TestZigZag_1x5(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map1x5.txt"       

        def walkFunction(self):
            return walkZigZag
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

    class TestZigZag_5x6(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map5x6.txt"       

        def walkFunction(self):
            return walkZigZag
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestZigZag_2x2(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map2x2.txt"     

        def walkFunction(self):
            return walkZigZag

        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

    class TestZigZag_7x9(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map7x9.txt"      

        def walkFunction(self):
            return walkZigZag
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestZigZag_9x4(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map9x4.txt"      

        def walkFunction(self):
            return walkZigZag
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
    class TestZigZag_10x10(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map10x10.txt"      

        def walkFunction(self):
            return walkZigZag
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

        def test_5_5(self):
            testSetSkeleton(self, 5, 5)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    
    class TestZigZag_6x4(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map6x4.txt"      

        def walkFunction(self):
            return walkZigZag
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    

    
    class TestDirectZigZag_5x1(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map5x1.txt"       

        def walkFunction(self):
            return walkDirectZigZag
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

    class TestDirectZigZag_1x5(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map1x5.txt"       

        def walkFunction(self):
            return walkDirectZigZag
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

    class TestDirectZigZag_5x6(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map5x6.txt"       

        def walkFunction(self):
            return walkDirectZigZag
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestDirectZigZag_2x2(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map2x2.txt"     

        def walkFunction(self):
            return walkDirectZigZag

        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

    class TestDirectZigZag_7x9(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map7x9.txt"      

        def walkFunction(self):
            return walkDirectZigZag
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDirectZigZag_9x4(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map9x4.txt"      

        def walkFunction(self):
            return walkDirectZigZag
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
    class TestDirectZigZag_10x10(unittest.TestCase):

        strategies = [None]
        padFile = "./testData/maps/rectangle/map10x10.txt"      

        def walkFunction(self):
            return walkDirectZigZag
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

        def test_5_5(self):
            testSetSkeleton(self, 5, 5)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    


    class TestDFSWithBounds_5x1(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map5x1.txt"       

        def walkFunction(self):
            return dfsWithBounds
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

    class TestDFSWithBounds_1x5(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map1x5.txt"       

        def walkFunction(self):
            return dfsWithBounds
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

    class TestDFSWithBounds_5x6(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map5x6.txt"      

        def walkFunction(self):
            return dfsWithBounds
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestDFSWithBounds_2x2(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map2x2.txt"     

        def walkFunction(self):
            return dfsWithBounds

        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

    class TestDFSWithBounds_7x9(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map7x9.txt"      

        def walkFunction(self):
            return dfsWithBounds
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFSWithBounds_9x4(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map9x4.txt"      

        def walkFunction(self):
            return dfsWithBounds
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
    class TestDFSWithBounds_10x10(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map10x10.txt"      

        def walkFunction(self):
            return dfsWithBounds
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

        def test_5_5(self):
            testSetSkeleton(self, 5, 5)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    
    class TestDFSWithBounds_6x4(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map6x4.txt"      

        def walkFunction(self):
            return dfsWithBounds
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    
    
    class TestDFS_5x1(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map5x1.txt"       

        def walkFunction(self):
            return dfs
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

    class TestDFS_1x5(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map1x5.txt"       

        def walkFunction(self):
            return dfs
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

    class TestDFS_5x6(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map5x6.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestDFS_2x2(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map2x2.txt"     

        def walkFunction(self):
            return dfs

        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

    class TestDFS_7x9(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map7x9.txt"      


        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFS_9x4(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map9x4.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
    class TestDFS_10x10(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map10x10.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

        def test_5_5(self):
            testSetSkeleton(self, 5, 5)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    
    class TestDFS_6x4(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map6x4.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    
    
    class TestDFSLookWithBounds_5x1(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map5x1.txt"       

        def walkFunction(self):
            return dfsLookWithBounds
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

    class TestDFSLookWithBounds_1x5(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map1x5.txt"       

        def walkFunction(self):
            return dfsLookWithBounds
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

    class TestDFSLookWithBounds_5x6(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map5x6.txt"      

        def walkFunction(self):
            return dfsLookWithBounds
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestDFSLookWithBounds_2x2(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map2x2.txt"     

        def walkFunction(self):
            return dfsLookWithBounds

        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

    class TestDFSLookWithBounds_7x9(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map7x9.txt"      

        def walkFunction(self):
            return dfsLookWithBounds
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFSLookWithBounds_9x4(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map9x4.txt"      

        def walkFunction(self):
            return dfsLookWithBounds
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
    class TestDFSLookWithBounds_10x10(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map10x10.txt"      

        def walkFunction(self):
            return dfsLookWithBounds
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

        def test_5_5(self):
            testSetSkeleton(self, 5, 5)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    
    class TestDFSLookWithBounds_6x4(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map6x4.txt"      

        def walkFunction(self):
            return dfsLookWithBounds
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    
   
    class TestDFSLook_5x1(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map5x1.txt"       

        def walkFunction(self):
            return dfsLook
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

    class TestDFSLook_1x5(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map1x5.txt"       

        def walkFunction(self):
            return dfsLook
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

    class TestDFSLook_5x6(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map5x6.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestDFSLook_2x2(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map2x2.txt"     

        def walkFunction(self):
            return dfsLook

        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

    class TestDFSLook_7x9(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map7x9.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFSLook_9x4(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map9x4.txt"      


        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
    class TestDFSLook_10x10(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map10x10.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

        def test_5_5(self):
            testSetSkeleton(self, 5, 5)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    
    class TestDFSLook_6x4(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/rectangle/map6x4.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    

      
    class TestShortestBacktrack_5x1(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map5x1.txt"       

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

    class TestShortestBacktrack_1x5(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map1x5.txt"       

        def walkFunction(self):
            return shortestBacktrack
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

    class TestShortestBacktrack_5x6(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map5x6.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestShortestBacktrack_2x2(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map2x2.txt"     

        def walkFunction(self):
            return shortestBacktrack

        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

    class TestShortestBacktrack_7x9(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map7x9.txt"      


        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestShortestBacktrack_9x4(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map9x4.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
    class TestShortestBacktrack_10x10(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map10x10.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

        def test_5_5(self):
            testSetSkeleton(self, 5, 5)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    
    class TestShortestBacktrack_6x4(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map6x4.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    
     
    class TestShortestBacktrackLook_5x1(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map5x1.txt"       

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

    class TestShortestBacktrackLook_1x5(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map1x5.txt"       

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

    class TestShortestBacktrackLook_5x6(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map5x6.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestShortestBacktrackLook_2x2(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map2x2.txt"     

        def walkFunction(self):
            return shortestBacktrackLook

        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

    class TestShortestBacktrackLook_7x9(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map7x9.txt"      


        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestShortestBacktrackLook_9x4(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map9x4.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
    class TestShortestBacktrackLook_10x10(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map10x10.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

        def test_5_5(self):
            testSetSkeleton(self, 5, 5)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    
    class TestShortestBacktrackLook_6x4(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/rectangle/map6x4.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
    
# End test rectangle pads

# Test pads with one hole

if (True):

    class TestDFS_5x6_1_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map5x6_1.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestDFS_5x6_2_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map5x6_2.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestDFS_5x6_3_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map5x6_3.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestDFS_5x6_4_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map5x6_4.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
        def test_3_2(self):
            testSetSkeleton(self, 3, 2)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)


    class TestDFSLook_5x6_1_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map5x6_1.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestDFSLook_5x6_2_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map5x6_2.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestDFSLook_5x6_3_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map5x6_3.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestDFSLook_5x6_4_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map5x6_4.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
        def test_3_2(self):
            testSetSkeleton(self, 3, 2)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)



    class TestShortestBacktrack_5x6_1_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map5x6_1.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestShortestBacktrack_5x6_2_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map5x6_2.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestShortestBacktrack_5x6_3_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map5x6_3.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestShortestBacktrack_5x6_4_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map5x6_4.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
        def test_3_2(self):
            testSetSkeleton(self, 3, 2)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)


    class TestShortestBacktrackLook_5x6_1_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map5x6_1.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestShortestBacktrackLook_5x6_2_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map5x6_2.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestShortestBacktrackLook_5x6_3_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map5x6_3.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

    class TestShortestBacktrackLook_5x6_4_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map5x6_4.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
        def test_3_2(self):
            testSetSkeleton(self, 3, 2)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)



    class TestDFS_2x2_1(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map2x2_1.txt" 

        def walkFunction(self):
            return dfs
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] 
            testSetSkeleton(self, 0, 0, doNotTest)

    class TestDFS_2x2_2(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map2x2_2.txt" 

        def walkFunction(self):
            return dfs
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """
        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)


    class TestDFSLook_2x2_1(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map2x2_1.txt" 

        def walkFunction(self):
            return dfsLook
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

    class TestDFSLook_2x2_2(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map2x2_2.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """
        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)


    class TestShortestBacktrack_2x2_1(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map2x2_1.txt" 

        def walkFunction(self):
            return shortestBacktrack
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] 
            testSetSkeleton(self, 0, 0, doNotTest)

    class TestShortestBacktrack_2x2_2(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map2x2_2.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """
        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)


    class TestShortestBacktrackLook_2x2_1(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map2x2_1.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] 
            testSetSkeleton(self, 0, 0, doNotTest)

    class TestShortestBacktrackLook_2x2_2(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map2x2_2.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """
        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)



    class TestDFS_4x2_1_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map4x2_1.txt" 

        def walkFunction(self):
            return dfs
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """

        def test_3_1(self):
            testSetSkeleton(self, 3, 1)

    class TestDFS_4x2_2_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map4x2_2.txt" 

        def walkFunction(self):
            return dfs
        
        def test_0_1(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_1(self):
            testSetSkeleton(self, 3, 1)

    class TestDFS_4x2_3_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map4x2_3.txt" 

        def walkFunction(self):
            return dfs
        
        def test_0_1(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_1(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 3, 1, doNotTest)


    class TestDFSLook_4x2_1_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map4x2_1.txt" 

        def walkFunction(self):
            return dfsLook

        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """

        def test_3_1(self):
            testSetSkeleton(self, 3, 1)

    class TestDFSLook_4x2_2_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map4x2_2.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_0_1(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_1(self):
            testSetSkeleton(self, 3, 1)

    class TestDFSLook_4x2_3_oneHole(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map4x2_3.txt" 

        def walkFunction(self):
            return dfsLook

        def test_0_1(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_1(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 3, 1, doNotTest)


    class TestShortestBacktrack_4x2_1_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map4x2_1.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """

        def test_3_1(self):
            testSetSkeleton(self, 3, 1)

    class TestShortestBacktrack_4x2_2_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map4x2_2.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_0_1(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_1(self):
            testSetSkeleton(self, 3, 1)

    class TestShortestBacktrack_4x2_3_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map4x2_3.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_0_1(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_1(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 3, 1, doNotTest)


    class TestShortestBacktrackLook_4x2_1_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map4x2_1.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """

        def test_3_1(self):
            testSetSkeleton(self, 3, 1)

    class TestShortestBacktrackLook_4x2_2_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map4x2_2.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_0_1(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_1(self):
            testSetSkeleton(self, 3, 1)

    class TestShortestBacktrackLook_4x2_3_oneHole(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map4x2_3.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_0_1(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_1(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 3, 1, doNotTest)



    class TestDFS_7x9_1_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map7x9_1.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFS_7x9_2_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map7x9_2.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFS_7x9_3_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map7x9_3.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)


    class TestDFSLook_7x9_1_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map7x9_1.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFSLook_7x9_2_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map7x9_2.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFSLook_7x9_3_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map7x9_3.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)


    class TestShortestBacktrack_7x9_1_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map7x9_1.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestShortestBacktrack_7x9_2_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map7x9_2.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestShortestBacktrack_7x9_3_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map7x9_3.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)


    class TestShortestBacktrackLook_7x9_1_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map7x9_1.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestShortestBacktrackLook_7x9_2_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map7x9_2.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestShortestBacktrackLook_7x9_3_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map7x9_3.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)



    class TestDFS_9x4_1_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map9x4_1.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFS_9x4_2_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map9x4_2.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFS_9x4_3_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map9x4_3.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_3(self):
            testSetSkeleton(self, 5, 3)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFS_9x4_4_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map9x4_4.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFS_9x4_5_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map9x4_5.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)


    class TestDFSLook_9x4_1_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map9x4_1.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFSLook_9x4_2_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map9x4_2.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFSLook_9x4_3_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map9x4_3.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_3(self):
            testSetSkeleton(self, 5, 3)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFSLook_9x4_4_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map9x4_4.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFSLook_9x4_5_oneHole(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/singleHole/map9x4_5.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)


    class TestShortestBacktrack_9x4_1_oneHole(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map9x4_1.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrack_9x4_2_oneHole(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map9x4_2.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrack_9x4_3_oneHole(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map9x4_3.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_3(self):
            testSetSkeleton(self, 5, 3)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrack_9x4_4_oneHole(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map9x4_4.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrack_9x4_5_oneHole(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map9x4_5.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)


    class TestShortestBacktrackLook_9x4_1_oneHole(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map9x4_1.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrackLook_9x4_2_oneHole(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map9x4_2.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrackLook_9x4_3_oneHole(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map9x4_3.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_3(self):
            testSetSkeleton(self, 5, 3)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrackLook_9x4_4_oneHole(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map9x4_4.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrackLook_9x4_5_oneHole(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/singleHole/map9x4_5.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)


# End test pads with one hole


# Test pads with some holes

if (True):

    class TestDFS_4x2_1_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map4x2_1.txt" 

        def walkFunction(self):
            return dfs
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_3_0(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 3, 0, doNotTest)
    
    class TestDFS_4x2_2_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map4x2_2.txt" 

        def walkFunction(self):
            return dfs
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        """
        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        """
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_1(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 3, 1, doNotTest)

    class TestDFS_4x2_3_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map4x2_3.txt" 

        def walkFunction(self):
            return dfs
        
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)

        def test_3_1(self):
            testSetSkeleton(self, 3, 1)


    class TestDFSLook_4x2_1_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map4x2_1.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_3_0(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 3, 0, doNotTest)
        
    class TestDFSLook_4x2_2_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map4x2_2.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        """
        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        """
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_1(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 3, 1, doNotTest)

    class TestDFSLook_4x2_3_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map4x2_3.txt" 

        def walkFunction(self):
            return dfsLook
        
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)

        def test_3_1(self):
            testSetSkeleton(self, 3, 1)


    class TestShortestBacktrack_4x2_1_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map4x2_1.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_3_0(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 3, 0, doNotTest)
    
    class TestShortestBacktrack_4x2_2_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map4x2_2.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        """
        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        """
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_1(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 3, 1, doNotTest)

    class TestShortestBacktrack_4x2_3_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map4x2_3.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)

        def test_3_1(self):
            testSetSkeleton(self, 3, 1)


    class TestShortestBacktrackLook_4x2_1_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map4x2_1.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_3_0(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 3, 0, doNotTest)
    
    class TestShortestBacktrackLook_4x2_2_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map4x2_2.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        """
        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        """
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_1(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 3, 1, doNotTest)

    class TestShortestBacktrackLook_4x2_3_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map4x2_3.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)

        def test_3_1(self):
            testSetSkeleton(self, 3, 1)



    class TestDFS_5x6_1_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map5x6_1.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_3_5(self):
            testSetSkeleton(self, 3, 5)

    class TestDFS_5x6_2_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map5x6_2.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)
        
        def test_2_5(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 5, doNotTest)

    class TestDFS_5x6_3_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map5x6_3.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_4(self):
            testSetSkeleton(self, 1, 4)

    class TestDFS_5x6_4_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map5x6_4.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
        def test_3_2(self):
            testSetSkeleton(self, 3, 2)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)


    class TestDFSLook_5x6_1_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map5x6_1.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_3_5(self):
            testSetSkeleton(self, 3, 5)

    class TestDFSLook_5x6_2_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map5x6_2.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)
        
        def test_2_5(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 5, doNotTest)

    class TestDFSLook_5x6_3_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map5x6_3.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_4(self):
            testSetSkeleton(self, 1, 4)

    class TestDFSLook_5x6_4_someHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map5x6_4.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
        def test_3_2(self):
            testSetSkeleton(self, 3, 2)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)


    class TestShortestBacktrack_5x6_1_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map5x6_1.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_3_5(self):
            testSetSkeleton(self, 3, 5)

    class TestShortestBacktrack_5x6_2_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map5x6_2.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)
        
        def test_2_5(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 5, doNotTest)

    class TestShortestBacktrack_5x6_3_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map5x6_3.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_4(self):
            testSetSkeleton(self, 1, 4)

    class TestShortestBacktrack_5x6_4_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map5x6_4.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
        def test_3_2(self):
            testSetSkeleton(self, 3, 2)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)


    class TestShortestBacktrackLook_5x6_1_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map5x6_1.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_3_5(self):
            testSetSkeleton(self, 3, 5)

    class TestShortestBacktrackLook_5x6_2_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map5x6_2.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)
        
        def test_2_5(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 5, doNotTest)

    class TestShortestBacktrackLook_5x6_3_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map5x6_3.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_4(self):
            testSetSkeleton(self, 1, 4)

    class TestShortestBacktrackLook_5x6_4_someHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map5x6_4.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
        def test_3_2(self):
            testSetSkeleton(self, 3, 2)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)



    class TestDFS_7x9_1_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map7x9_1.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_4(self):
            testSetSkeleton(self, 2, 4)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFS_7x9_2_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map7x9_2.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFS_7x9_3_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map7x9_3.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)
        
        def test_5_5(self):
            testSetSkeleton(self, 5, 5)

    class TestDFS_7x9_4_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map7x9_4.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        """
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)
        """

        def test_2_4(self):
            testSetSkeleton(self, 2, 4)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_5_5(self):
            testSetSkeleton(self, 5, 5)
        
    
    class TestDFSLook_7x9_1_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map7x9_1.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_4(self):
            testSetSkeleton(self, 2, 4)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFSLook_7x9_2_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map7x9_2.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFSLook_7x9_3_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map7x9_3.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)
        
        def test_5_5(self):
            testSetSkeleton(self, 5, 5)

    class TestDFSLook_7x9_4_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map7x9_4.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        """
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)
        """

        def test_2_4(self):
            testSetSkeleton(self, 2, 4)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_5_5(self):
            testSetSkeleton(self, 5, 5)


    class TestShortestBacktrack_7x9_1_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map7x9_1.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_4(self):
            testSetSkeleton(self, 2, 4)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestShortestBacktrack_7x9_2_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map7x9_2.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestShortestBacktrack_7x9_3_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map7x9_3.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)
        
        def test_5_5(self):
            testSetSkeleton(self, 5, 5)

    class TestShortestBacktrack_7x9_4_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map7x9_4.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        """
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)
        """

        def test_2_4(self):
            testSetSkeleton(self, 2, 4)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_5_5(self):
            testSetSkeleton(self, 5, 5)
        
    
    class TestShortestBacktrackLook_7x9_1_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map7x9_1.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_4(self):
            testSetSkeleton(self, 2, 4)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestShortestBacktrackLook_7x9_2_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map7x9_2.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestShortestBacktrackLook_7x9_3_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map7x9_3.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)
        
        def test_5_5(self):
            testSetSkeleton(self, 5, 5)

    class TestShortestBacktrackLook_7x9_4_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map7x9_4.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        """
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)
        """

        def test_2_4(self):
            testSetSkeleton(self, 2, 4)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_5_5(self):
            testSetSkeleton(self, 5, 5)
        
    

    class TestDFS_9x4_1_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map9x4_1.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        """
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)
        """

        """
        def test_2_3(self):
            testSetSkeleton(self, 2, 3)
        """

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFS_9x4_2_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map9x4_2.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """

        def test_3_0(self):
            testSetSkeleton(self, 3, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFS_9x4_3_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map9x4_3.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_3(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 5, 3, doNotTest)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFS_9x4_4_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map9x4_4.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)


    class TestDFSLook_9x4_1_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map9x4_1.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        """
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)
        """

        """
        def test_2_3(self):
            testSetSkeleton(self, 2, 3)
        """

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFSLook_9x4_2_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map9x4_2.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """

        def test_3_0(self):
            testSetSkeleton(self, 3, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFSLook_9x4_3_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map9x4_3.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_3(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 5, 3, doNotTest)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFSLook_9x4_4_someHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/someHoles/map9x4_4.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)



    class TestShortestBacktrack_9x4_1_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map9x4_1.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        """
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)
        """

        """
        def test_2_3(self):
            testSetSkeleton(self, 2, 3)
        """

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrack_9x4_2_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map9x4_2.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """

        def test_3_0(self):
            testSetSkeleton(self, 3, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrack_9x4_3_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map9x4_3.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_3(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 5, 3, doNotTest)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrack_9x4_4_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map9x4_4.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)


    class TestShortestBacktrackLook_9x4_1_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map9x4_1.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        """
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        """
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        """
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)
        """

        """
        def test_2_3(self):
            testSetSkeleton(self, 2, 3)
        """

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrackLook_9x4_2_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map9x4_2.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        """
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)
        """

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        """
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        """

        def test_3_0(self):
            testSetSkeleton(self, 3, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrackLook_9x4_3_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map9x4_3.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_3(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 5, 3, doNotTest)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrackLook_9x4_4_someHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/someHoles/map9x4_4.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)



# End test pads with some holes


# Test pad with many holes

if (True):

    class TestDFS_4x2_1_manyHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map4x2_1.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 0, doNotTest)
            
    class TestDFS_4x2_2_manyHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map4x2_2.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)


    class TestDFSLook_4x2_1_manyHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map4x2_1.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 0, doNotTest)
            
    class TestDFSLook_4x2_2_manyHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map4x2_2.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)


    class TestShortestBacktrack_4x2_1_manyHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map4x2_1.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 0, doNotTest)
            
    class TestShortestBacktrack_4x2_2_manyHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map4x2_2.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)


    class TestShortestBacktrackLook_4x2_1_manyHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map4x2_1.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 0, doNotTest)
            
    class TestShortestBacktrackLook_4x2_2_manyHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map4x2_2.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)



    class TestDFS_5x6_1_manyHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_1.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

    class TestDFS_5x6_2_manyHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_2.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 1, 0, doNotTest)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)
        
        def test_2_5(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 5, doNotTest)

    class TestDFS_5x6_3_manyHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_3.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_2_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 0, doNotTest)

        def test_3_3(self):
            testSetSkeleton(self, 3, 3)

        def test_3_4(self):
            testSetSkeleton(self, 3, 4)

    class TestDFS_5x6_4_manyHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_4.txt" 

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
        def test_3_2(self):
            testSetSkeleton(self, 3, 2)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)


    class TestDFSLook_5x6_1_manyHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_1.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

    class TestDFSLook_5x6_2_manyHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_2.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 1, 0, doNotTest)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)
        
        def test_2_5(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 5, doNotTest)

    class TestDFSLook_5x6_3_manyHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_3.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_2_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 0, doNotTest)

        def test_3_3(self):
            testSetSkeleton(self, 3, 3)

        def test_3_4(self):
            testSetSkeleton(self, 3, 4)

    class TestDFSLook_5x6_4_manyHoles(unittest.TestCase):
        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_4.txt" 

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
        def test_3_2(self):
            testSetSkeleton(self, 3, 2)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)


    class TestShortestBacktrack_5x6_1_manyHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_1.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

    class TestShortestBacktrack_5x6_2_manyHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_2.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 1, 0, doNotTest)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)
        
        def test_2_5(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 5, doNotTest)

    class TestShortestBacktrack_5x6_3_manyHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_3.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_2_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 0, doNotTest)

        def test_3_3(self):
            testSetSkeleton(self, 3, 3)

        def test_3_4(self):
            testSetSkeleton(self, 3, 4)

    class TestShortestBacktrack_5x6_4_manyHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_4.txt" 

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
        def test_3_2(self):
            testSetSkeleton(self, 3, 2)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)


    class TestShortestBacktrackLook_5x6_1_manyHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_1.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
        
        def test_0_2(self):
            testSetSkeleton(self, 0, 2)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

    class TestShortestBacktrackLook_5x6_2_manyHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_2.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_1_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 1, 0, doNotTest)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)
        
        def test_2_5(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 5, doNotTest)

    class TestShortestBacktrackLook_5x6_3_manyHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_3.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)
        
        def test_2_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 0, doNotTest)

        def test_3_3(self):
            testSetSkeleton(self, 3, 3)

        def test_3_4(self):
            testSetSkeleton(self, 3, 4)

    class TestShortestBacktrackLook_5x6_4_manyHoles(unittest.TestCase):
        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map5x6_4.txt" 

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)

        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)
        
        def test_3_2(self):
            testSetSkeleton(self, 3, 2)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)


    class TestDFS_7x9_1_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_1.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_7(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 7, doNotTest)

        def test_2_4(self):
            testSetSkeleton(self, 2, 4)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_6(self):
            testSetSkeleton(self, 5, 6)

    class TestDFS_7x9_2_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_2.txt"      

        def walkFunction(self):
            return dfs

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)
        
        def test_1_4(self):
            testSetSkeleton(self, 1, 4)

    class TestDFS_7x9_3_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_3.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 2, doNotTest)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)
        
        def test_5_4(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 5, 4, doNotTest)

    class TestDFS_7x9_4_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_4.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_6(self):
            testSetSkeleton(self, 5, 6)


    class TestDFSLook_7x9_1_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_1.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_7(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 7, doNotTest)

        def test_2_4(self):
            testSetSkeleton(self, 2, 4)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_6(self):
            testSetSkeleton(self, 5, 6)

    class TestDFSLook_7x9_2_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_2.txt"      

        def walkFunction(self):
            return dfsLook

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)
        
        def test_1_4(self):
            testSetSkeleton(self, 1, 4)

    class TestDFSLook_7x9_3_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_3.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 2, doNotTest)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)
        
        def test_5_4(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 5, 4, doNotTest)

    class TestDFSLook_7x9_4_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_4.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_6(self):
            testSetSkeleton(self, 5, 6)


    class TestShortestBacktrack_7x9_1_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_1.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_7(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 7, doNotTest)

        def test_2_4(self):
            testSetSkeleton(self, 2, 4)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_6(self):
            testSetSkeleton(self, 5, 6)

    class TestShortestBacktrack_7x9_2_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_2.txt"      

        def walkFunction(self):
            return shortestBacktrack

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)
        
        def test_1_4(self):
            testSetSkeleton(self, 1, 4)

    class TestShortestBacktrack_7x9_3_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_3.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 2, doNotTest)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)
        
        def test_5_4(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 5, 4, doNotTest)

    class TestShortestBacktrack_7x9_4_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_4.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_6(self):
            testSetSkeleton(self, 5, 6)


    class TestShortestBacktrackLook_7x9_1_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_1.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_7(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 7, doNotTest)

        def test_2_4(self):
            testSetSkeleton(self, 2, 4)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_6(self):
            testSetSkeleton(self, 5, 6)

    class TestShortestBacktrackLook_7x9_2_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_2.txt"      

        def walkFunction(self):
            return shortestBacktrackLook

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)
        
        def test_1_4(self):
            testSetSkeleton(self, 1, 4)

    class TestShortestBacktrackLook_7x9_3_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_3.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_2_1(self):
            testSetSkeleton(self, 2, 1)
        
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_0_2(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 2, doNotTest)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)
        
        def test_5_4(self):
            doNotTest = [(True, Ori.N, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.N, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 5, 4, doNotTest)

    class TestShortestBacktrackLook_7x9_4_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map7x9_4.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_5(self):
            testSetSkeleton(self, 2, 5)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_6(self):
            testSetSkeleton(self, 5, 6)



    class TestDFS_9x4_1_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_1.txt"      

        def walkFunction(self):
            return dfs
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFS_9x4_2_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_2.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
            
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_0(self):
            testSetSkeleton(self, 3, 0)

        def test_3_3(self):
            testSetSkeleton(self, 3, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFS_9x4_3_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_3.txt"      

        def walkFunction(self):
            return dfs
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_3(self):
            testSetSkeleton(self, 5, 3)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFS_9x4_4_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_4.txt"      

        def walkFunction(self):
            return dfs

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_6_2(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 6, 2, doNotTest)

        def test_2_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 1, doNotTest)
        
        def test_3_1(self):
            testSetSkeleton(self, 3, 1)

    class TestDFS_9x4_5_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_5.txt"      

        def walkFunction(self):
            return dfs
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_2_3(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 3, doNotTest)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)


    class TestDFSLook_9x4_1_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_1.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFSLook_9x4_2_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_2.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
            
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_0(self):
            testSetSkeleton(self, 3, 0)

        def test_3_3(self):
            testSetSkeleton(self, 3, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestDFSLook_9x4_3_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_3.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_3(self):
            testSetSkeleton(self, 5, 3)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestDFSLook_9x4_4_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_4.txt"      

        def walkFunction(self):
            return dfsLook

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_6_2(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 6, 2, doNotTest)

        def test_2_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 1, doNotTest)
        
        def test_3_1(self):
            testSetSkeleton(self, 3, 1)

    class TestDFSLook_9x4_5_manyHoles(unittest.TestCase):

        strategies = list(DFSStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_5.txt"      

        def walkFunction(self):
            return dfsLook
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_2_3(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 3, doNotTest)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)


    class TestShortestBacktrack_9x4_1_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_1.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrack_9x4_2_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_2.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
            
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_0(self):
            testSetSkeleton(self, 3, 0)

        def test_3_3(self):
            testSetSkeleton(self, 3, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestShortestBacktrack_9x4_3_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_3.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_3(self):
            testSetSkeleton(self, 5, 3)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrack_9x4_4_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_4.txt"      

        def walkFunction(self):
            return shortestBacktrack

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_6_2(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 6, 2, doNotTest)

        def test_2_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 1, doNotTest)
        
        def test_3_1(self):
            testSetSkeleton(self, 3, 1)

    class TestShortestBacktrack_9x4_5_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_5.txt"      

        def walkFunction(self):
            return shortestBacktrack
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_2_3(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 3, doNotTest)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)


    class TestShortestBacktrackLook_9x4_1_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_1.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_0_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 1, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrackLook_9x4_2_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_2.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)
            
        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_3_0(self):
            testSetSkeleton(self, 3, 0)

        def test_3_3(self):
            testSetSkeleton(self, 3, 3)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

    class TestShortestBacktrackLook_9x4_3_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_3.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_1_2(self):
            testSetSkeleton(self, 1, 2)
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)
        
        def test_1_0(self):
            testSetSkeleton(self, 1, 0)

        def test_0_0(self):
            testSetSkeleton(self, 0, 0)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)
        
        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_5_3(self):
            testSetSkeleton(self, 5, 3)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)

    class TestShortestBacktrackLook_9x4_4_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_4.txt"      

        def walkFunction(self):
            return shortestBacktrackLook

        def test_2_3(self):
            testSetSkeleton(self, 2, 3)

        def test_1_3(self):
            testSetSkeleton(self, 1, 3)

        def test_6_2(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 6, 2, doNotTest)

        def test_2_1(self):
            doNotTest = [(True, Ori.S, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.S, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 1, doNotTest)
        
        def test_3_1(self):
            testSetSkeleton(self, 3, 1)

    class TestShortestBacktrackLook_9x4_5_manyHoles(unittest.TestCase):

        strategies = list(BacktrackStrategy)
        padFile = "./testData/maps/manyHoles/map9x4_5.txt"      

        def walkFunction(self):
            return shortestBacktrackLook
        
        def test_0_1(self):
            testSetSkeleton(self, 0, 1)

        def test_0_0(self):
            doNotTest = [(True, Ori.E, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.E, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 0, 0, doNotTest)

        def test_2_0(self):
            testSetSkeleton(self, 2, 0)

        def test_2_3(self):
            doNotTest = [(True, Ori.W, s, RofibotType.SINGLE) for s in self.strategies] + [(False, Ori.W, s, RofibotType.SINGLE) for s in self.strategies]
            testSetSkeleton(self, 2, 3, doNotTest)

        def test_5_2(self):
            testSetSkeleton(self, 5, 2)

        def test_2_2(self):
            testSetSkeleton(self, 2, 2)


# End test pads with many holes


if __name__ == '__main__':
    unittest.main()
