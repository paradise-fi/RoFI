#!/usr/bin/python

# https://www.tutorialspoint.com/python/python_command_line_arguments.htm

import sys, getopt
from enums import Ori, Side, Dock, Degree, RofibotType
from actions import StatisticsFunction, getStatisticsFunctionByName
from dfs import dfsIterative
from directZigZag import walkDirect, walkZigZag
from strategies import DFSStrategy, getStrategyByName, BacktrackStrategy, getStrategyByNameAndType
from algorithms import Algorithm, getAlgorithmByName
from rofibots import SingleRofibot, DoubleRofibot
from common import Node, Pad, Origin
from configuration import getProcessConfigsFunctionByName, ProcessConfigsFunction




def __run(arguments):
    """
    Run the algorithm for walking the pad, process the results by arguments

    Can end with exit code 2 if some conditions are not met

    :param arguments: arguments instance
    :return: list of configurations, rofibot actions and rofibot non-concurent actions
    """

    rofibot = __createRofibot(arguments.rofibotType)
    origin = Origin(arguments.x, arguments.y, arguments.ori)
    pad = Pad(arguments.padFilename)
    if (not pad.isConnected()):
        print("Pad is not connected, cannot work with that.")
        sys.exit(2)

    if (not arguments.algorithm.value[0] and not pad.isRectangle()):
        print("Algorithm " + arguments.algorithm.value[3][0] + " can use only rectangle pad (without holes).")
        sys.exit(2)

    if (not pad.canConnect(arguments.x, arguments.y)):
        print("Rofibots initial position is not on the pad.")
        sys.exit(2)

    if (arguments.strategy is None):
        configs = arguments.algorithm.value[2](rofibot, pad, origin, arguments.thinkOneStepFurther)
    else:
        configs = arguments.algorithm.value[2](rofibot, pad, origin, arguments.strategy, arguments.thinkOneStepFurther)

    if (arguments.configsFilename is None):
        arguments.processCongfigsFunc.value[1](configs, pad.config)
    else:
        arguments.processCongfigsFunc.value[1](configs, pad.config, arguments.configsFilename)

    if (arguments.statisticsFilename is None):
        arguments.statisticsFunc.value[1](rofibot.actions, rofibot.nonConcurentActions)
    else:
        arguments.statisticsFunc.value[1](arguments.statisticsFilename, rofibot.actions, rofibot.nonConcurentActions)
    
    return configs, rofibot.actions, rofibot.nonConcurentActions

def __createRofibot(rofibotType):
    """
    Creates rofibot by type

    :param rofibotType: type of rofibot
    :return: rofibot instance
    """

    if (rofibotType is RofibotType.SINGLE):
        return SingleRofibot("20001")
    if (rofibotType is RofibotType.DOUBLE):
        return DoubleRofibot("20001", "20002")
        

def printExactlyOne(short, long):
    print("There must be exactly one " + short + " or " + long + " option.")

def printAtMostOne(short, long):
    print("There must be at most one " + short + " or " + long + " option.")

def printNotAvailable(option, dependency):
    print("The option " + option + " is not available with " + dependency + ".")

def printNeedsOption(option, dependency):
    print("There must be " + option + " specified with " + dependency + ".")


class Arguments:
    """
    Class representing arguments from cmd
    """

    rofibotType = None
    padFilename = None 
    x, y = None, None
    ori = None
    algorithm = None
    statisticsFunc = None
    statisticsFilename = None
    processCongfigsFunc = None
    configsFilename = None
    strategy = None
    strategyName = None
    thinkOneStepFurther = None

