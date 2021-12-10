#from enum import Enum
import utils
from common import Node, Pad, Origin, getConfig, reconnect, getNewDirection
from enums import Ori, Side, Dock, Degree
from actions import Action
from configuration import Module, Edge, Configuration

class Rofibot:
    """
    Abstract class representing basic rofibot

    Rofibot has fixedId, fixedSide, fixedDock and fixedOri - specify connection to the pad
    Rofibot has actions and nonConcurent actions - actions performed during the walk for energy analysis and time analysis
    """

    fixedId = None
    fixedSide = None
    fixedDock = None
    fixedOri = None

    def __init__(self):
        self.actions = {}
        self.nonConcurentActions = {}
        for action in Action:
            self.actions[action] = 0
            self.nonConcurentActions[action] = 0

    def incrementAction(self, action):
        """
        Increment action counter

        :param action: action to incerement
        """

        self.actions.update({action : self.actions.get(action) + 1})
    
    def incermentNonConcurentAction(self, action):
        """
        Increment non-concurent action counter

        :param action: action to increment
        """

        self.nonConcurentActions.update({action : self.nonConcurentActions.get(action) + 1})

    def incrementNonConcurentAndAllActions(self, action):
        """
        Increment both action and non-concurent action counter

        :param action: action to increment
        """

        self.incrementAction(action)
        self.incermentNonConcurentAction(action)

    def toString(self):
        """
        String representation of current rofibot configuration

        :return: string representation of rofibot configuration
        """

        pass

    def toConfiguration(self):
        """
        Get current rofibot configuration

        :return: rofibot configuration
        """

        pass
    
    def freePosition(self):
        """
        Get relative position where the rofibot is not connected (fixed) to the pad

        :return: x, y free position of the rofibot
        """

        pass

    def fixedPosition(self):
        """
        Get relative position where the rofibot is connected (fixed) to the pad

        :return: x, y fixed position of the rofibot
        """
        
        pass 

    def computeNewEdge(self):
        """
        Get edge properties on free position (where the rofibot can connect)

        :return: id, newSide, newDock, newOri of the new edge
        """

        pass 
    
    def getPadEdge(self):
        """
        Get edge where the rofibot is connected to the pad

        :return: id, side, dock, ori, position of the pad edge
        """

        return self.fixedId, self.fixedSide, self.fixedDock, self.fixedOri, self.fixedPosition()

    def computeReconfig(self, pad, origin, newNode, nextPossibleDirections = []):
        """
        Compute reconfiguration from current position to new node (rotation above the new node)

        :param pad: pad to connect
        :param origin: origin to convert relative and absolute positions and orientations
        :param newNode: new node where to connect
        :param nextPossibleDirections: list of next possible direction to prerotate the rofibot
        :return: list of configurations neede for reconfiguration
        """

        pass

    def getWorstStepDirection(self):
        """
        Get the most complex direction for next step

        :return: most complex direction
        """

        pass    

    def needsReconnectionInNextStep(self, newNode):
        """
        Decides if the rofibot will need reconnection to get to newNode

        :param newNode: new node where to get
        :return: True if the rofibot neeed reconnection while getting to newNode, False otherwise (can only rotate)
        """

        pass


