from enum import Enum, auto

# possible names
# rofibot best
# early end detection
class DFSStrategy(Enum):
    """
    Enum class representing possible strategies for dfs algorithms

    The value is triple
    The first value is list of possible strategies names
    The second value says if the strategy uses knowledge of the rofibot and uses its best
    The third value says if the strategy uses early end detection
    """

    STRICT = (["strict", "s"], False, False) # oder N, E, S, W
    RANDOM = (["ranodom", "r"], False, False)
    STRICT_ROFIBOT_BEST = (["strict-best", "sb", "strict_best", "strictbest"], True, False) # looks how the rofibot is positioned and puts the worst possibility as the last one
    RANDOM_ROFIBOT_BEST = (["random-best", "rb", "random_best", "randombest"], True, False)

    STRICT_EARLY = (["strict-early", "se", "strict_early", "strictearly"], False, True)
    RANDOM_EARLY = (["ranodom-early", "re", "random_early", "randomearly"], False, True)
    STRICT_ROFIBOT_BEST_EARLY = (["strict-best-early", "sbe", "seb", "strict_best_early", "strictbestearly"], True, True) 
    RANDOM_ROFIBOT_BEST_EARLY = (["random-best-early", "rbe", "reb", "random_best_early", "randombestearly"], True, True)


class BacktrackStrategy(Enum):
    """
    Enum class representing possible strategies for shortest backtrack algorithms

    The value is unit
    The value is list of possible strategies names
    """

    SCAN = (["scan", "s"],)
    RANDOM = (["random", "r"],)
    RANDOM_ROFIBOT_BEST = (["random-best", "rb"],)


def getStrategyByName(name):
    """
    Gets the strategy by its name

    Does not depend on strategy type

    :param name: name of the strategy
    :return: strategy instance or None if does not exist
    """

    for strat in DFSStrategy:
        if (name.lower() in strat.value[0]):
            return strat
    for strat in BacktrackStrategy:
        if (name.lower() in strat.value[0]):
            return strat
    return None

def getStrategyByNameAndType(name, strategyType):
    """
    Gets the strategy by its name and type

    :param name: name of the strategy
    :param type: type of strategy
    :return: strategy instance or None if strategy of strategyType with name name does not exist
    """

    for strat in strategyType:
        if (name.lower() in strat.value[0]):
            return strat
    return None