def checkArgumentsAndSetDefault(arguments):
    """
    Check arguments and set default if not set

    :param arguments: arguments instance to be checked
    :return: True if arguments are ok, False otherwise
    """
    # check all mandatory arguments
    if (arguments.padFilename is None or arguments.padFilename == ""):
        printExactlyOne("-p", "--padFile")
        return False
    
    # set default values:
    if (arguments.rofibotType is None):
        arguments.rofibotType = RofibotType.SINGLE
    if (arguments.x is None):
        arguments.x = 0
    if (arguments.y is None):
        arguments.y = 0
    if (arguments.ori is None):
        arguments.ori = Ori.N
    if (arguments.algorithm is None):
        arguments.algorithm = Algorithm.DFS
    if (arguments.statisticsFunc is None):
        arguments.statisticsFunc = StatisticsFunction.PRINT_TO_STDOUT
    if (arguments.processCongfigsFunc is None):
        arguments.processCongfigsFunc = ProcessConfigsFunction.PRINT_TO_STDOUT
    if (arguments.thinkOneStepFurther is None):
        arguments.thinkOneStepFurther = True
    
    if (arguments.statisticsFilename is not None and not arguments.statisticsFunc.value[0]):
        printNotAvailable("-v or --statFile", "statistics output function " + arguments.statisticsFunc.value[2][0])
        return False
    if (arguments.statisticsFilename is None and arguments.statisticsFunc.value[0]):
        printNeedsOption("-v or --statFile", "statistics output function " + arguments.statisticsFunc.value[2][0])
        return False

    if (arguments.configsFilename is not None and not arguments.processCongfigsFunc.value[0]):
        printNotAvailable("-u or --configsFile", "configurations output function " + arguments.processCongfigsFunc.value[2][0])
        return False
    if (arguments.configsFilename is None and arguments.processCongfigsFunc.value[0]):
        printNeedsOption("-u or --configsFile", "configurations output function " + arguments.processCongfigsFunc.value[2][0])
        return False

    if (arguments.algorithm.value[1] is None):
        if (arguments.strategy is not None):
            printNotAvailable("-g or --strategy", "algorithm function " + arguments.algorithm.value[3][0])
            return False
    else: # algorithm strategy is not None
        if (arguments.strategy is None):
            # set default
            if (arguments.algorithm.value[1] is DFSStrategy):
                arguments.strategy = DFSStrategy.STRICT
            elif (arguments.algorithm.value[1] is BacktrackStrategy):
                arguments.strategy = BacktrackStrategy.SCAN
        else:
            arguments.strategy = getStrategyByNameAndType(arguments.strategyName, arguments.algorithm.value[1])
            if (arguments.strategy is None):
                print("--------", arguments.strategyName, arguments.algorithm.value[1])
                printNotAvailable("-g or --strategy " + arguments.strategyName, "algorithm function " + arguments.algorithm.value[3][0])
                return False
    
    return True