class SingleRofibot(Rofibot):
    """
    Class representing rofibot made from single universal module
    """

    def __init__(self, id):
        Rofibot.__init__(self)
        self.ID = id

        # relative positions of side A and B
        self.xA, self.yA = 0, 0
        self.xB, self.yB = 1, 0

        self.fixedId = id
        self.fixedSide = Side.A
        self.fixedDock = Dock.PLUS_X
        self.fixedOri = Ori.W

        self.alpha = Degree.ZERO
        self.beta = Degree.ZERO
        self.gamma = Degree.ZERO

    def toString(self):
        return ["M " + self.ID + " " + str(self.alpha.value) + " " + str(self.beta.value) + " " + str(self.gamma.value)]

    def toConfiguration(self):
        m = Module(self.ID, self.alpha, self.beta, self.gamma)
        return Configuration([m], [])

    def freePosition(self):
        if (self.fixedSide is Side.A):
            return self.xB, self.yB
        return self.xA, self.yA

    def fixedPosition(self):
        if (self.fixedSide is Side.A):
            return self.xA, self.yA
        return self.xB, self.yB


    def __setAlpha(self, alpha):
        diff = utils.getDifference(self.alpha, alpha)
        self.alpha = alpha
        if (self.fixedSide is Side.A):
            if (diff == 90):
                self.incrementAction(Action.ROTATE_FIXED_A_B_90)
            elif (diff == 180):
                self.incrementAction(Action.ROTATE_FIXED_A_B_180)

            if ((self.fixedOri is Ori.E and alpha is Degree.ZERO) or
                (self.fixedOri is Ori.N and alpha is Degree.NINETY) or
                (self.fixedOri is Ori.S and alpha is Degree.MINUS_NINETY)):
                self.xB = self.xA - 1
                self.yB = self.yA
            elif ((self.fixedOri is Ori.S and alpha is Degree.ZERO) or
                 (self.fixedOri is Ori.E and alpha is Degree.NINETY) or
                 (self.fixedOri is Ori.W and alpha is Degree.MINUS_NINETY)):
                self.xB = self.xA
                self.yB = self.yA + 1
            elif ((self.fixedOri is Ori.W and alpha is Degree.ZERO) or
                 (self.fixedOri is Ori.S and alpha is Degree.NINETY) or
                 (self.fixedOri is Ori.N and alpha is Degree.MINUS_NINETY) ):
                self.xB = self.xA + 1
                self.yB = self.yA
            elif ((self.fixedOri is Ori.N and alpha is Degree.ZERO) or
                 (self.fixedOri is Ori.W and alpha is Degree.NINETY) or
                 (self.fixedOri is Ori.E and alpha is Degree.MINUS_NINETY) ):
                self.xB = self.xA
                self.yB = self.yA - 1
        else:
            if (diff == 90):
                self.incrementAction(Action.ROTATE_FREE_A_B_90)
            elif (diff == 180):
                self.incrementAction(Action.ROTATE_FREE_A_B_180)

    def __setBeta(self, beta):
        diff = utils.getDifference(self.beta, beta)
        self.beta = beta
        if (self.fixedSide is Side.B):
            if (diff == 90):
                self.incrementAction(Action.ROTATE_FIXED_A_B_90)
            elif (diff == 180):
                self.incrementAction(Action.ROTATE_FIXED_A_B_180)

            if ((self.fixedOri is Ori.E and beta is Degree.ZERO) or
                (self.fixedOri is Ori.N and beta is Degree.MINUS_NINETY) or
                (self.fixedOri is Ori.S and beta is Degree.NINETY)):
                self.xA = self.xB + 1
                self.yA = self.yB
            elif ((self.fixedOri is Ori.S and beta is Degree.ZERO) or
                 (self.fixedOri is Ori.E and beta is Degree.MINUS_NINETY) or
                 (self.fixedOri is Ori.W and beta is Degree.NINETY) ):
                self.xA = self.xB
                self.yA = self.yB - 1
            elif ((self.fixedOri is Ori.W and beta is Degree.ZERO) or
                 (self.fixedOri is Ori.S and beta is Degree.MINUS_NINETY) or
                 (self.fixedOri is Ori.N and beta is Degree.NINETY) ):
                self.xA = self.xB - 1
                self.yA = self.yB
            elif ((self.fixedOri is Ori.N and beta is Degree.ZERO) or
                 (self.fixedOri is Ori.W and beta is Degree.MINUS_NINETY) or
                 (self.fixedOri is Ori.E and beta is Degree.NINETY) ):
                self.xA = self.xB
                self.yA = self.yB + 1
        else:
            if (diff == 90):
                self.incrementAction(Action.ROTATE_FREE_A_B_90)
            elif (diff == 180):
                self.incrementAction(Action.ROTATE_FREE_A_B_180)

    def computeNewEdge(self):
        newSide, newDock, newOri = utils.computeNewEdge(self.fixedSide, self.fixedDock, self.fixedOri, self.alpha, self.beta, self.gamma)
        return self.ID, newSide, newDock, newOri
       

    def __computeBetaFromNextDirections(self, newDirection, nextPossibleDirections):    
        # compute rotation of beta for next directions to be easy (without reconnection)
        # can handle at most two next directions   
        if (len(nextPossibleDirections) == 0):
            return self.beta
        elif (len(nextPossibleDirections) > 1):
            if (utils.getLeftOri(newDirection) in nextPossibleDirections[0:2] and utils.getRightOri(newDirection) in nextPossibleDirections[0:2]):
                return Degree.ZERO
            elif (utils.getLeftOri(newDirection) in nextPossibleDirections[0:2] and newDirection in nextPossibleDirections[0:2]):
                return Degree.NINETY
            elif (utils.getRightOri(newDirection) in nextPossibleDirections[0:2] and newDirection in nextPossibleDirections[0:2]):
                return Degree.MINUS_NINETY
            # else contains way back, ignore it, always can go back
            
        nextDirection = nextPossibleDirections[0]
        if (nextDirection is utils.getOppositeOri(newDirection)):
            # next direction is way back, dont use it
            if (len(nextPossibleDirections) > 1):
                nextDirection = nextPossibleDirections[1]
            else:
                return self.beta

        if (nextDirection is newDirection):
            if (self.beta is Degree.NINETY or self.beta is Degree.MINUS_NINETY):
                return self.beta 
            else:
                return Degree.NINETY 
        elif (nextDirection is utils.getLeftOri(newDirection)):
            if (self.beta is Degree.ZERO or self.beta is Degree.NINETY):
                return self.beta
            else:
                return Degree.ZERO
        elif (nextDirection is utils.getRightOri(newDirection)):
            if (self.beta is Degree.ZERO or self.beta is Degree.MINUS_NINETY):
                return self.beta
            else:
                return Degree.ZERO

        return self.beta
    
    def __computeAlphaFromNextDirections(self, newDirection, nextPossibleDirections):       
        if (len(nextPossibleDirections) == 0):
            return self.alpha
        elif (len(nextPossibleDirections) > 1):
            if (utils.getLeftOri(newDirection) in nextPossibleDirections[0:2] and utils.getRightOri(newDirection) in nextPossibleDirections[0:2]):
                return Degree.ZERO
            elif (utils.getLeftOri(newDirection) in nextPossibleDirections[0:2] and newDirection in nextPossibleDirections[0:2]):
                return Degree.MINUS_NINETY
            elif (utils.getRightOri(newDirection) in nextPossibleDirections[0:2] and newDirection in nextPossibleDirections[0:2]):
                return Degree.NINETY
            # else contains way back, ignore it, always can go back
            
        nextDirection = nextPossibleDirections[0]
        if (nextDirection is utils.getOppositeOri(newDirection)):
            # next direction is way back, dont use it
            if (len(nextPossibleDirections) > 1):
                nextDirection = nextPossibleDirections[1]
            else:
                return self.alpha

        if (nextDirection is newDirection):
            if (self.alpha is Degree.NINETY or self.alpha is Degree.MINUS_NINETY):
                return self.alpha 
            else:
                return Degree.NINETY 
        elif (nextDirection is utils.getLeftOri(newDirection)):
            if (self.alpha is Degree.ZERO or self.alpha is Degree.MINUS_NINETY):
                return self.alpha
            else:
                return Degree.ZERO
        elif (nextDirection is utils.getRightOri(newDirection)):
            if (self.alpha is Degree.ZERO or self.alpha is Degree.NINETY):
                return self.alpha
            else:
                return Degree.ZERO

        return self.alpha
    
    def __computeNonConcurentActions(self, newAlpha, newBeta):
        alphaDiff = utils.getDifference(self.alpha, newAlpha)
        betaDiff = utils.getDifference(self.beta, newBeta)
        if (self.fixedSide is Side.A):
            fixedDiff = alphaDiff
            freeDiff = betaDiff
        else:
            fixedDiff = betaDiff
            freeDiff = alphaDiff
        if (fixedDiff == 180):
            self.incermentNonConcurentAction(Action.ROTATE_FIXED_A_B_180)
        elif (freeDiff == 180):
            self.incermentNonConcurentAction(Action.ROTATE_FREE_A_B_180)
        elif (fixedDiff == 90):
            self.incermentNonConcurentAction(Action.ROTATE_FIXED_A_B_90)       
        elif (freeDiff == 90):
            self.incermentNonConcurentAction(Action.ROTATE_FREE_A_B_90)

   
    def __oneStepTurnAlphaBeta(self, origin, newAlpha, newBeta):
        if (self.alpha is newAlpha and self.beta is newBeta):
            return []
        self.__computeNonConcurentActions(newAlpha, newBeta)
        self.__setAlpha(newAlpha)
        self.__setBeta(newBeta)
        return [getConfig(self, origin)]

    def __oneStepTurnAlphaAndRotateBeta(self, origin, newDirection, nextPossibleDirections):
        newBeta = self.__computeBetaFromNextDirections(newDirection, nextPossibleDirections)
        if (self.fixedOri is utils.getOppositeOri(newDirection)):
            newAlpha = Degree.ZERO
        elif (self.fixedOri is utils.getLeftOri(newDirection)):
            newAlpha =  Degree.MINUS_NINETY
        elif (self.fixedOri is utils.getRightOri(newDirection)):
            newAlpha =  Degree.NINETY
        else: 
            newAlpha = self.alpha

        return self.__oneStepTurnAlphaBeta(origin, newAlpha, newBeta)

    def __oneStepTurnBetaAndRotateAlpha(self, origin, newDirection, nextPossibleDirections):
        newAlpha = self.__computeAlphaFromNextDirections(newDirection, nextPossibleDirections)
        if (self.fixedOri is newDirection):
            newBeta = Degree.ZERO
        elif (self.fixedOri is utils.getLeftOri(newDirection)):
            newBeta = Degree.MINUS_NINETY
        elif (self.fixedOri is utils.getRightOri(newDirection)):
            newBeta = Degree.NINETY
        else:
            newBeta = self.beta

        return self.__oneStepTurnAlphaBeta(origin, newAlpha, newBeta)
    
    def __incremetnFixedNonConcurent(self, fixedAngle, newAngle):
        diff = utils.getDifference(fixedAngle, newAngle)
        if (diff == 90):
            self.incermentNonConcurentAction(Action.ROTATE_FIXED_A_B_90)
        elif (diff == 180):
            self.incermentNonConcurentAction(Action.ROTATE_FIXED_A_B_180)
    
    def __oneStepTurnAlpha(self, origin, newAlpha):
        if (self.alpha == newAlpha):
            return []
        self.__incremetnFixedNonConcurent(self.alpha, newAlpha)
        self.__setAlpha(newAlpha)
        return [getConfig(self, origin)]
    
    def __oneStepTurnBeta(self, origin, newBeta):
        if (self.beta == newBeta):
            return []
        self.__incremetnFixedNonConcurent(self.beta, newBeta)
        self.__setBeta(newBeta)
        return [getConfig(self, origin)]
    
    def __turnFixedModule(self, pad, origin, angle, side):
        turnConfigs = []
        currConfigs = reconnect(self, pad, origin)
        turnConfigs.extend(currConfigs)
        if (side is Side.A):
            diff = utils.getDifference(self.alpha, angle)
            self.__setAlpha(angle)
        elif (side is Side.B):
            diff = utils.getDifference(self.beta, angle)
            self.__setBeta(angle)
        if (diff == 180):
            self.incermentNonConcurentAction(Action.ROTATE_FREE_A_B_180)
        elif (diff == 90):
            self.incermentNonConcurentAction(Action.ROTATE_FREE_A_B_90)
        turnConfigs.append(getConfig(self, origin))
        currConfigs = reconnect(self, pad, origin)
        turnConfigs.extend(currConfigs)
        return turnConfigs

    def __tryTurnFixedModuleAndReconfig(self, pad, origin, newNode, angle, side, nextPossibleDirections):
        configs = []
        if (pad.canConnect(*origin.relativeToAbsolutePosition(*self.freePosition()))):
            currConfigs = self.__turnFixedModule(pad, origin, angle, side)
            configs.extend(currConfigs)
            currConfigs = self.computeReconfig(pad, origin, newNode, nextPossibleDirections)
            configs.extend(currConfigs)
            return True, configs
        return False, []
 
    def __twoStepReconfig(self, pad, origin, newNode, nextPossibleDirections):
        configs = []
        if (self.fixedSide is Side.A):
            angle = self.alpha
        else:
            angle = self.beta
        angles = []
        # prepare angles in proper order (to minimize rotations)
        if (angle is Degree.ZERO):
            angles = [Degree.ZERO, Degree.NINETY, Degree.MINUS_NINETY]
        elif (angle is Degree.NINETY):
            angles = [Degree.NINETY, Degree.ZERO, Degree.MINUS_NINETY]
        else:
            angles = [Degree.MINUS_NINETY, Degree.ZERO, Degree.NINETY]

        for angleOrder in angles:
            if (self.fixedSide is Side.A):
                currConfigs = self.__oneStepTurnAlpha(origin, angleOrder)
            else:
                currConfigs = self.__oneStepTurnBeta(origin, angleOrder)
            configs.extend(currConfigs)
            newAngle = Degree.ZERO
            if (angleOrder is Degree.ZERO):
                # new angle must be different (must rotate alpha/beta somehow)
                newAngle = Degree.NINETY
            turned, currConfigs = self.__tryTurnFixedModuleAndReconfig(pad, origin, newNode, newAngle, self.fixedSide, nextPossibleDirections)
            if (turned):
                configs.extend(currConfigs)
                return configs	
        print("no possibility to go to node: ", origin.relativeToAbsolutePosition(newNode.x, newNode.y), " when fixed ", self.fixedSide.value, " to pad at", origin.relativeToAbsolutePosition(*self.fixedPosition()), " with ori ", origin.relativeToAbsoluteOri(self.fixedOri).value)
        return None

    def computeReconfig(self, pad, origin, newNode, nextPossibleDirections = []):
        newDirection = getNewDirection(self, newNode)
        if (self.fixedSide is Side.A):
            if (self.fixedOri is newDirection):
                # cannot rotate directly because of modules shape
                # try rotate alpha to 0, 90 and -90 (in proper order), reconnect and rotate alpha
                # to get proper rotation
                return self.__twoStepReconfig(pad, origin, newNode, nextPossibleDirections)
            return self.__oneStepTurnAlphaAndRotateBeta(origin, newDirection, nextPossibleDirections)

        elif (self.fixedSide is Side.B):
            if (self.fixedOri is utils.getOppositeOri(newDirection)):
                # cannot rotate directly because of modules shape
                # try rotate beta to 0, 90 and -90 (in proper order), reconnect and rotate beta
                # to get proper rotation
                return self.__twoStepReconfig(pad, origin, newNode, nextPossibleDirections)
            return self.__oneStepTurnBetaAndRotateAlpha(origin, newDirection, nextPossibleDirections)
    
        return "err"

    def getWorstStepDirection(self):
        if (self.fixedSide is Side.A):
            return self.fixedOri 
        return utils.getOppositeOri(self.fixedOri)
    
    def needsReconnectionInNextStep(self, newNode):
        newDirection = getNewDirection(self, newNode)
        if (self.fixedSide is Side.A):
            return newDirection is self.fixedOri
        else:
            return newDirection is utils.getOppositeOri(self.fixedOri)




