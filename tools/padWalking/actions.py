from enum import Enum, auto

class Action(Enum):
    """
    Enum class representing possible unit actions of rofibots

    Incliding energy consumption
    (a, b) means a +- b J
    RoFI accumulator capacity: 15 500 J
    """

    ROTATE_FREE_A_B_90 = (5.5, 1)  # rotate free alpha or beta by 90 degrees
    ROTATE_FIXED_A_B_90 = (11, 2) # rotate fixed alpha or beta by 90 degrees -> rotates also the second part of the module
    ROTATE_FREE_A_B_180 = (10, 1)
    ROTATE_FIXED_A_B_180 = (18, 2) 
    ROTATE_GAMMA_90 = (20, 3)     # rotate gamma with the second module by 90 degrees
    ROTATE_GAMMA_180 = (30, 3)
    ADD_EDGE = (2, 0.6, "a")            # connects 
    REMOVE_EDGE = (2, 0.6, "r")         # disconnects

def computeTotalCost(actions):
    """
    Compute total energy consumption of actions

    :param actions: all actions of the rofibot
    """

    sumCosts = 0
    sumDiff = 0
    for action, number in actions.items():
        sumCosts += action.value[0] * number
        sumDiff += action.value[1] * number
    return sumCosts, sumDiff


def printStatisticsToStdout(statistics, nonConcurentActions = {}):
    """
    Print actions counts to stdout

    :param statistics: all actions of the rofibot to be printed (for energy analysis)
    :param nonConcurentActions: non-concurent actions of the rofibot to be printed (for time analysis)
    """
    
    print("All actions:")
    for action, number in statistics.items():
        print(action.name, ":", number)
    cost, diff = computeTotalCost(statistics)
    print("Cost:", cost)
    print()
    print("Non concurent actions:")
    for action, number in nonConcurentActions.items():
        print(action.name, ":", number)


def printStatisticsToFile(filename, statistics, nonConcurentActions = {}):
    """
    Print actions counts to file

    :param filename: path to file where to write the result
    :param statistics: all actions of the rofibot to be printed (for energy analysis)
    :param nonConcurentActions: non-concurent actions of the rofibot to be printed (for time analysis)
    """

    f = open(filename, "w+")
    f.write("All actions: \n")
    for action, number in statistics.items():
        f.write(action.name +  ": " +  str(number) + "\n")
    f.write("Cost: " + str(computeTotalCost(statistics)[0]) + "\n")
    f.write("\n")
    f.write("Non concurent actions: \n")
    for action, number in nonConcurentActions.items():
        f.write(action.name +  ": " +  str(number) + "\n")
    f.close()


def doNothing(statistics, nonConcurentActions = {}):
    """
    Ignore all statistics
    Do nothing with the actions

    :param statistics: all actions of the rofibot
    :param nonConcurentActions: non-concurent actions of the rofibot
    """
    pass


class StatisticsFunction(Enum):
    """
    Enum class representing possible statistics functions
    
    Defines possibilities how to process and output actions of the rofibot

    The value is triple. 
    The first value says if the function needs filename specified (True/False)
    The second value specifies the function which will be called
    The third value is a list of possible names used while running the whole program
    """

    PRINT_TO_STDOUT = (False, printStatisticsToStdout, ["stdout", "o", "out"])
    PRINT_TO_FILE = (True, printStatisticsToFile, ["file", "f"])
    DO_NOTHING = (False, doNothing, ["nothing", "n", "none"])


def getStatisticsFunctionByName(name):
    """
    Get statistics process function by its name

    :param name: name of the function
    :return: the function
    """

    for stat in StatisticsFunction:
        if (name.lower() in stat.value[2]):
            return stat
    return None
