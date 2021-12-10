from enums import Degree, Ori, Side, Dock
from enum import Enum

class Module:
    """
    Class representing universal module of rofibot in configuration

    Module has its ID and three degrees of freedom - alpha, beta, gamma.
    """

    ID = "0"
    alpha = Degree.ZERO
    beta = Degree.ZERO
    gamma = Degree.ZERO

    def __init__(self, id, alpha, beta, gamma):
        self.id = id
        self.alpha = alpha
        self.beta = beta
        self.gamma = gamma

    def toString(self):
        """
        String representation of module
        """

        return "M " + str(self.id) + " " + str(self.alpha.value) + " " + str(self.beta.value) + " " + str(self.gamma.value) 


class Edge:
    """
    Class representing connection (edge) between two modules of rofibot

    Edge defines connection between left module with idLeft, connected by side sideLeft with dock dockLeft with 
    right module with idRight, connected by side sideRight with dock dockRight in orientation ori
    """

    idLeft = "0"
    sideLeft = Side.A
    dockLeft = Dock.PLUS_X 
    ori = Ori.N 
    dockRight = Side.A 
    sideRight = Dock.PLUS_X 
    idRight = "1"

    def __init__(self, idLeft, sideLeft, dockLeft, ori, dockRight, sideRight, idRight):
        self.idLeft = idLeft
        self.sideLeft = sideLeft
        self.dockLeft = dockLeft
        self.ori = ori
        self.dockRight = dockRight
        self.sideRight = sideRight
        self.idRight = idRight

    def toString(self):
        """
        String representation of edge
        """

        return "E " + str(self.idLeft) + " " + self.sideLeft.value + " " + self.dockLeft.value + " " + \
         self.ori.value + " " + self.dockRight.value + " " + self.sideRight.value + " " + str(self.idRight)


class Configuration:
    """
    Class representign configuration of rofibot, pad 

    Configuration has list of modules and list of edges
    """

    modules = []
    edges = []

    def __init__(self, modules, edges):
        self.modules = modules
        self.edges = edges
    
    def toString(self):
        """
        String representation of configuration
        """

        modulesStr = []
        for m in self.modules:
            modulesStr.append(m.toString())
        edgesStr = []
        for e in self.edges:
            edgesStr.append(e.toString())

        if (len(self.modules) == 0 and len(self.edges) == 0):
            return ""
        
        if (len(self.modules) == 0):
            return '\n'.join(edgesStr)
        
        if (len(self.edges) == 0):
            return '\n'.join(modules)
        
        return '\n'.join(modulesStr) + '\n' + '\n'.join(edgesStr)

    def isEmpty(self):
        return len(self.modules) == 0 and len(self.edges) == 0


def printConfigs(configs, padConfig):
    """
    Print configurations to stdout

    :param configs: list of configurations of the rofibot
    :param padConfig: configuration of the pad built from rofibots
    """

    for c in configs:
        print("C")
        if (not padConfig.isEmpty()):
            print(padConfig.toString())
        print(c.toString())
        print()

def printConfigsToFile(configs, padConfig, filename):
    """
    Write configurations to file

    :param configs: list of configurations of the rofibot
    :param padConfig: configuration of the pad built from rofibots
    :param filename: path to file where to write the configurations
    """

    f = open(filename, "w+")
    for c in configs:
        f.write("C\n")
        if (not padConfig.isEmpty()):
            f.write(padConfig.toString() + "\n")
        f.write(c.toString() + "\n")
        f.write("\n")
    f.close()

def doNothing(cofigs, padConfig):
    """
    Ignore configurations, do nothing (do not write it)

    :param configs: list of configurations of the rofibot
    :param padConfig: configuration of the pad built from rofibots
    """

    pass

# Needs file
# function
# possible names
class ProcessConfigsFunction(Enum):
    """
    Enum class representing possible functions to process configurations

    The value is triple
    The first value says if the function needs a file
    The second value is the function
    The third value is list of possible names for the function
    """
    
    PRINT_TO_STDOUT = (False, printConfigs, ["stdout", "o", "out"])
    PRINT_TO_FILE = (True, printConfigsToFile, ["file", "f"])
    DO_NOTHING = (False, doNothing, ["nothing", "n", "none"])


def getProcessConfigsFunctionByName(name):
    """
    Get function to process configurations by its name

    :param name: name of the function
    :return: function to pocess configs instance
    """
    
    for func in ProcessConfigsFunction:
        if (name.lower() in func.value[2]):
            return func
    return None