class DoubleRofibot(Rofibot):
    """
    Class representing rofibot built from two universal modules

    One module is denoted red, the second blue
    """

    def __init__(self, idBlue, idRed):
        Rofibot.__init__(self)

        self.ID_BLUE = idBlue
        self.ID_RED = idRed

        self.alphaBlue, self.betaBlue, self.gammaBlue = Degree.ZERO, Degree.ZERO, Degree.ZERO
        self.alphaRed, self.betaRed, self.gammaRed = Degree.ZERO, Degree.ZERO, Degree.ZERO 

        self.fixedId = self.ID_BLUE
        self.fixedSide = Side.A 
        self.fixedDock = Dock.MINUS_Z
        self.fixedOri = Ori.E

        # relative positions of both modules
        self.xBlue, self.yBlue = 0, 0
        self.xRed, self.yRed = 1, 0

    def toString(self):
        blueModule = "M " + self.ID_BLUE + " " + str(self.alphaBlue.value) + " " + str(self.betaBlue.value) + " " + str(self.gammaBlue.value)
        redModule = "M " + self.ID_RED + " " + str(self.alphaRed.value) + " " + str(self.betaRed.value) + " " + str(self.gammaRed.value)
        edge = "E " + self.ID_BLUE + " " + Side.B.value + " " + Dock.MINUS_X.value + " " + Ori.S.value + " " + Dock.PLUS_X.value + " " + Side.B.value + " " + self.ID_RED
        return [blueModule, redModule, edge]

    def toConfiguration(self):
        blueModule = Module(self.ID_BLUE, self.alphaBlue, self.betaBlue, self.gammaBlue)
        redModule = Module(self.ID_RED, self.alphaRed, self.betaRed, self.gammaRed)
        edge = Edge(self.ID_BLUE, Side.B, Dock.MINUS_X, Ori.S, Dock.PLUS_X, Side.B, self.ID_RED)
        return Configuration([blueModule, redModule], [edge])

    def freePosition(self):
        if (self.fixedId == self.ID_BLUE):
            return self.xRed, self.yRed
        return self.xBlue, self.yBlue 
    
    def fixedPosition(self):
        if (self.fixedId == self.ID_BLUE):
            return self.xBlue, self.yBlue 
        return self.xRed, self.yRed

    def __setGammaBlue(self, gamma):
        diff = utils.getDifference(self.gammaBlue, gamma)
        if (diff == 90):
            self.incrementNonConcurentAndAllActions(Action.ROTATE_GAMMA_90)
        elif (diff == 180):
            self.incrementNonConcurentAndAllActions(Action.ROTATE_GAMMA_180)
            
        self.gammaBlue = gamma
        if (self.fixedId == self.ID_BLUE):
            if ((self.fixedOri is Ori.E and gamma is Degree.ZERO) or 
                (self.fixedOri is Ori.S and gamma is Degree.NINETY) or 
                (self.fixedOri is Ori.W and gamma is Degree.ONEHUNDREDEIGHTY) or 
                (self.fixedOri is Ori.N and gamma is Degree.MINUS_NINETY)):
                self.xRed = self.xBlue + 1
                self.yRed = self.yBlue
            elif ((self.fixedOri is Ori.S and gamma is Degree.ZERO) or
                  (self.fixedOri is Ori.W and gamma is Degree.NINETY) or 
                  (self.fixedOri is Ori.N and gamma is Degree.ONEHUNDREDEIGHTY) or 
                  (self.fixedOri is Ori.E and gamma is Degree.MINUS_NINETY)):
                self.xRed = self.xBlue 
                self.yRed = self.yBlue - 1
            elif ((self.fixedOri is Ori.W and gamma is Degree.ZERO) or
                  (self.fixedOri is Ori.N and gamma is Degree.NINETY) or 
                  (self.fixedOri is Ori.E and gamma is Degree.ONEHUNDREDEIGHTY) or 
                  (self.fixedOri is Ori.S and gamma is Degree.MINUS_NINETY)):
                self.xRed = self.xBlue - 1
                self.yRed = self.yBlue 
            elif ((self.fixedOri is Ori.N and gamma is Degree.ZERO) or
                  (self.fixedOri is Ori.E and gamma is Degree.NINETY) or 
                  (self.fixedOri is Ori.S and gamma is Degree.ONEHUNDREDEIGHTY) or 
                  (self.fixedOri is Ori.W and gamma is Degree.MINUS_NINETY)):
                self.xRed = self.xBlue
                self.yRed = self.yBlue + 1
    
    def __setGammaRed(self, gamma):
        diff = utils.getDifference(self.gammaRed, gamma)
        if (diff == 90):
            self.incrementNonConcurentAndAllActions(Action.ROTATE_GAMMA_90)
        elif (diff == 180):
            self.incrementNonConcurentAndAllActions(Action.ROTATE_GAMMA_180)

        self.gammaRed = gamma 
        if (self.fixedId == self.ID_RED):
            if ((self.fixedOri is Ori.E and gamma is Degree.ZERO) or 
                (self.fixedOri is Ori.S and gamma is Degree.NINETY) or 
                (self.fixedOri is Ori.W and gamma is Degree.ONEHUNDREDEIGHTY) or 
                (self.fixedOri is Ori.N and gamma is Degree.MINUS_NINETY)):
                self.xBlue = self.xRed - 1
                self.yBlue = self.yRed
            elif ((self.fixedOri is Ori.S and gamma is Degree.ZERO) or
                  (self.fixedOri is Ori.W and gamma is Degree.NINETY) or 
                  (self.fixedOri is Ori.N and gamma is Degree.ONEHUNDREDEIGHTY) or 
                  (self.fixedOri is Ori.E and gamma is Degree.MINUS_NINETY)):
                self.xBlue = self.xRed
                self.yBlue = self.yRed + 1
            elif ((self.fixedOri is Ori.W and gamma is Degree.ZERO) or
                  (self.fixedOri is Ori.N and gamma is Degree.NINETY) or 
                  (self.fixedOri is Ori.E and gamma is Degree.ONEHUNDREDEIGHTY) or 
                  (self.fixedOri is Ori.S and gamma is Degree.MINUS_NINETY)):
                self.xBlue = self.xRed + 1
                self.yBlue = self.yRed
            elif ((self.fixedOri is Ori.N and gamma is Degree.ZERO) or
                  (self.fixedOri is Ori.E and gamma is Degree.NINETY) or 
                  (self.fixedOri is Ori.S and gamma is Degree.ONEHUNDREDEIGHTY) or 
                  (self.fixedOri is Ori.W and gamma is Degree.MINUS_NINETY)):
                self.xBlue = self.xRed
                self.yBlue = self.yRed - 1

    def __getFixedGamma(self):
        if (self.fixedId == self.ID_BLUE):
            return self.gammaBlue 
        return self.gammaRed 

    def __getFreeGamma(self):
        if (self.fixedId == self.ID_BLUE):
            return self.gammaRed
        return self.gammaBlue
         
    def __computeNewOri(self):
        newOri = self.fixedOri
        # fixed rotation
        if (self.__getFixedGamma() is Degree.NINETY):
            newOri = utils.getLeftOri(newOri)
        elif (self.__getFixedGamma() is Degree.ONEHUNDREDEIGHTY):
            newOri = utils.getOppositeOri(newOri)
        elif (self.__getFixedGamma() is Degree.MINUS_NINETY):
            newOri = utils.getRightOri(newOri)

        # free rotation
        if (self.__getFreeGamma() is Degree.NINETY):
            newOri = utils.getRightOri(newOri)
        elif (self.__getFreeGamma() is Degree.ONEHUNDREDEIGHTY):
            newOri = utils.getOppositeOri(newOri)
        elif (self.__getFreeGamma() is Degree.MINUS_NINETY):
            newOri = utils.getLeftOri(newOri)
        
        return newOri

    def computeNewEdge(self):
        newSide = Side.A 
        newId = self.ID_RED if (self.fixedId == self.ID_BLUE) else self.ID_BLUE
        newDock = Dock.MINUS_Z
        newOri = self.__computeNewOri()
        return newId, newSide, newDock, newOri

    def __getCurrentDirection(self):
        # get the direction of the free module within the fixed module
        if (self.fixedId == self.ID_BLUE):
            fixedX, fixedY = self.xBlue, self.yBlue
            freeX, freeY = self.xRed, self.yRed 
        else:
            fixedX, fixedY = self.xRed, self.yRed
            freeX, freeY = self.xBlue, self.yBlue
        
        if (fixedX == freeX - 1):
            return Ori.E 
        if (fixedX == freeX + 1):
            return Ori.W 
        if (fixedY == freeY - 1):
            return Ori.N 
        if (fixedY == freeY + 1):
            return Ori.S
    
    def __getAngleRotation(self, currDirection, newDirection):
        if (currDirection is newDirection):
            return Degree.ZERO
        if (currDirection is utils.getOppositeOri(newDirection)):
            return Degree.ONEHUNDREDEIGHTY
        if (newDirection is utils.getLeftOri(currDirection)):
            return Degree.NINETY
        return Degree.MINUS_NINETY

    def __rotateFixedGamma(self, angleDiff):
        origGamma = self.__getFixedGamma().value
        newGamma = (origGamma + angleDiff.value) % 360
        if (newGamma == 270):
            newGamma = -90
        newGammaEnum = Degree(newGamma)
        if (self.fixedId == self.ID_BLUE):
            self.__setGammaBlue(newGammaEnum)
        else:
            self.__setGammaRed(newGammaEnum)

    def computeReconfig(self, pad, origin, newNode, nextPossibleDirections = []): 
        # can ignore nextPossibleDirections, this rofibot can go in any direction
        freeX, freeY = self.freePosition()
        fixedX, fixedY = self.fixedPosition()
        if ((freeX == newNode.x and freeY == newNode.y) or (fixedX == newNode.x and fixedY == newNode.y)):
            # already rotated
            return []

        newDirection = getNewDirection(self, newNode)
        currDirection = self.__getCurrentDirection()
        angleDiff = self.__getAngleRotation(currDirection, newDirection)
        self.__rotateFixedGamma(angleDiff)
        return  [getConfig(self, origin)]

    def getWorstStepDirection(self):
        return utils.getOppositeOri(self.__getCurrentDirection())

    def needsReconnectionInNextStep(self, newNode):
        # never needs reconnection, always can go in one step
        return False

   