def __parseInputArguments(argv, arguments, helpText):
    """
    Parse cmd arguments

    If there is error in arguments, ends with error code 2.

    :param argv: list of arguments from cmd
    :param arguments: instance of arguments, where to store parsed arguments
    :param helpText: text for printing help
    """

    try:
        opts, args = getopt.getopt(argv,"ht:p:x:y:o:a:s:v:c:u:g:f:",["help=", "type=", "padFile=", "x=", "y=", "ori=", "algorithm=", "statistcs=", "statFile=", "configs=", "configsFile=", "strategy=", "thinkFurther="])
    except getopt.GetoptError:
        print ("cannot read options")
        print(helpText)
        sys.exit(2)
    for opt, arg in opts:
        if opt in ("-h", "--help"):
            print(helpText)
            sys.exit()
        elif opt in ("-t", "--type"):
            if (arguments.rofibotType is not None):
                printAtMostOne("-t", "--type")
                sys.exit(2)
            if (arg.lower() == "s" or arg.lower() == "single"):
                arguments.rofibotType = RofibotType.SINGLE
            elif (arg.lower() == "d" or arg.lower() == "double"):
                arguments.rofibotType = RofibotType.DOUBLE
            else:
                print("Uknown rofibot type:", arg)
                sys.exit(2)
        elif opt in ("-p", "--padFile"):
            if (arguments.padFilename is not None):
                printAtMostOne("-p", "--padFile")
                sys.exit(2)
            try:
                f = open(arg, "r")
            except IOError:
                print("Could not open file " + arg + ".")
                sys.exit(2)
            f.close()
            arguments.padFilename = arg 
        elif opt in ("-x", "--x"):
            if (arguments.x is not None):
                printAtMostOne("-x", "--x")
                sys.exit(2)
            if (arg.isdigit()):
                arguments.x = int(arg)
            else:
                print("X coordinate " + arg + " is not an non-negative integer.")
                sys.exit(2)
        elif opt in ("-y", "--y"):
            if (arguments.y is not None):
                printAtMostOne("-y", "--y")
                sys.exit(2)
            if (arg.isdigit()):
                arguments.y = int(arg)
            else:
                print("Y coordinate " + arg + " is not an non-negative integer.")
                sys.exit(2)
        elif opt in ("-o", "--ori"):
            if (arguments.ori is not None):
                printAtMostOne("-o", "--ori")
                sys.exit(2)
            if (arg.lower() not in ("n", "e", "s", "w")):
                print ("Uknown orientation " + arg + ".")
                sys.exit(2)
            arguments.ori = Ori(arg.upper())
        elif opt in ("-a", "--algorithm"):
            if (arguments.algorithm is not None):
                printAtMostOne("-a", "--algorithm")
                sys.exit(2)
            arguments.algorithm = getAlgorithmByName(arg)
            if (arguments.algorithm is None):
                print("Uknown algorithm " + arg + ".")
                sys.exit(2)
        elif opt in ("-s", "--statistics"):
            if (arguments.statisticsFunc is not None):
                printAtMostOne("-s", "--statistics")
                sys.exit(2)
            arguments.statisticsFunc = getStatisticsFunctionByName(arg)
            if (arguments.statisticsFunc is None):
                print("Uknown statistics output function " + arg + ".")
                sys.exit(2) 
        elif opt in ("-v", "--statFile"):
            if (arguments.statisticsFilename is not None):
                printAtMostOne("-v", "--statFile")
                sys.exit(2)
            try:
                f = open(arg, "w+")
            except IOError:
                print("Could not open file " + arg + ".")
                sys.exit(2)
            f.close()
            arguments.statisticsFilename = arg 
        elif opt in ("-c", "--configs"):
            if (arguments.processCongfigsFunc is not None):
                printAtMostOne("-c", "--configs")
                sys.exit(2)
            arguments.processCongfigsFunc = getProcessConfigsFunctionByName(arg)
            if (arguments.processCongfigsFunc is None):
                print("Uknown configs output function " + arg + ".")
                sys.exit(2) 
        elif opt in ("-u", "--configsFile"):
            if (arguments.configsFilename is not None):
                printAtMostOne("-u", "--configsFile")
                sys.exit(2)
            try:
                f = open(arg, "w+")
            except IOError:
                print("Could not open file " + arg + ".")
                sys.exit(2)
            f.close()
            arguments.configsFilename = arg 
        elif opt in ("-g", "--strategy"):
            if (arguments.strategy is not None):
                printAtMostOne("-g", "--strategy")
                sys.exit(2)
            arguments.strategy = getStrategyByName(arg)
            arguments.strategyName = arg
            if (arguments.strategy is None):
                print("Uknown strategy " + arg + ".")
                sys.exit(2) 
        elif opt in ("-f", "--thinkFurther"):
            if (arguments.thinkOneStepFurther is not None):
                printAtMostOne("-f", "--thinkFurther")
                sys.exit(2)
            if (arg in ("y", "yes")):
                arguments.thinkOneStepFurther = True
            elif (arg in ("n", "no")):
                arguments.thinkOneStepFurther = False
            else:
                print("Uknown think one step further option " + arg + ".")
                sys.exit(2)

def main(argv):
    """
    The main function

    :param argv: list of cmd arguments
    :return: list of configurations, rofibot actions and rofibot non-concurent actions
    """

    arguments = Arguments()

    helpText = """ main.py 
    Computes sequence of configurations for rofibot walking on a pad.
    Usage: 
      python main.py [OPTION...]

      -h, --help            Print this help
      -t, --type arg        Rofibots type 
      -p, --padFile arg     Path to file with defined pad
      -x, --x arg           Initial x coordinate of the rofibot
      -y, --y arg           Initial y coordinate of the rofibot
      -o, --ori arg         Initial orientation of the rofibot (N, E, S or W)
      -a, --algorithm arg   Algorithm used for walking the pad
      -s, --statistics arg  Function to output statistics
      -v, --statFile arg    Path to file to save statistics
      -c, --configs arg     Function to output configurations
      -u, --configsFile arg Paht to file to save configurations
      -g, --strategy arg    Strategy for dfs or shortest backtrack algorithm
      -f, --thinkFurther    Think one step further, prepare rotation
    """ 

    __parseInputArguments(argv, arguments, helpText)


    ok = checkArgumentsAndSetDefault(arguments)
    if (not(ok)):
        sys.exit(2)
    
    return __run(arguments)
   

if __name__ == "__main__":
   main(sys.argv[1:])
